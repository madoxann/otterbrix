#include "distributed_planner.hpp"

using namespace components;

namespace distributed_plan {
    distributed_plan_t distributed_planner::create_plan(const char* str) {
        auto resource = std::pmr::synchronized_pool_resource();
        sql::transform::transformer transformer(&resource);

        auto lst = raw_parser(str);
        distributed_plan_t plan;
        plan.reserve(meta.physical_nodes.size());

        for (auto query : lst->lst) {
            logical_plan::parameter_node_t agg(&resource);
            auto node = transformer.transform(sql::transform::pg_cell_to_node_cast(query.data), &agg);

            switch (node->type()) {
                case logical_plan::node_type::create_database_t:
                    plan_create_database(node, plan);
                    break;
                case logical_plan::node_type::create_collection_t:
                    plan_create_collection(node, plan);
                    break;
                case logical_plan::node_type::create_index_t:
                    //                    plan_create_index(dynamic_cast<logical_plan::node_create_index_t&>(*node), plan);
                    break;
                case logical_plan::node_type::drop_database_t:
                    plan_drop_database(node, plan);
                    break;
                case logical_plan::node_type::drop_collection_t:
                    plan_drop_collection(node, plan);
                    break;
                case logical_plan::node_type::drop_index_t:
                    //                    plan_drop_index(dynamic_cast<logical_plan::node_drop_index_t&>(*node), plan);
                    break;
                case logical_plan::node_type::insert_t:
                    plan_insert(node, plan);
                    break;
                case logical_plan::node_type::delete_t:
                    plan_delete(node, plan);
                    break;
                case logical_plan::node_type::aggregate_t:
                    plan_aggregate(node, plan);
                    break;
                case logical_plan::node_type::join_t:
                    plan_join(node, plan);
                    break;
                case logical_plan::node_type::update_t:
                    plan_update(node, plan);
                    break;
                default:
                    throw std::runtime_error("unsupported node type: " + to_string(node->type()));
            }
        }

        return plan;
    }
} // namespace distributed_plan