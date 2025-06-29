#pragma once

#include "table_metadata.hpp"

namespace catalog {
    class transaction_scope;

    class metadata_transaction {
    public:
        enum class State
        {
            ACTIVE,
            COMMITED,
            ABORTED
        };

        State state() const;

        metadata_transaction& update_schema(const schema& new_schema);
        metadata_transaction& update_uuid(table_uuid new_uuid);

        metadata_transaction& add_column(const std::string& name,
                                         const iceberg::iceberg_t& type,
                                         bool required = false,
                                         const std::string& doc = "");
        metadata_transaction& add_column(const std::string& parent,
                                         const std::string& name,
                                         const iceberg::iceberg_t& type,
                                         bool required = false,
                                         const std::string& doc = "");
        metadata_transaction& delete_column(const std::string& name);
        metadata_transaction& rename_column(const std::string& name, const std::string& new_name);
        metadata_transaction& update_column(const std::string& name, const iceberg::iceberg_t& new_type);
        metadata_transaction& update_column_doc(const std::string& name, const std::string& doc);
        metadata_transaction& make_optional(const std::string& name);
        metadata_transaction& make_required(const std::string& name);

        metadata_transaction& savepoint(const std::string& name);
        metadata_transaction& rollback_to_savepoint(const std::string& name);

    private:
        metadata_transaction(std::reference_wrapper<table_metadata> meta_ref); // transactions encapsulates state

        void enusureActive();
        void commit();
        void abort();

        State state_ = State::ACTIVE;
        std::reference_wrapper<table_metadata> meta_ref; // writes by reference
        table_metadata meta;                             // mutates during transaction, copies state in constructor
        std::map<std::string, table_metadata> savepoints;

        friend class transaction_scope;
    };

} // namespace catalog