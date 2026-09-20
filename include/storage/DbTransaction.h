#pragma once

#include "storage/DbError.h"
#include "storage/SqliteDb.h"

#include <expected>

namespace nubilo {

/**
 * @class DbTransaction
 * @brief RAII wrapper around a SQL transaction.
 *
 * Begins a transaction on construction. If commit() is never called before the object goes out of scope,
 * the destructor rolls the transaction back, so any early return on error automatically undoes partial changes.
 */
class DbTransaction {
public:
    /**
     * @brief Begins a transaction on the given database.
     * @param db The database to run the transaction on.
     * @return The transaction, DbError if BEGIN fails.
     */
    static std::expected<DbTransaction, DbError> begin(SqliteDb& db);

    ~DbTransaction();

    DbTransaction(const DbTransaction&) = delete;
    DbTransaction& operator=(const DbTransaction&) = delete;

    DbTransaction(DbTransaction&& other) noexcept;
    DbTransaction& operator=(DbTransaction&& other) noexcept;

    /**
     * @brief Commits the transactions. After this, the destructor does nothing.
     * @return Nothing on success, DbError on failure.
     */
    std::expected<void, DbError> commit();

private:
    explicit DbTransaction(SqliteDb& db);

    SqliteDb* db_;
    bool active_ = true;
};

}