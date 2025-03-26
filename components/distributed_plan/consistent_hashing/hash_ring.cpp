#include "hash_ring.hpp"
#include "murmur3.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace components;

namespace consistent_hashing {
    node_id_t hash_ring::get_node_by_hash(token_t hash) const {
        auto it = token_to_node.lower_bound(hash);

        // If reached the end, wrap around to the first token
        if (it == token_to_node.end()) {
            it = token_to_node.begin();
        }

        return it->second;
    }

    node_id_t hash_ring::get_node_by_key(document::value_t value) const {
        static auto get = []<typename T>(T value, uint64_t sz) {
            const uint64_t m = 0xc6a4a7935bd1e995ULL;
            uint64_t h = 0x8445d61a4e774912ULL ^ (sz * m);

            void* data = &value;
            return murmurhash3_x64_128(data, sz, h);
        };

        uint64_t hash;
        switch (value.physical_type()) {
            case types::physical_type::BOOL:
                hash = get(value.as_bool(), sizeof(bool));
                break;
            case types::physical_type::UINT8:
            case types::physical_type::UINT16:
            case types::physical_type::UINT32:
            case types::physical_type::UINT64:
                hash = get(value.as_unsigned(), sizeof(uint64_t));
                break;
            case types::physical_type::INT8:
            case types::physical_type::INT16:
            case types::physical_type::INT32:
            case types::physical_type::INT64:
                hash = get(value.as_int(), sizeof(int64_t));
                break;
            case types::physical_type::UINT128:
            case types::physical_type::INT128:
                hash = get(value.as_int128(), sizeof(absl::int128));
                break;
            case types::physical_type::FLOAT:
                hash = get(value.as_float(), sizeof(float));
                break;
            case types::physical_type::DOUBLE:
                hash = get(value.as_double(), sizeof(double));
                break;
            case types::physical_type::STRING:
                hash = get(value.as_string(), value.as_string().size());
                break;
            default:
                throw std::runtime_error("unknown value type");
        }

        return get_node_by_hash(hash);
    }

    void hash_ring::add_node_(size_t node_id, std::mt19937_64& rng) {
        static std::uniform_int_distribution<token_t> dist(std::numeric_limits<token_t>::min(),
                                                           std::numeric_limits<token_t>::max());
        uint64_t remaining = CLUSTER_VNODES;

        if (token_to_node.size() + CLUSTER_VNODES > std::numeric_limits<token_t>::max()) {
            throw std::runtime_error("Token space exhausted, consider changing CLUSTER_VNODES to a smaller number");
        }

        while (remaining) {
            if (token_t tkn = dist(rng); token_to_node.find(tkn) == token_to_node.end()) {
                token_to_node[tkn] = node_id;
                remaining--;
            }
        }
    }
} // namespace consistent_hashing
