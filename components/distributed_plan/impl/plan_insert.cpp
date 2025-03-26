#include "distributed_plan/catalog/analyze.hpp"
#include "distributed_plan/distributed_planner.hpp"

using namespace components;

namespace distributed_plan {
    void distributed_planner::plan_insert(components::logical_plan::node_ptr node,
                                          distributed_plan::distributed_plan_t& plan) {
        auto node_data = dynamic_cast<logical_plan::node_insert_t&>(*node);

        auto [ids, need_sharding] = analyze_insert(node_data, meta); // TODO: use this bool
        if (ids.has_value()) {
            // collection is distributed, each document goes to its node according to ring
            assert(ids->size() == node_data.documents().size());

            std::unordered_map<consistent_hashing::node_id_t, std::pmr::vector<components::document::document_ptr>>
                node_to_doc;
            for (size_t i = 0; i < ids->size(); ++i) {
                if (auto it = node_to_doc.find(ids->operator[](i)); it != node_to_doc.end()) {
                    it->second.push_back(std::move(node_data.documents()[i]));
                } else {
                    node_to_doc.insert(
                        {ids->operator[](i),
                         std::pmr::vector<components::document::document_ptr>({std::move(node_data.documents()[i])},
                                                                              node_data.resource())});
                }
            }

            // sigsegv, troubles with returning plan for create_plan, something's up with documents destructor and intrusive_ptr
            // serializing will solve this problem!
            for (auto& [id, docs] : node_to_doc) {
//                plan[id].emplace_back(
//                    logical_plan::make_node_insert(node_data.resource(),
//                                                   {node_data.database_name(), node_data.collection_name()},
//                                                   std::move(docs)));
            }
            return;
        }

        // not distributed - everything is inserted into primary shard
        plan[meta.try_get_db_info(node_data.database_name())->get().primary_node].emplace_back(node.detach());
    }
} // namespace distributed_plan
