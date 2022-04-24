#pragma once

#include <cstdint>
#include <msgpack.hpp>
#include <vector>
#include "components/protocol/insert_many.hpp"
#include "base.hpp"

namespace services::wal {

    using buffer_element_t = char;
    using buffer_t = std::vector<buffer_element_t>;

    using size_tt = std::uint16_t;
    using log_number_t = std::uint64_t;
    using crc32_t = std::uint32_t;

    template<class T>
    struct wal_entry_t final {
        wal_entry_t() {}
        size_tt size_;
        T entry_;
        crc32_t last_crc32_;
        log_number_t log_number_;
        statement_type type_;
        crc32_t crc32_;
    };

    crc32_t pack(buffer_t& storage, char* data, size_t size);
    buffer_t read_payload(buffer_t& input, int index_start, int index_stop);
    crc32_t read_crc32(buffer_t& input, int index_start);
    size_tt read_size_impl(buffer_t& input, int index_start);

    template<class T>
    crc32_t pack(buffer_t& storage, crc32_t last_crc32, log_number_t log_number, T& data) {
        msgpack::sbuffer buffer;
        msgpack::packer<msgpack::sbuffer> packer(buffer);

        packer.pack_array(4);
        packer.pack_fix_uint32(last_crc32);
        packer.pack_fix_uint64(log_number);
        packer.pack_char(static_cast<char>(data.type()));
        packer.pack(data);

        return pack(storage, buffer.data(), buffer.size());
    }

    template<class T>
    void unpack(buffer_t& storage, wal_entry_t<T>& entry) {
        msgpack::unpacked msg;
        msgpack::unpack(msg, storage.data(), storage.size());
        const auto& o = msg.get();
        entry.last_crc32_ = o.via.array.ptr[0].as<crc32_t>();
        entry.log_number_ = o.via.array.ptr[1].as<log_number_t>();
        entry.type_ = static_cast<statement_type>(o.via.array.ptr[2].as<char>());
        entry.entry_ = std::move(o.via.array.ptr[3].as<T>());
    }

} //namespace services::wal