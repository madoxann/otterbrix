#pragma once

#include "metadata_transaction.hpp"
#include "table_metadata.hpp"

#include <mutex>

namespace catalog {
    class catalog {
    public:
        // namespace operations
        std::vector<table_namespace_t> list_namespaces() const;
        std::vector<table_namespace_t> list_namespaces(const table_namespace_t& parent) const;
        bool namespace_exists(const table_namespace_t& namespace_name) const;
        void create_namespace(const table_namespace_t& namespace_name);
        void drop_namespace(const table_namespace_t& namespace_name);

        // table operations
        std::vector<table_id> list_tables(const table_namespace_t& namespace_name) const;
        schema get_table_schema(const table_id& identifier) const;

        void create_table(const table_id& identifier, const schema& schema);
        void drop_table(const table_id& identifier);
        void rename_table(const table_id& from, const table_id& to);
        bool table_exists(const table_id& identifier) const;

        // schema transactions
        std::shared_ptr<metadata_transaction> begin_transaction(const table_id& id);

        // return ongoing transaction, if it exists
        std::shared_ptr<metadata_transaction> get_or_begin_transaction(const table_id& id);
        bool has_active_transactions(const table_id& id);

        void commit_all();
        void abort_all();

    private:
        std::map<table_id, std::unique_ptr<metadata_transaction>> active_transactions;
        std::map<table_id, table_metadata> meta;

        mutable std::mutex transaction_mutex;
    };

} // namespace catalog
