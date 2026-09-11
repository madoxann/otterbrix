#include <catch2/catch_test_macros.hpp>
#include <components/context/context.hpp>

// actor-zeta/spawn.hpp uses std::unique_ptr but does not include <memory>
#include <memory>

#include <actor-zeta/spawn.hpp>
#include <components/catalog/catalog_codes.hpp>
#include <components/catalog/catalog_oids.hpp>
#include <components/context/execution_context.hpp>
#include <components/log/log.hpp>
#include <components/session/session.hpp>
#include <components/table/column_definition.hpp>
#include <components/types/types.hpp>
#include <components/vector/data_chunk.hpp>
#include <core/non_thread_scheduler/scheduler_test.hpp>
#include <core/pmr.hpp>
#include <services/disk/manager_disk.hpp>
#include <services/wal/manager_wal_replicate.hpp>

#include "disk_test_helpers.hpp"

#include <chrono>
#include <filesystem>
#include <thread>
#include <unistd.h>

// B-068. An append that brings a new column sends PHYSICAL_ADD_COLUMN, then PHYSICAL_INSERT, from one disk agent
// handler and awaits the insert first. The WAL manager does not answer in send order, so the add_column reply can
// trail the insert reply; the agent must still finish the append.

using namespace services::disk;
namespace catalog = components::catalog;
using session_id_t = components::session::session_id_t;
using namespace std::chrono_literals;

namespace {
    using namespace disk_test_helpers;

    std::string reply_order_dir() {
        static std::string dir = "/tmp/test_otterbrix_add_column_reply_order_" + std::to_string(::getpid());
        return dir;
    }

    struct fixture {
        core::pmr::otterbrix_resource resource;
        log_t log;
        core::non_thread_scheduler::scheduler_test_t* scheduler;
        configuration::config_wal wal_config;
        configuration::config_disk disk_config;
        std::unique_ptr<manager_disk_t, actor_zeta::pmr::deleter_t> disk;
        std::unique_ptr<services::wal::manager_wal_replicate_t, actor_zeta::pmr::deleter_t> wal;

        explicit fixture(const std::string& dir)
            : log(initialization_logger("python", "/tmp/docker_logs/"))
            , scheduler(new core::non_thread_scheduler::scheduler_test_t(1, 1))
            , wal_config([&]() {
                configuration::config_wal c;
                c.path = dir;
                return c;
            }())
            , disk_config([&]() {
                configuration::config_disk c;
                c.path = dir;
                return c;
            }())
            , disk(actor_zeta::spawn<manager_disk_t>(&resource, scheduler, scheduler, disk_config, log))
            , wal(actor_zeta::spawn<services::wal::manager_wal_replicate_t>(&resource,
                                                                             scheduler,
                                                                             wal_config,
                                                                             log,
                                                                             disk->address(),
                                                                             components::pipeline::no_mailbox())) {
            std::filesystem::create_directories(dir);
            disk->set_manager_wal_sync(wal->address());
            disk->bootstrap_system_tables_sync();
        }

        ~fixture() {
            disk.reset();
            wal.reset();
            scheduler->stop();
            delete scheduler;
        }

        template<typename Fn, typename... Args>
        auto invoke(Fn fn, Args&&... args) {
            auto [_, future] = actor_zeta::otterbrix::send(disk->address(), fn, std::forward<Args>(args)...);
            for (int i = 0; i < 100000 && !future.is_ready(); ++i) {
                scheduler->run(1000);
                std::this_thread::yield();
            }
            REQUIRE(future.is_ready());
            return std::move(future).take_ready();
        }

        components::execution_context_t ctx() {
            return components::execution_context_t{session_id_t{}, components::table::transaction_data{0, 0}, {}};
        }

        // Agents and the WAL worker run only here; the disk and WAL pumps keep polling on their own threads.
        template<typename Pred>
        bool drive_until(Pred pred, std::chrono::milliseconds deadline) {
            const auto until = std::chrono::steady_clock::now() + deadline;
            while (!pred()) {
                if (std::chrono::steady_clock::now() >= until) {
                    return false;
                }
                scheduler->run(1000);
                std::this_thread::sleep_for(1ms);
            }
            return true;
        }

        void drive_for(std::chrono::milliseconds period) {
            drive_until([] { return false; }, period);
        }
    };

    std::pmr::vector<components::vector::data_chunk_t> one_row(std::pmr::memory_resource* resource,
                                                                bool with_new_column) {
        using components::types::complex_logical_type;
        using components::types::logical_type;
        std::pmr::vector<complex_logical_type> types(resource);
        types.push_back(complex_logical_type{logical_type::BIGINT});
        types.back().set_alias("a");
        if (with_new_column) {
            types.push_back(complex_logical_type{logical_type::STRING_LITERAL});
            types.back().set_alias("b");
        }
        components::vector::data_chunk_t chunk(resource, types, 1);
        chunk.set_cardinality(1);
        chunk.set_value(0, 0, std::int64_t{1});
        if (with_new_column) {
            chunk.set_value(1, 0, std::string_view("x"));
        }
        std::pmr::vector<components::vector::data_chunk_t> batch(resource);
        batch.emplace_back(std::move(chunk));
        return batch;
    }
} // namespace

TEST_CASE("services::disk::append_add_column::completes_when_the_add_column_reply_trails_the_insert_reply") {
    auto dir = reply_order_dir() + "/trailing_add_column";
    std::filesystem::remove_all(dir);
    {
        fixture fx(dir);
        auto ns_oid = test_create_namespace(fx, "ns");
        auto table_oid = test_create_table(fx,
                                           ns_oid,
                                           "docs",
                                           std::vector<components::table::column_definition_t>{},
                                           catalog::relkind::computed);
        fx.invoke(&manager_disk_t::create_storage_disk,
                  session_id_t{},
                  table_oid,
                  catalog::well_known_oid::main_database,
                  std::vector<components::table::column_definition_t>{},
                  /*is_computed=*/true);

        test_computed_register(fx, table_oid, "a", catalog::well_known_oid::int64_type);
        auto adopted = fx.invoke(&manager_disk_t::storage_append, txn_ctx(), table_oid, one_row(&fx.resource, false));
        REQUIRE_FALSE(adopted.has_error());

        test_computed_register(fx, table_oid, "b", catalog::well_known_oid::string_type);
        fx.wal->hold_add_column_replies();
        auto [_, append] = actor_zeta::otterbrix::send(fx.disk->address(),
                                                       &manager_disk_t::storage_append,
                                                       txn_ctx(),
                                                       table_oid,
                                                       one_row(&fx.resource, true));

        INFO("the PHYSICAL_ADD_COLUMN reply is held at the WAL manager");
        REQUIRE(fx.drive_until([&] { return fx.wal->held_add_column_replies() == 1; }, 5000ms));

        // The pump answers the PHYSICAL_INSERT within microseconds; this lets the agent take that reply.
        fx.drive_for(300ms);
        REQUIRE_FALSE(append.is_ready());

        fx.wal->release_add_column_replies();
        INFO("with both WAL replies in, whatever their order, the append completes");
        REQUIRE(fx.drive_until([&] { return append.is_ready(); }, 5000ms));
        auto appended = std::move(append).take_ready();
        REQUIRE_FALSE(appended.has_error());
        REQUIRE(appended.value().second == 1);
    }
    std::filesystem::remove_all(dir);
}
