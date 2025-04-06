#include "distributed_plan/distributed_planner.hpp"
#include "distributed_plan/meta/analyze.hpp"

using namespace components;

namespace {
    // may be optimized to targeted requests, if condition includes sharing key (however, selection of sharding keys must be discussed)
    void common_distribute(logical_plan::node_ptr node,
                           distributed_plan::distributed_plan_t& plan,
                           meta::catalog& meta,
                           bool ignore_unpopulated = true,
                           const std::optional<consistent_hashing::cluster_graph>& graph = {}) {
        if (auto coll_info = meta.try_get_collection_info(node->database_name(), node->collection_name());
            coll_info.has_value()) {
            auto& coll = coll_info->get();
            auto serial_node = node->to_string();

            if (!coll.distrib_info) {
                // node, containing collection receives plan
                plan[coll.primary_node].emplace_back(std::move(serial_node));
                return;
            }

            // collection is distributed
            if (ignore_unpopulated) {
                // each populated node receives plan
                coll.distrib_info->distribution.for_each_populated(
                    [&](const auto& id) { plan[id].emplace_back(serial_node); });
                return;
            }

            if (graph) {
                // send plan to all neighbouring nodes
                for (const auto& id : graph->graph().at(coll.primary_node)) {
                    plan[id].emplace_back(serial_node);
                }
                return;
            }

            // graph is full, send plan to all nodes
            for (const auto& [id, _] : meta.physical_nodes) {
                plan[id].emplace_back(serial_node);
            }
            return;
        }

        throw std::runtime_error("Collection is missing in catalog - planning is impossible");
    }
} // namespace

namespace distributed_plan {
    void distributed_planner::plan_aggregate(logical_plan::node_ptr node, distributed_plan_t& plan) {
        common_distribute(node, plan, meta);

        // if select has groupBy/orderBy, these operations must be performed LOCALLY
        // TODO: add this logic
    }

    void distributed_planner::plan_update(logical_plan::node_ptr node, distributed_plan_t& plan) {
        common_distribute(node, plan, meta);
    }

    void distributed_planner::plan_delete(logical_plan::node_ptr node, distributed_plan_t& plan) {
        common_distribute(node, plan, meta);
        // todo async statistics update!
    }

    void distributed_planner::plan_create_index(logical_plan::node_ptr node, distributed_plan_t& plan) {
        auto node_data = dynamic_cast<logical_plan::node_create_index_t&>(*node);
        meta::analyze_create_index(node_data, meta);
        common_distribute(node, plan, meta, false, graph);
    }

    void distributed_planner::plan_drop_index(logical_plan::node_ptr node, distributed_plan_t& plan) {
        auto node_data = dynamic_cast<logical_plan::node_drop_index_t&>(*node);
        meta::analyze_drop_index(node_data, meta);
        common_distribute(node, plan, meta, false, graph);
    }

} // namespace distributed_plan
