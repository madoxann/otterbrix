#pragma once

#include "distributed_plan/cluster_configuration.hpp"
#include <cstddef>
#include <random>

namespace consistent_hashing {
    using token_t = uint64_t;
    using node_id_t = size_t;

    class physical_node {
    public:
        explicit physical_node(size_t id)
            : node_id(id) {}

        [[nodiscard]] node_id_t id() const { return node_id; }

    private:
        node_id_t node_id;
        // TODO: include arrow::Location
    };
} // namespace consistent_hashing