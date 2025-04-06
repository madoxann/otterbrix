#pragma once

#include "distributed_collection_info.hpp"
#include "distributed_plan/cluster_configuration.hpp"
#include "distributed_plan/consistent_hashing/hash_ring.hpp"
#include "document/document.hpp"
#include "logical_plan/node_create_index.hpp"

namespace meta {
    using node_load_t = std::unordered_map<consistent_hashing::node_id_t, uint64_t>; // TODO: BigInteger?

    struct collection_info {
        explicit collection_info(consistent_hashing::node_id_t primary_node,
                                 std::reference_wrapper<node_load_t> record_cnt)
            : records(0)
            , primary_node(primary_node)
            , node_record_loads(record_cnt)
            , indexes()
            , distrib_info() {}

        // all create document methods return bool, signalling necessity of sharding
        bool create_documents_primary(uint64_t num);

        // id's of targeted node + is partitioning needed?
        std::pair<consistent_hashing::node_id_t, bool>
        create_document_distributed(components::document::document_ptr doc);

        void erase_documents_unsharded(uint64_t num);

        void erase_document(components::document::document_ptr doc);

        uint64_t records; // TODO: BigInteger?
        consistent_hashing::node_id_t primary_node;
        std::unordered_map<std::string, components::logical_plan::keys_base_storage_t> indexes;
        std::reference_wrapper<node_load_t> node_record_loads;
        std::optional<distributed_collection_info> distrib_info;
    };
} // namespace meta
