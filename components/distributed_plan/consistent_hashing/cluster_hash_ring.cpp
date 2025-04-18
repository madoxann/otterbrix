#include "cluster_hash_ring.hpp"

namespace consistent_hashing {
    cluster_hash_ring::cluster_hash_ring(const std::vector<physical_node>& nodes,
                                         std::mt19937_64& rng,
                                         const consistent_hashing::cluster_graph& graph)
        : rings() {
        for (const auto& node : nodes) {
            rings.emplace(std::make_pair(node.id(), hash_ring()));
        }

        hash_ring full_ring;
        for (const auto& node : nodes) {
            auto tokens = full_ring.add_node_(node.id(), rng);

            // add tokens to all connected rings
            for (auto origin : graph.graph().at(node.id())) {
                for (auto tkn : tokens) {
                    rings.at(origin).token_to_node.emplace(std::make_pair(tkn, node.id()));
                }
            }
        }
    }

    // we have complete rings, simply forward requests to them
    node_id_t cluster_hash_ring::get_node_by_hash(node_id_t node, token_t hash) const {
        return rings.at(node).get_node_by_hash(hash);
    }

    node_id_t cluster_hash_ring::get_node_by_key(node_id_t node, components::document::value_t value) const {
        return rings.at(node).get_node_by_key(value);
    }
} // namespace consistent_hashing
