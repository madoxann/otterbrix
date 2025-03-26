#include "distributed_plan/catalog/analyze.hpp"
#include "distributed_plan/distributed_planner.hpp"

using namespace components;

namespace distributed_plan {
    void distributed_planner::plan_create_collection(components::logical_plan::node_ptr node,
                                                     distributed_plan::distributed_plan_t& plan) {
        auto node_data = dynamic_cast<logical_plan::node_create_collection_t&>(*node);
        auto& coll = analyze_create_collection(node_data, meta);
        plan[coll.primary_node].emplace_back(node);
    }

    void distributed_planner::plan_drop_collection(components::logical_plan::node_ptr node,
                                                   distributed_plan::distributed_plan_t& plan) {
        auto node_data = dynamic_cast<logical_plan::node_drop_collection_t&>(*node);
        if (auto coll_info = meta.try_get_collection_info(node_data.database_name(), node_data.collection_name());
            coll_info.has_value()) {
            auto& coll = coll_info->get();

            if (!coll.distrib_info) {
                plan[coll.primary_node].emplace_back(node);
            } else {
                coll.distrib_info->distribution.for_each_populated(
                    [&](const auto& id) { plan[id].emplace_back(node); });
            }

            analyze_drop_collection(node_data, meta);
            return;
        }

        throw std::runtime_error("Collection is missing in catalog - planning is impossible");
    }
} // namespace distributed_plan