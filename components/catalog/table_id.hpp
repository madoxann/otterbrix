#pragma once

#include <string>
#include <vector>

namespace catalog {
    using table_namespace_t = std::vector<std::string>;

    struct table_uuid {
        std::string value;

        static table_uuid generate();
        static table_uuid parse(const std::string& uuid_str);

        std::string to_string() const { return value; }
    };

    struct table_id {
        table_namespace_t namespace_parts;
        std::string name;

        std::string to_string() const;
        static table_id parse(const std::string& identifier_str);

        table_namespace_t get_namespace() const { return namespace_parts; }

        std::string get_table_name() const { return name; }
    };

} // namespace catalog