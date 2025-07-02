#pragma once

#include <string>
#include <vector>

namespace catalog {
    using table_namespace_t = std::pmr::vector<std::pmr::string>;

    class table_uuid {
    public:
        table_uuid(std::pmr::memory_resource* resource);

        static table_uuid generate();
        static table_uuid parse(const std::string& uuid_str, std::pmr::memory_resource* resource);

        std::pmr::string to_string() const { return value; }

    private:
        std::pmr::string value;
    };

    class table_id {
    public:
        table_id(std::pmr::memory_resource* resource);

        static table_id parse(const std::string& identifier_str, std::pmr::memory_resource* resource);

        table_namespace_t get_namespace() const { return namespace_parts; }
        std::pmr::string get_table_name() const { return name; }

        std::pmr::string to_string() const;

    private:
        table_namespace_t namespace_parts;
        std::pmr::string name;
    };

} // namespace catalog