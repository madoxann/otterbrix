#pragma once

#include <utility>

#include "catalog.hpp"
#include "metadata_transaction.hpp"

namespace catalog {
    class transaction_scope {
    public:
        // if no strong catalog ref - transaction aborts
        transaction_scope(std::weak_ptr<catalog> cat,
                          std::unique_ptr<metadata_transaction> transaction,
                          const table_id& id);

        // unregisters transaction from catalog, aborts if not committed
        ~transaction_scope();

        metadata_transaction& transaction();

        void commit();
        void abort();

        transaction_scope(const transaction_scope&) = delete;
        transaction_scope& operator=(const transaction_scope&) = delete;

        transaction_scope(transaction_scope&& other) noexcept;

        transaction_scope& operator=(transaction_scope&& other) noexcept;

    private:
        bool is_commited;
        bool is_aborted;
        table_id id;
        std::weak_ptr<catalog> catalog;
        std::unique_ptr<metadata_transaction> transaction_; // owns transaction
    };

    /* example usage:
     * void modify_table_schema(catalog::catalog& cat, const catalog::table_id& table_id) {
     *     catalog::transaction_scope tx_scope = cat.begin_transaction(table_id);
     *     auto& tx = tx_scope.transaction();
     *
     *     tx.add_column("user_id", iceberg::primitive::INT), true)
     *         .add_column("username", iceberg::primitive::STRING), true)
     *         .add_column("profile", iceberg::primitive::STRING), false)
     *         .savepoint("basic_schema")
     *         .add_column("created_at", iceberg::primitive::TIMESTAMP), true);
     *
     *     // commiting changes
     *     tx_scope.commit();
     *
     *     // aborts, if didn't commit
     * }
     */
} // namespace catalog