#pragma once

#include "catalog.hpp"
#include "logical_plan/node_create_collection.hpp"
#include "logical_plan/node_create_database.hpp"
#include "logical_plan/node_create_index.hpp"
#include "logical_plan/node_delete.hpp"
#include "logical_plan/node_drop_collection.hpp"
#include "logical_plan/node_drop_database.hpp"
#include "logical_plan/node_drop_index.hpp"
#include "logical_plan/node_insert.hpp"
#include "logical_plan/node_update.hpp"

database_info& analyze_create_database(const components::logical_plan::node_create_database_t& node, catalog& stat);
void analyze_drop_database(const components::logical_plan::node_drop_database_t& node, catalog& stat);

collection_info& analyze_create_collection(const components::logical_plan::node_create_collection_t& node,
                                           catalog& stat);
void analyze_drop_collection(const components::logical_plan::node_drop_collection_t& node, catalog& stat);

void analyze_create_index(components::logical_plan::node_create_index_t& node, catalog& stat);
void analyze_drop_index(const components::logical_plan::node_drop_index_t& node, catalog& stat);

std::pair<std::optional<std::vector<consistent_hashing::node_id_t>>, bool>
analyze_insert(const components::logical_plan::node_insert_t& node, catalog& stat);

void analyze_delete(const components::logical_plan::node_delete_t& node, catalog& stat);