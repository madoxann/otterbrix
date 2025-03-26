#include "distributed_plan/catalog/analyze.hpp"
#include "distributed_plan/distributed_planner.hpp"
#include "distributed_plan/local_join.hpp"

using namespace components;

namespace distributed_plan {
    namespace {
        plan_id_t build_aggregate_plan(std::pmr::memory_resource* resource,
                                       collection_full_name_t name,
                                       const collection_info& coll,
                                       distributed_plan_t& plan) {
            if (!coll.distrib_info) {
                plan_node_id_t agg = distributed_planner::get_next_plan_id(coll.primary_node, plan);
                plan[coll.primary_node].emplace_back(logical_plan::make_node_aggregate(resource, name));
                return plan_id_t{agg};
            }

            plan_id_t plan_id{};
            coll.distrib_info->distribution.for_each_populated([&](const auto& id) {
                plan_id.push_back(distributed_planner::get_next_plan_id(id, plan));
                plan[id].emplace_back(logical_plan::make_node_aggregate(resource, name));
            });

            return plan_id;
        }

        std::optional<std::pair<logical_plan::node_join_ptr, consistent_hashing::node_id_t>>
        join_dfs(const components::logical_plan::node_join_t& node,
                 distributed_plan_t& plan,
                 std::optional<local_join_node>& local_join,
                 catalog& meta) {
            if (node.children().front()->type() == logical_plan::node_type::join_t) {
                // recursion
                // check structure
                assert(node.children().size() == 2);
                assert(node.children().back()->type() == logical_plan::node_type::aggregate_t);

                auto& right = dynamic_cast<const logical_plan::node_aggregate_t&>(*node.children().back());
                if (auto coll_info_r = meta.try_get_collection_info(right.database_name(), right.collection_name());
                    coll_info_r.has_value()) {
                    auto& coll_r = coll_info_r->get();

                    auto res = join_dfs(dynamic_cast<const logical_plan::node_join_t&>(*node.children().front()),
                                        plan,
                                        local_join,
                                        meta);

                    // aggregate right node
                    plan_id_t&& right_agg =
                        build_aggregate_plan(node.resource(), right.collection_full_name(), coll_r, plan);

                    if (res.has_value()) {
                        // join was local, try to extend
                        auto prev_join = res->first.get();
                        if (coll_r.primary_node == res->second) {
                            // join may be extended, same primary node!
                            auto join =
                                logical_plan::make_node_join(node.resource(), collection_full_name_t(), node.type());
                            join->append_child(prev_join);
                            join->append_child(
                                logical_plan::make_node_aggregate(node.resource(), right.collection_full_name()));
                            join->append_expressions(node.expressions());
                            return std::make_pair(join, res->second);
                        }

                        // extension failed, save node-local subtree
                        plan_node_id_t&& saved_join = distributed_planner::get_next_plan_id(res->second, plan);
                        plan[res->second].emplace_back(prev_join);

                        assert(!local_join); // leaf did not create local_join, because extension failed
                        local_join.emplace(local_join_node(plan_id_t{std::move(saved_join)},
                                                           std::move(right_agg),
                                                           node.type(),
                                                           node.expressions().front()));
                        return {};
                    }

                    // join could not be performed locally, continue building local tree
                    assert(local_join); // tree MUST have been created before
                    auto&& temp = std::move(*local_join);
                    local_join.emplace(local_join_node(std::move(temp),
                                                       std::move(right_agg),
                                                       node.type(),
                                                       node.expressions().front()));
                    return {};
                }

                throw std::runtime_error("Collection is missing in catalog - planning is impossible");
            }

            // leaf
            // check structure
            assert(node.children().size() == 2);
            assert(node.children().front()->type() == logical_plan::node_type::aggregate_t);
            assert(node.children().back()->type() == logical_plan::node_type::aggregate_t);

            auto& left = dynamic_cast<const logical_plan::node_aggregate_t&>(*node.children().front());
            auto& right = dynamic_cast<const logical_plan::node_aggregate_t&>(*node.children().back());
            if (auto coll_info_l = meta.try_get_collection_info(left.database_name(), left.collection_name());
                coll_info_l.has_value()) {
                if (auto coll_info_r = meta.try_get_collection_info(right.database_name(), right.collection_name());
                    coll_info_r.has_value()) {
                    auto& coll_l = coll_info_l->get();
                    auto& coll_r = coll_info_r->get();

                    if (!coll_l.distrib_info && !coll_r.distrib_info && coll_l.primary_node == coll_r.primary_node) {
                        // collections are not distributed and belong to same primary node => join on primary node
                        auto join =
                            logical_plan::make_node_join(node.resource(), collection_full_name_t(), node.type());
                        join->append_child(
                            logical_plan::make_node_aggregate(node.resource(), left.collection_full_name()));
                        join->append_child(
                            logical_plan::make_node_aggregate(node.resource(), right.collection_full_name()));
                        join->append_expressions(node.expressions());
                        return std::make_pair(join, coll_l.primary_node);
                    }

                    // join must be performed locally
                    plan_id_t&& right_agg =
                        build_aggregate_plan(node.resource(), right.collection_full_name(), coll_r, plan);
                    plan_id_t&& left_agg =
                        build_aggregate_plan(node.resource(), left.collection_full_name(), coll_l, plan);

                    assert(!local_join); // we are a leaf
                    local_join.emplace(std::move(left_agg),
                                       std::move(right_agg),
                                       node.type(),
                                       node.expressions().front());
                    return {};
                }
            }

            throw std::runtime_error("Collection is missing in catalog - planning is impossible");
        }
    } // namespace

    void distributed_planner::plan_join(components::logical_plan::node_ptr node,
                                        distributed_plan::distributed_plan_t& plan) {
        auto node_data = dynamic_cast<logical_plan::node_join_t&>(*node);

        std::optional<local_join_node> local_join;
        auto node_local = join_dfs(node_data, plan, local_join, meta);

        if (node_local.has_value()) {
            // congrats, we can execute everything on a single node
            plan[node_local->second].emplace_back(node_local->first);
            return;
        }

        // otherwise, join_dfs filled plan with needed aggregates, now we need to execute them
        assert(local_join); // local plan must exist at this point
        // TODO: store local_join somewhere for later execution
    }
} // namespace distributed_plan
