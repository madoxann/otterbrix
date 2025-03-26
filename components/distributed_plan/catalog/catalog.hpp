#pragma once

#include "collection_info.hpp"
#include "database_info.hpp"
#include "distributed_collection_info.hpp"
#include "distributed_plan/consistent_hashing/hash_ring.hpp"
#include "distributed_plan/consistent_hashing/physical_node.hpp"
#include "logical_plan/node_create_index.hpp"
#include <optional>
#include <vector>

// TODO: cpp separation
struct catalog {
    catalog(std::vector<consistent_hashing::physical_node>&& nodes, std::mt19937_64& rng)
        : node_record_loads()
        , node_db_loads()
        , hash_ring(nodes, rng)
        , physical_nodes()
        , databases() {
        if (nodes.empty()) {
            throw std::runtime_error("Cluster must include at least one node");
        }

        physical_nodes.reserve(nodes.size());
        for (auto&& node : nodes) {
            auto id = node.id();
            physical_nodes.emplace(id, std::move(node));
        }

        for (const auto& [id, _] : physical_nodes) {
            node_record_loads[id] = 0;
            node_db_loads[id] = 0;
        }
    }

    decltype(auto) create_db(const database_name_t& name) {
        consistent_hashing::node_id_t min_id = node_record_loads.begin()->first;
        uint64_t min_load = node_record_loads.begin()->second;

        for (const auto& [id, load] : node_record_loads) {
            if (load < min_load || (load == min_load && node_db_loads[id] < node_db_loads[min_id])) {
                min_id = id;
                min_load = load;
            }
        }

        node_db_loads[min_id]++;
        return databases.insert({name, database_info(min_id, std::ref(node_record_loads))});
    }

    decltype(auto) erase_db(const database_name_t& name) {
        if (auto db_info = try_get_db_info(name); db_info.has_value()) {
            auto& db = db_info->get();

            node_db_loads[db.primary_node]--;
            auto it = db.collections.begin();
            while (it != db.collections.end()) {
                collection_name_t coll_name = it->first;
                ++it;
                db.erase_collection(coll_name);
            }
        }

        return databases.erase(name);
    }

    std::optional<std::reference_wrapper<database_info>> try_get_db_info(const database_name_t& name) {
        if (auto db = databases.find(name); db != databases.end()) {
            return db->second;
        }
        return {};
    }

    std::optional<std::reference_wrapper<collection_info>> try_get_collection_info(const database_name_t& db_name,
                                                                                   const collection_name_t& coll_name) {
        if (auto db = try_get_db_info(db_name); db.has_value()) {
            return db->get().try_get_collection_info(coll_name);
        }
        return {};
    }

    bool make_sharded(const database_name_t& db_name,
                      const collection_name_t& coll_name,
                      const std::string& shard_key,
                      shard_distribution&& distribution) {
        if (auto coll_info = try_get_collection_info(db_name, coll_name)) {
            auto& coll = coll_info->get();

            // partitioned records == collection size
            // otherwise something is very, very wrong...
            assert(distribution.total_records() == coll.records);

            // distributing same collection twice makes no sense
            assert(!coll.distrib_info);

            node_record_loads[coll.primary_node] -= coll.records;
            for (const auto& [node_id, new_load] : distribution.records_per_node) {
                node_record_loads[node_id] += new_load;
            }

            coll.distrib_info.emplace(distributed_collection_info{shard_key, hash_ring, std::move(distribution)});
            return true;
        }

        return false;
    }

    node_load_t node_record_loads;
    node_load_t node_db_loads;
    consistent_hashing::hash_ring hash_ring;
    std::unordered_map<consistent_hashing::node_id_t, consistent_hashing::physical_node> physical_nodes;
    std::unordered_map<database_name_t, database_info> databases;
};
