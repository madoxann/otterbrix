#pragma once

#include "collection_info.hpp"
#include "database_info.hpp"
#include "distributed_collection_info.hpp"
#include "distributed_plan/consistent_hashing/cluster_hash_ring.hpp"
#include "distributed_plan/consistent_hashing/physical_node.hpp"
#include "logical_plan/node_create_index.hpp"
#include <optional>
#include <vector>

namespace meta {
    struct catalog {
        // consider cluster to be a full graph
        catalog(std::vector<consistent_hashing::physical_node>&& nodes, std::mt19937_64& rng);

        // custom cluster graph
        catalog(std::vector<consistent_hashing::physical_node>&& nodes,
                std::mt19937_64& rng,
                const consistent_hashing::cluster_graph& graph);

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

        std::optional<std::reference_wrapper<database_info>> try_get_db_info(const database_name_t& name);

        std::optional<std::reference_wrapper<collection_info>>
        try_get_collection_info(const database_name_t& db_name, const collection_name_t& coll_name);

        bool is_full() const { return std::holds_alternative<consistent_hashing::hash_ring>(hash_ring); }

        const consistent_hashing::hash_ring& get_hash_ring() const {
            assert(is_full());
            return std::get<consistent_hashing::hash_ring>(hash_ring);
        }

        const consistent_hashing::cluster_hash_ring& get_cluster_hash_ring() const {
            assert(!is_full());
            return std::get<consistent_hashing::cluster_hash_ring>(hash_ring);
        }

        bool make_sharded(const database_name_t& db_name,
                          const collection_name_t& coll_name,
                          const std::string& shard_key,
                          shard_distribution&& distribution);

        node_load_t node_record_loads;
        node_load_t node_db_loads;
        std::variant<consistent_hashing::hash_ring, consistent_hashing::cluster_hash_ring> hash_ring;
        std::unordered_map<consistent_hashing::node_id_t, consistent_hashing::physical_node> physical_nodes;
        std::unordered_map<database_name_t, database_info> databases;
    };
} // namespace meta