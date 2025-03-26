#include "distributed_plan/catalog/analyze.hpp"
#include "distributed_plan/distributed_planner.hpp"

using namespace components;

namespace distributed_plan {
    void distributed_planner::plan_create_database(components::logical_plan::node_ptr node,
                                                   distributed_plan::distributed_plan_t& plan) {
        auto node_data = dynamic_cast<logical_plan::node_create_database_t&>(*node);
        auto& db = analyze_create_database(node_data, meta);
        plan[db.primary_node].emplace_back(node);
    }

    void distributed_planner::plan_drop_database(components::logical_plan::node_ptr node,
                                                 distributed_plan::distributed_plan_t& plan) {
        auto node_data = dynamic_cast<logical_plan::node_drop_database_t&>(*node);
        if (auto db_info = meta.try_get_db_info(node_data.database_name()); db_info.has_value()) {
            auto& db = db_info->get();

            plan[db.primary_node].emplace_back(node);
            for (const auto& [name, info] : db.collections) {
                if (!info.distrib_info) {
                    continue;
                }

                // collection was distributed: deletion is required on each affected node
                info.distrib_info->distribution.for_each_populated([&](const auto& id) {
                    plan[id].emplace_back(logical_plan::make_node_drop_collection(node_data.resource(),
                                                                                  {node_data.database_name(), name}));
                });
            }

            analyze_drop_database(node_data, meta); // plan is created, delete db from meta
            return;
        }
        throw std::runtime_error("Collection is missing in catalog - planning is impossible");
    }

} // namespace distributed_plan
