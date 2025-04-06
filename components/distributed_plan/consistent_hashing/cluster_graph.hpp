#pragma once

#include "physical_node.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace consistent_hashing {
    using graph_t = std::unordered_map<node_id_t, std::unordered_set<node_id_t>>;
    class cluster_graph_builder;

    class cluster_graph {
    private:
        cluster_graph()
            : graph_() {}

    public:
        const graph_t& graph() const { return graph_; }

    private:
        friend class cluster_graph_builder;

        graph_t graph_;
    };

    class cluster_graph_builder {
    public:
        cluster_graph_builder() = default;

        // let's call fully-connected sub-graph on a set of nodes a "family"
        cluster_graph_builder&& add_family(const std::unordered_set<node_id_t>& family) && {
            for (auto n : family) {
                // loops makes sense, each node can be migrated to itself
                clstr.graph_[n].insert(family.begin(), family.end());
            }

            return std::move(*this);
        }

        // undirected
        cluster_graph_builder&& add_edge(node_id_t id1, node_id_t id2) && {
            clstr.graph_[id1].emplace(id2);
            clstr.graph_[id2].emplace(id1);

            return std::move(*this);
        }

        cluster_graph&& finish() && {
            for (auto& [id, nodes] : clstr.graph_) {
                if (nodes.find(id) == nodes.end()) {
                    nodes.emplace(id);
                }
            }

            return std::move(clstr);
        }

    private:
        cluster_graph clstr;
    };
} // namespace consistent_hashing