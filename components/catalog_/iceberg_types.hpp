#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <components/types/types.hpp>

namespace catalog::iceberg {
    using timestamp = std::chrono::milliseconds;
    using schema_id = int32_t;
    using field_id = int32_t;

    // complex iceberg types
    struct list_t;
    struct map_t;
    struct struct_t;

    // no DATE and TIME in enum, other needed types are present
    using primitive = components::types::logical_type;

    using iceberg_t =
        std::variant<primitive, std::unique_ptr<list_t>, std::unique_ptr<map_t>, std::unique_ptr<struct_t>>;

    std::pmr::string primitive_to_json(primitive type, std::pmr::memory_resource* resource);
    primitive primitive_from_json(const std::string& j);

    struct list_t {
        explicit list_t(std::pmr::memory_resource* resource,
                        field_id elem_id,
                        iceberg_t elem_type,
                        bool required = true)
            : resource(resource)
            , element_id(elem_id)
            , element_type(std::move(elem_type))
            , element_required(required) {}

        std::pmr::string to_json() const;
        static list_t from_json(const std::string& j, std::pmr::memory_resource* resource);

        std::pmr::memory_resource* resource;
        field_id element_id;
        iceberg_t element_type;
        bool element_required;
    };

    struct map_t {
        explicit map_t(std::pmr::memory_resource* resource,
                       field_id k_id,
                       iceberg_t k_type,
                       field_id v_id,
                       iceberg_t v_type,
                       bool v_required = true)
            : resource(resource)
            , key_id(k_id)
            , key_type(std::move(k_type))
            , value_id(v_id)
            , value_type(std::move(v_type))
            , value_required(v_required) {}

        std::pmr::string to_json() const;
        static map_t from_json(const std::string& j, std::pmr::memory_resource* resource);

        std::pmr::memory_resource* resource;
        field_id key_id;
        iceberg_t key_type;
        field_id value_id;
        iceberg_t value_type;
        bool value_required;
    };

    struct nested_field {
        explicit nested_field(std::pmr::memory_resource* resource,
                              field_id id,
                              std::pmr::string name,
                              iceberg_t type,
                              bool required = true,
                              std::pmr::string doc = "")
            : resource(resource)
            , id(id)
            , name(std::move(name))
            , type(std::move(type))
            , required(required)
            , doc(std::move(doc)) {}

        std::pmr::string to_json() const;
        static nested_field from_json(const std::string& j, std::pmr::memory_resource* resource);

        std::pmr::memory_resource* resource;
        field_id id;
        std::pmr::string name;
        iceberg_t type;
        bool required;
        std::pmr::string doc;
    };

    struct struct_t {
        struct_t(std::pmr::memory_resource* resource)
            : fields_(resource){};

        struct_t& add_field(nested_field field);

        nested_field field(field_id id) const;
        nested_field field(const std::pmr::string& name) const;

        const std::pmr::vector<nested_field>& fields() const { return fields_; }

        std::pmr::string to_json() const;
        static struct_t from_json(const std::string& j, std::pmr::memory_resource* resource);

    private:
        std::pmr::vector<nested_field> fields_;
    };
} // namespace catalog::iceberg