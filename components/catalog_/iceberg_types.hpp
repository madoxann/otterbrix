#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace catalog::iceberg {
    using timestamp = std::chrono::milliseconds;
    using schema_id = int32_t;
    using field_id = int32_t;

    // complex iceberg types
    struct list_t;
    struct map_t;
    struct struct_t;

    // may be replaced with logical_type
    enum class primitive
    {
        BOOLEAN,
        INT,
        LONG,
        FLOAT,
        DOUBLE,
        DECIMAL,
        DATE,
        TIME,
        TIMESTAMP,
        TIMESTAMPTZ,
        STRING,
        UUID,
        FIXED,
        BINARY
    };

    using iceberg_t =
        std::variant<primitive, std::shared_ptr<list_t>, std::shared_ptr<map_t>, std::shared_ptr<struct_t>>;

    struct list_t {
        field_id element_id;
        iceberg_t element_type;
        bool element_required;
    };

    struct map_t {
        field_id key_id;
        iceberg_t key_type;
        field_id value_id;
        iceberg_t value_type;
        bool value_required;
    };

    struct nested_field {
        field_id id;
        std::string name;
        iceberg_t type;
        bool required;
        std::string doc;

        nlohmann::json to_json() const;
        static nested_field from_json(const nlohmann::json& j);
    };

    struct struct_t {
        std::vector<nested_field> fields_;

        std::optional<nested_field> field(field_id id) const;
        std::optional<nested_field> field(const std::string& name) const;

        std::vector<nested_field> fields() const { return fields_; }
    };
} // namespace catalog::iceberg