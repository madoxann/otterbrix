#pragma once

#include "iceberg_types.hpp"

namespace catalog {
    struct schema {
        std::optional<iceberg::nested_field> find_field(iceberg::field_id id) const;
        std::optional<iceberg::nested_field> find_field(const std::string& name) const;
        std::vector<iceberg::nested_field> columns() const;
        iceberg::field_id highest_field_id() const;

        void add_column(const std::string& name,
                        const iceberg::iceberg_t& type,
                        bool required = false,
                        const std::string& doc = "");
        void add_column(const std::string& parent,
                        const std::string& name,
                        const iceberg::iceberg_t& type,
                        bool required = false,
                        const std::string& doc = "");
        void delete_column(const std::string& name);
        void rename_column(const std::string& name, const std::string& new_name);
        void update_column(const std::string& name, const iceberg::iceberg_t& new_type);
        void update_column_doc(const std::string& name, const std::string& doc);
        void make_optional(const std::string& name);
        void make_required(const std::string& name);

        nlohmann::json to_json() const;
        static schema from_json(const nlohmann::json& j);

    private:
        iceberg::schema_id schema_id;
        iceberg::struct_t schema_struct;
        std::vector<iceberg::field_id> primary_key_field_ids;
    };
} // namespace catalog
