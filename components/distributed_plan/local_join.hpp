#pragma once

#include <utility>

#include "distributed_plan/consistent_hashing/physical_node.hpp"
#include "logical_plan/node.hpp"
#include "logical_plan/node_join.hpp"
#include <boost/container/static_vector.hpp>

namespace distributed_plan {
    /* each plan in distributed plan can be described by pair:
     * first - node_id
     * second - position in vector<plan> to this node */
    using plan_node_id_t = std::pair<consistent_hashing::node_id_t, size_t>;

    template<typename T>
    using cluster_vector = boost::container::static_vector<T, CLUSTER_MAX_NODES>;

    /* however, plan may be distributed across nodes, thus vector<plan_node_id_t>
     * as this will be used for joins, plan will contain aggregate nodes for collections
     * and collection may not be distributed to more than CLUSTER_MAX_NODES nodes */
    using plan_id_t = cluster_vector<plan_node_id_t>;

    // simple tree, representing join operation
    struct local_join_node {
        local_join_node(plan_id_t&& l,
                        plan_id_t&& r,
                        components::logical_plan::join_type t,
                        components::logical_plan::expression_ptr e)
            : left(std::move(l))
            , right(std::move(r))
            , type(t)
            , expression(std::move(e)) {}

        local_join_node(local_join_node&& l,
                        plan_id_t&& r,
                        components::logical_plan::join_type t,
                        components::logical_plan::expression_ptr e)
            : left(std::make_unique<local_join_node>(std::move(l)))
            , right(std::move(r))
            , type(t)
            , expression(std::move(e)) {}

        std::variant<plan_id_t, std::unique_ptr<local_join_node>> left;
        plan_id_t right;
        components::logical_plan::join_type type;
        components::logical_plan::expression_ptr expression;
    };
} // namespace distributed_plan