#pragma once

#include "distributed_plan/local_join.hpp"
#include "distributed_plan/meta/catalog.hpp"
#include "logical_plan/node_aggregate.hpp"
#include "logical_plan/node_create_collection.hpp"
#include "logical_plan/node_create_database.hpp"
#include "logical_plan/node_create_index.hpp"
#include "logical_plan/node_delete.hpp"
#include "logical_plan/node_drop_collection.hpp"
#include "logical_plan/node_drop_database.hpp"
#include "logical_plan/node_drop_index.hpp"
#include "logical_plan/node_insert.hpp"
#include "logical_plan/node_update.hpp"
#include "sql/parser/parser.h"
#include "sql/transformer/transformer.hpp"
#include "sql/transformer/utils.hpp"

namespace distributed_plan {
    // serializes into std::pmr::string
    using distributed_plan_t = std::unordered_map<consistent_hashing::node_id_t, std::vector<std::pmr::string>>;
    using local_plan_t = local_join_node;
    using complete_plan_t = std::pair<distributed_plan_t, std::optional<local_plan_t>>;

    class distributed_planner {
    public:
        distributed_planner(std::vector<consistent_hashing::physical_node>&& nodes, std::mt19937_64& rng)
            : meta(std::move(nodes), rng)
            , graph() {}

        distributed_planner(std::vector<consistent_hashing::physical_node>&& nodes,
                            std::mt19937_64& rng,
                            consistent_hashing::cluster_graph&& graph)
            : meta(std::move(nodes), rng, graph)
            , graph(std::move(graph)) {}

        complete_plan_t create_plan(const char* str);

        static plan_node_id_t get_next_plan_id(consistent_hashing::node_id_t node, const distributed_plan_t& plan) {
            if (auto it = plan.find(node); it != plan.end()) {
                return std::make_pair(node, it->second.size());
            }
            return std::make_pair(node, 0);
        }

        //    private:
        void plan_create_database(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_create_collection(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_create_index(components::logical_plan::node_ptr node, distributed_plan_t& plan);

        void plan_drop_database(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_drop_collection(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_drop_index(components::logical_plan::node_ptr node, distributed_plan_t& plan);

        void plan_insert(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_aggregate(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        std::optional<local_plan_t> plan_join(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_update(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_delete(components::logical_plan::node_ptr node, distributed_plan_t& plan);

        meta::catalog meta;
        std::optional<consistent_hashing::cluster_graph> graph; // missing graph means that graph is full
    };
} // namespace distributed_plan