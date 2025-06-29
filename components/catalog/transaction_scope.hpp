#pragma once

#include <utility>

#include "metadata_transaction.hpp"

namespace catalog {
    class transaction_scope {
    public:
        transaction_scope(std::shared_ptr<metadata_transaction> tx)
            : tx(std::move(tx)) {}

        ~transaction_scope() {
            if (tx && tx->state() == metadata_transaction::State::ACTIVE) {
                tx->abort();
            }
        }

        std::shared_ptr<metadata_transaction> transaction() { return tx; }

        void commit() {
            if (tx && tx->state() == metadata_transaction::State::ACTIVE) {
                tx->commit();
            }
        }

        transaction_scope(const transaction_scope&) = delete;
        transaction_scope& operator=(const transaction_scope&) = delete;

        transaction_scope(transaction_scope&& other) noexcept
            : tx(std::move(other.tx)) {
            other.tx = nullptr;
        }

        transaction_scope& operator=(transaction_scope&& other) noexcept {
            if (this != &other) {
                if (tx && tx->state() == metadata_transaction::State::ACTIVE) {
                    tx->abort();
                }
                tx = std::move(other.tx);
                other.tx = nullptr;
            }
            return *this;
        }

    private:
        std::shared_ptr<metadata_transaction> tx;
    };

    /* example usage:
     * void modify_table_schema(catalog::catalog& cat, const catalog::table_id& table_id) {
     *     catalog::transaction_scope tx_scope(cat.begin_transaction(table_id));
     *     auto& tx = *tx_scope.transaction();
     *
     *     tx.add_column("user_id", iceberg::iceberg_t::primitive_type(iceberg::primitive::INT), true)
     *         .add_column("username", iceberg::iceberg_t::primitive_type(iceberg::primitive::STRING), true)
     *         .add_column("profile", iceberg::iceberg_t::primitive_type(iceberg::primitive::STRING), false)
     *         .savepoint("basic_schema")
     *         .add_column("created_at", iceberg::iceberg_t::primitive_type(iceberg::primitive::TIMESTAMP), true);
     *
     *     // commiting changes
     *     tx_scope.commit();
     *
     *     // aborts, if didn't commit
     * }
     */
} // namespace catalog