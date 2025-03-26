#pragma once

#include "distributed_plan/catalog/catalog.hpp"
#include "distributed_plan/local_join.hpp"
#include "logical_plan/node_aggregate.hpp" // node_join?
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
    using distributed_plan_t =
        std::unordered_map<consistent_hashing::node_id_t, std::vector<components::logical_plan::node_ptr>>;

    class distributed_planner {
    public:
        distributed_planner(std::vector<consistent_hashing::physical_node>&& nodes, std::mt19937_64& rng)
            : meta(std::move(nodes), rng) {}

        distributed_plan_t create_plan(const char* str);

        static plan_node_id_t get_next_plan_id(consistent_hashing::node_id_t node, const distributed_plan_t& plan) {
            return std::make_pair(node, plan.at(node).size());
        }

    private:
        void plan_create_database(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_create_collection(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        // TODO: how to distribute indexes?
        //        void plan_create_index(components::logical_plan::node_create_index_t& node,
        //                               std::vector<distributed_plan_t>& plan);

        void plan_drop_database(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_drop_collection(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        //        void plan_drop_index(const components::logical_plan::node_drop_index_t& node,
        //                             std::vector<distributed_plan_t>& plan);

        void plan_insert(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_aggregate(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_join(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_update(components::logical_plan::node_ptr node, distributed_plan_t& plan);
        void plan_delete(components::logical_plan::node_ptr node, distributed_plan_t& plan);

        catalog meta;
    };
} // namespace distributed_plan