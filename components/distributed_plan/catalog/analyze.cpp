#include "distributed_plan/catalog/analyze.hpp"

using namespace components;

database_info& analyze_create_database(const logical_plan::node_create_database_t& node, catalog& stat) {
    return stat.create_db(node.database_name()).first->second;
}

void analyze_drop_database(const logical_plan::node_drop_database_t& node, catalog& stat) {
    stat.erase_db(node.database_name());
}

collection_info& analyze_create_collection(const logical_plan::node_create_collection_t& node, catalog& stat) {
    if (auto db = stat.try_get_db_info(node.database_name()); db.has_value()) {
        return db->get().create_collection(node.collection_name()).first->second;
    }

    throw std::runtime_error("missing database!");
}

void analyze_drop_collection(const logical_plan::node_drop_collection_t& node, catalog& stat) {
    if (auto db = stat.try_get_db_info(node.database_name()); db.has_value()) {
        db->get().erase_collection(node.collection_name());
        return;
    }

    throw std::runtime_error("missing database!");
}

void analyze_create_index(logical_plan::node_create_index_t& node, catalog& stat) {
    if (auto col = stat.try_get_collection_info(node.database_name(), node.collection_name()); col.has_value()) {
        col->get().indexes.emplace<std::pair<std::string, logical_plan::keys_base_storage_t>>(
            {node.name(), node.keys()});
        return;
    }

    throw std::runtime_error("invalid database or collection");
}

void analyze_drop_index(const logical_plan::node_drop_index_t& node, catalog& stat) {
    if (auto col = stat.try_get_collection_info(node.database_name(), node.collection_name()); col.has_value()) {
        col->get().indexes.erase(node.name());
        return;
    }

    throw std::runtime_error("invalid database or collection");
}

std::pair<std::optional<std::vector<consistent_hashing::node_id_t>>, bool>
analyze_insert(const components::logical_plan::node_insert_t& node, catalog& stat) {
    if (auto coll = stat.try_get_collection_info(node.database_name(), node.collection_name()); coll.has_value()) {
        auto& coll_info = coll->get();

        if (coll_info.distrib_info) {
            std::vector<consistent_hashing::node_id_t> ids;
            ids.reserve(node.documents().size());
            bool sharding_needed;

            for (const auto& d : node.documents()) {
                auto [id, shrd] = coll_info.create_document_distributed(d);
                ids.push_back(id);
                sharding_needed &= shrd;
            }
            return {ids, sharding_needed};
        } else {
            bool sharding_needed = coll_info.create_documents_primary(node.documents().size());
            return {{}, sharding_needed};
        }
    }

    throw std::runtime_error("invalid database or collection");
}

// TODO: delete this? analysis here is useless
void analyze_delete(const components::logical_plan::node_delete_t& node, catalog& stat) {
    if (auto col = stat.try_get_collection_info(node.database_name(), node.collection_name()); col.has_value()) {
        // ?? delete is deferred operation, statistics must be updated afterwards
        return;
    }

    throw std::runtime_error("invalid database or collection");
}
