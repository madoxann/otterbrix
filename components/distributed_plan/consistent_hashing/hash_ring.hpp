#pragma once

#include "physical_node.hpp"
#include <document/value.hpp>
#include <map>

namespace consistent_hashing {
    class hash_ring {
    public:
        hash_ring(const std::vector<physical_node>& nodes, std::mt19937_64& rng)
            : token_to_node() {
            for (const auto& n : nodes) {
                add_node_(n.id(), rng);
            }
        }

        [[nodiscard]] node_id_t get_node_by_hash(token_t hash) const;

        [[nodiscard]] node_id_t get_node_by_key(components::document::value_t value) const;

    private:
        void add_node_(size_t node_id, std::mt19937_64& rng);

        std::map<token_t, node_id_t> token_to_node; // Map from token to node
    };
} // namespace consistent_hashing