#include "catalog.hpp"

namespace meta {
    namespace {
        void init_physical_nodes(
            std::vector<consistent_hashing::physical_node>&& nodes,
            std::unordered_map<consistent_hashing::node_id_t, consistent_hashing::physical_node>& physical_nodes) {
            if (nodes.empty()) {
                throw std::runtime_error("Cluster must include at least one node");
            }

            physical_nodes.reserve(nodes.size());
            for (auto&& node : nodes) {
                auto id = node.id();
                physical_nodes.emplace(id, std::move(node));
            }
        }

        void init_counters(
            const std::unordered_map<consistent_hashing::node_id_t, consistent_hashing::physical_node>& physical_nodes,
            node_load_t& node_record_loads,
            node_load_t& node_db_loads) {
            for (const auto& [id, _] : physical_nodes) {
                node_record_loads[id] = 0;
                node_db_loads[id] = 0;
            }
        }
    } // namespace

    catalog::catalog(std::vector<consistent_hashing::physical_node>&& nodes, std::mt19937_64& rng)
        : node_record_loads()
        , node_db_loads()
        , hash_ring(consistent_hashing::hash_ring(nodes, rng))
        , physical_nodes()
        , databases() {
        init_physical_nodes(std::move(nodes), physical_nodes);
        init_counters(physical_nodes, node_record_loads, node_db_loads);
    }

    catalog::catalog(std::vector<consistent_hashing::physical_node>&& nodes,
                     std::mt19937_64& rng,
                     const consistent_hashing::cluster_graph& graph)
        : node_record_loads()
        , node_db_loads()
        , hash_ring(consistent_hashing::cluster_hash_ring(nodes, rng, graph))
        , physical_nodes()
        , databases() {
        init_physical_nodes(std::move(nodes), physical_nodes);
        init_counters(physical_nodes, node_record_loads, node_db_loads);
    }

    std::optional<std::reference_wrapper<database_info>> catalog::try_get_db_info(const database_name_t& name) {
        if (auto db = databases.find(name); db != databases.end()) {
            return db->second;
        }
        return {};
    }

    std::optional<std::reference_wrapper<collection_info>>
    catalog::try_get_collection_info(const database_name_t& db_name, const collection_name_t& coll_name) {
        if (auto db = try_get_db_info(db_name); db.has_value()) {
            return db->get().try_get_collection_info(coll_name);
        }
        return {};
    }

    bool catalog::make_sharded(const database_name_t& db_name,
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

            // collection was stored on it's primary node, it was migrated according to cluster_graph
            // use primary node's hash_ring for further distribution
            coll.distrib_info.emplace(distributed_collection_info{
                shard_key,
                is_full() ? get_hash_ring() : get_cluster_hash_ring().rings.at(coll.primary_node),
                std::move(distribution)});
            return true;
        }

        return false;
    }
} // namespace meta
