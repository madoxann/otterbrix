#pragma once

#include "iceberg_types.hpp"

namespace catalog {
    struct schema {
        std::optional<iceberg::nested_field> find_field(iceberg::field_id id) const;
        std::optional<iceberg::nested_field> find_field(const std::string& name) const;
        std::vector<iceberg::nested_field> columns() const;
        iceberg::field_id highest_field_id() const;

        nlohmann::json to_json() const;
        static schema from_json(const nlohmann::json& j);

    private:
        iceberg::schema_id schema_id;
        iceberg::struct_t schema_struct;
        std::vector<iceberg::field_id> primary_key_field_ids;
    };
} // namespace catalog
