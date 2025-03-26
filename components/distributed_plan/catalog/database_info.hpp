#pragma once

#include "collection_info.hpp"

struct database_info {
    database_info(consistent_hashing::node_id_t primary_node, std::reference_wrapper<node_load_t> record_cnt)
        : primary_node(primary_node)
        , node_record_loads(record_cnt)
        , collections() {}

    std::optional<std::reference_wrapper<collection_info>> try_get_collection_info(const collection_name_t& name) {
        if (auto coll = collections.find(name); coll != collections.end()) {
            return coll->second;
        }
        return {};
    }

    decltype(auto) create_collection(const collection_name_t& name) {
        return collections.insert({name, collection_info(primary_node, node_record_loads)});
    }

    decltype(auto) erase_collection(const collection_name_t& name) {
        if (auto coll_info = try_get_collection_info(name); coll_info.has_value()) {
            auto& coll = coll_info->get();

            if (coll.distrib_info) {
                for (const auto& [node_id, load] : coll.distrib_info->distribution.records_per_node) {
                    node_record_loads.get()[node_id] -= load;
                }
            } else {
                node_record_loads.get()[primary_node] -= coll.records;
            }
        }

        return collections.erase(name);
    }

    consistent_hashing::node_id_t primary_node;
    std::reference_wrapper<node_load_t> node_record_loads;
    std::unordered_map<collection_name_t, collection_info> collections;
};
