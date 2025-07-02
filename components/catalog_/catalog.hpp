#pragma once

#include "table_metadata.hpp"

#include <mutex>
#include <unordered_set>

namespace catalog {
    class transaction_scope;
    class metadata_transaction;

    class catalog : public std::enable_shared_from_this<catalog> {
    public:
        // transactions need to know when catalog dies
        static std::shared_ptr<catalog> create(std::pmr::memory_resource* resource);

        // namespace operations
        std::pmr::vector<table_namespace_t> list_namespaces() const;
        std::pmr::vector<table_namespace_t> list_namespaces(const table_namespace_t& parent) const;
        bool namespace_exists(const table_namespace_t& namespace_name) const;
        void create_namespace(const table_namespace_t& namespace_name);
        void drop_namespace(const table_namespace_t& namespace_name);

        // table operations
        std::pmr::vector<table_id> list_tables(const table_namespace_t& namespace_name) const;
        schema get_table_schema(const table_id& identifier) const;

        void create_table(const table_id& identifier, const schema& schema);
        void drop_table(const table_id& identifier);
        void rename_table(const table_id& from, const table_id& to);
        bool table_exists(const table_id& identifier) const;

        // schema transactions
        transaction_scope begin_transaction(const table_id& id);

        bool has_active_transactions(const table_id& id);

        void commit_all();
        void abort_all();

    private:
        catalog(std::pmr::memory_resource* resource); // only with factory

        void end_transaction(const table_id& id); // called in ~transaction_scope()

        std::pmr::unordered_set<table_id> active_transactions;
        std::pmr::map<table_id, table_metadata> meta;
        std::pmr::memory_resource* resource;

        mutable std::mutex transaction_mutex;
        mutable std::mutex meta_mutex;

        friend class transaction_scope;
    };

} // namespace catalog
