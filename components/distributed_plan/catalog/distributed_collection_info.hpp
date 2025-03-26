#pragma once

#include "distributed_plan/consistent_hashing/hash_ring.hpp"
#include <functional>

// describes redistribution of existing collection's records across cluster
// primary node, that received partitioning request, MUST submit this data after successful partitioning
struct shard_distribution {
    std::unordered_map<consistent_hashing::node_id_t, uint64_t> records_per_node{};

    void for_each_populated(std::function<void(consistent_hashing::node_id_t)> fun) const {
        for (const auto& [id, records] : records_per_node) {
            if (records) {
                fun(id);
            }
        }
    }

    [[nodiscard]] uint64_t total_records() const {
        uint64_t total = 0;
        for (const auto& [_, count] : records_per_node) {
            total += count;
        }
        return total;
    }
};

// additional data for distributed collection
struct distributed_collection_info {
    std::string shard_key;
    const consistent_hashing::hash_ring& ring;
    shard_distribution distribution;
};
