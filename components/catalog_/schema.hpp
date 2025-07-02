#pragma once

#include "iceberg_types.hpp"

namespace catalog {
    struct schema {
        schema(std::pmr::memory_resource* resource);

        iceberg::nested_field find_field(iceberg::field_id id) const;
        iceberg::nested_field find_field(const std::pmr::string& name) const;
        std::pmr::vector<iceberg::nested_field> columns() const;
        iceberg::field_id highest_field_id() const;

        void add_column(const std::pmr::string& name,
                        const iceberg::iceberg_t& type,
                        bool required = false,
                        const std::pmr::string& doc = "");
        void add_column(const std::pmr::string& parent,
                        const std::pmr::string& name,
                        const iceberg::iceberg_t& type,
                        bool required = false,
                        const std::pmr::string& doc = "");
        void delete_column(const std::pmr::string& name);
        void rename_column(const std::pmr::string& name, const std::pmr::string& new_name);
        void update_column(const std::pmr::string& name, const iceberg::iceberg_t& new_type);
        void update_column_doc(const std::pmr::string& name, const std::pmr::string& doc);
        void make_optional(const std::pmr::string& name);
        void make_required(const std::pmr::string& name);

        std::pmr::string to_json() const;
        static schema from_json(const std::string& j, std::pmr::memory_resource* resource);

    private:
        iceberg::schema_id schema_id;
        iceberg::struct_t schema_struct;
        std::pmr::vector<iceberg::field_id> primary_key_field_ids;
    };
} // namespace catalog
