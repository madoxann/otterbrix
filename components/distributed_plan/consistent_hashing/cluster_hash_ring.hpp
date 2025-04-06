#pragma once

#include "cluster_graph.hpp"
#include "hash_ring.hpp"

namespace meta {
    struct catalog;
}

namespace consistent_hashing {
    class cluster_hash_ring {
    public:
        cluster_hash_ring(const std::vector<physical_node>& nodes, std::mt19937_64& rng, const cluster_graph& graph);

        [[nodiscard]] node_id_t get_node_by_hash(node_id_t node, token_t hash) const;

        [[nodiscard]] node_id_t get_node_by_key(node_id_t node, components::document::value_t value) const;

    private:
        friend struct meta::catalog;

        std::unordered_map<node_id_t, hash_ring> rings;
    };
} // namespace consistent_hashing