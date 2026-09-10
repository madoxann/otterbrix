#pragma once

// B-068 hunt (debug branch, never merged): timestamped per-thread hop rings. Recording needs
// OTTERBRIX_HOP=1; the dispatcher watchdog dumps the recent timeline when an await goes stale,
// into OTTERBRIX_HOP_DIR (stderr without it). OTTERBRIX_HOP_STDERR=1 also prints every hop live.

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include <sys/syscall.h>
#include <unistd.h>

namespace core::hop {

    inline constexpr std::size_t ring_capacity = 2048;
    inline constexpr std::uint32_t max_dumps_per_process = 64;
    inline constexpr std::int64_t dump_window_us = 5'000'000;

    struct record_t final {
        std::int64_t steady_us{0};
        char text[160]{};
    };

    struct ring_t final {
        long tid{0};
        std::atomic<std::uint64_t> written{0};
        std::array<record_t, ring_capacity> records{};
    };

    struct registry_t final {
        std::mutex mutex;
        std::vector<ring_t*> rings;
        std::atomic<std::uint32_t> dumps{0};
    };

    inline bool env_flag(const char* name) noexcept {
        const char* value = std::getenv(name);
        return value != nullptr && value[0] == '1';
    }

    inline bool enabled() noexcept {
        static const bool on = env_flag("OTTERBRIX_HOP");
        return on;
    }

    inline std::int64_t steady_now_us() noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }

    // Leaked on purpose: threads still hop while static destructors run at exit.
    inline registry_t& registry() {
        static auto* instance = new registry_t;
        return *instance;
    }

    class thread_ring_t final {
    public:
        thread_ring_t()
            : ring_(new ring_t) {
            ring_->tid = static_cast<long>(::syscall(SYS_gettid));
            auto& reg = registry();
            std::lock_guard<std::mutex> guard(reg.mutex);
            reg.rings.push_back(ring_);
        }

        ~thread_ring_t() {
            auto& reg = registry();
            {
                std::lock_guard<std::mutex> guard(reg.mutex);
                reg.rings.erase(std::remove(reg.rings.begin(), reg.rings.end(), ring_), reg.rings.end());
            }
            delete ring_;
        }

        thread_ring_t(const thread_ring_t&) = delete;
        thread_ring_t& operator=(const thread_ring_t&) = delete;

        ring_t& ring() noexcept { return *ring_; }

    private:
        ring_t* ring_;
    };

    [[gnu::format(printf, 1, 2)]] inline void emit(const char* format, ...) noexcept {
        if (!enabled()) {
            return;
        }
        thread_local thread_ring_t owner;
        auto& ring = owner.ring();
        const auto index = ring.written.load(std::memory_order_relaxed);
        auto& record = ring.records[index % ring_capacity];
        record.steady_us = steady_now_us();
        va_list args;
        va_start(args, format);
        std::vsnprintf(record.text, sizeof(record.text), format, args);
        va_end(args);
        ring.written.store(index + 1, std::memory_order_relaxed);

        static const bool live = env_flag("OTTERBRIX_HOP_STDERR");
        if (live) {
            std::fprintf(stderr,
                         "[hop] %lld t%ld %s\n",
                         static_cast<long long>(record.steady_us),
                         ring.tid,
                         record.text);
        }
    }

    // The next dump's stream, or nullptr when recording is off or the process spent its dump budget.
    inline std::FILE* begin_dump(const char* reason) {
        if (!enabled()) {
            return nullptr;
        }
        const auto seq = registry().dumps.fetch_add(1, std::memory_order_relaxed);
        if (seq >= max_dumps_per_process) {
            return nullptr;
        }
        std::FILE* out = stderr;
        if (const char* dir = std::getenv("OTTERBRIX_HOP_DIR"); dir != nullptr && dir[0] != '\0') {
            const auto file_name =
                std::string(dir) + "/hop-" + std::to_string(::getpid()) + "-" + std::to_string(seq) + ".log";
            if (auto* file = std::fopen(file_name.c_str(), "w"); file != nullptr) {
                out = file;
            }
        }
        const auto wall_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count();
        std::fprintf(out,
                     "=== hop dump pid=%d seq=%u reason=%s steady_us=%lld wall_us=%lld\n",
                     ::getpid(),
                     seq,
                     reason,
                     static_cast<long long>(steady_now_us()),
                     static_cast<long long>(wall_us));
        return out;
    }

    // Every live ring's records from the last dump_window_us, merged into one timeline.
    inline void write_timeline(std::FILE* out) {
        struct line_t final {
            std::int64_t steady_us;
            long tid;
            std::string text;
        };
        const auto now = steady_now_us();
        std::vector<line_t> lines;
        {
            auto& reg = registry();
            std::lock_guard<std::mutex> guard(reg.mutex);
            for (auto* ring : reg.rings) {
                const auto written = ring->written.load(std::memory_order_relaxed);
                const auto kept = std::min<std::uint64_t>(written, ring_capacity);
                for (auto index = written - kept; index < written; ++index) {
                    const auto& record = ring->records[index % ring_capacity];
                    if (now - record.steady_us <= dump_window_us) {
                        lines.push_back(
                            {record.steady_us, ring->tid, std::string(record.text, strnlen(record.text, sizeof(record.text)))});
                    }
                }
            }
        }
        std::stable_sort(lines.begin(), lines.end(), [](const line_t& a, const line_t& b) {
            return a.steady_us < b.steady_us;
        });
        std::fprintf(out,
                     "--- timeline (%zu hops, last %lld ms)\n",
                     lines.size(),
                     static_cast<long long>(dump_window_us / 1000));
        for (const auto& line : lines) {
            std::fprintf(out, "%lld t%ld %s\n", static_cast<long long>(line.steady_us), line.tid, line.text.c_str());
        }
    }

    inline void end_dump(std::FILE* out) {
        if (out == stderr) {
            std::fflush(out);
            return;
        }
        std::fclose(out);
    }

} // namespace core::hop
