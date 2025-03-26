#include "distributed_plan/distributed_planner.hpp"

using namespace components;

namespace {
    // may be optimized to targeted requests, if condition includes sharing key (however, selection of sharding keys must be discussed)
    void common_distribute(logical_plan::node_ptr node, distributed_plan::distributed_plan_t& plan, catalog& meta) {
        if (auto coll_info = meta.try_get_collection_info(node->database_name(), node->collection_name());
            coll_info.has_value()) {
            auto& coll = coll_info->get();

            if (!coll.distrib_info) {
                // node, containing collection receives plan
                plan[coll.primary_node].emplace_back(node);
                return;
            }

            // collection is distributed, each (?) node receives plan
            coll.distrib_info->distribution.for_each_populated([&](const auto& id) { plan[id].emplace_back(node); });
            return;
        }

        throw std::runtime_error("Collection is missing in catalog - planning is impossible");
    }
} // namespace

namespace distributed_plan {
    void distributed_planner::plan_aggregate(components::logical_plan::node_ptr node,
                                             distributed_plan::distributed_plan_t& plan) {
        common_distribute(node, plan, meta);

        // if select has groupBy/orderBy, these operations must be performed LOCALLY
        // TODO: add this logic
    }

    void distributed_planner::plan_update(components::logical_plan::node_ptr node,
                                          distributed_plan::distributed_plan_t& plan) {
        common_distribute(node, plan, meta);
    }

    void distributed_planner::plan_delete(components::logical_plan::node_ptr node,
                                          distributed_plan::distributed_plan_t& plan) {
        common_distribute(node, plan, meta);
    }
} // namespace distributed_plan
