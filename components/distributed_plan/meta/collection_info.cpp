#pragma once

#include "collection_info.hpp"

namespace meta {
    // all create document methods return bool, signalling necessity of sharding
    bool collection_info::create_documents_primary(uint64_t num) {
        assert(!distrib_info);

        records += num;
        node_record_loads.get()[primary_node] += num;
        return records >= CLUSTER_SPLIT_THRESHOLD;
    }

    // id's of targeted node + is partitioning needed?
    std::pair<consistent_hashing::node_id_t, bool>
    collection_info::create_document_distributed(components::document::document_ptr doc) {
        auto node =
            distrib_info ? distrib_info->ring.get_node_by_key(doc->get_value(distrib_info->shard_key)) : primary_node;
        records++;

        node_record_loads.get()[node]++;

        if (distrib_info) {
            distrib_info->distribution.records_per_node[node]++;
        }

        return {node, records >= CLUSTER_SPLIT_THRESHOLD};
    }

    void collection_info::erase_documents_unsharded(uint64_t num) {
        assert(!distrib_info);
        assert(num <= records);

        records -= num;
        node_record_loads.get()[primary_node] -= num;
    }

    void collection_info::erase_document(components::document::document_ptr doc) {
        auto node =
            distrib_info ? distrib_info->ring.get_node_by_key(doc->get_value(distrib_info->shard_key)) : primary_node;

        if (records > 0) {
            records--;
        }

        if (node_record_loads.get()[node] > 0) {
            node_record_loads.get()[node]--;
        }

        if (distrib_info && distrib_info->distribution.records_per_node[node] > 0) {
            distrib_info->distribution.records_per_node[node]--;
        }
    }
} // namespace meta
