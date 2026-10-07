#pragma once

#include "storage/DbError.h"

#include <expected>
#include <mutex>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <string>

namespace nubilo {

/**
 * @class Db
 * @brief RAII wrapper around a SQLite connection.
 *
 * Owns the underlying sqlite3 handle: opens it on construction, closes it on destruction.
 * Provides a minimal interface for executing raw SQL. Higher-level query building belongs
 * in the code that uses this class, not here.
 */
class Db {
public:
    /**
     * @brief Opens (or creates, if missing) the SQLite database at the given path.
     * @param path Filesystem path to the .db file
     * @return The opened connection, or a DbError if it fails.
     */
    static std::expected<Db, DbError> open(const std::string& path);

    /**
     * @brief Closes the underlying connection.
     */
    ~Db();

    // Non-copyable: the connection shouldn't be duplicated
    Db(const Db&) = delete;
    Db& operator=(const Db&) = delete;

    Db(Db&& other) noexcept;
    Db& operator=(Db&& other) noexcept;

    /**
     * @brief Locks the database for the calling thread until the returned lock goes out of scope.
     *
     * exec(), query() and lastInsertId() already lock internally. Hold this lock around a sequence of
     * calls that must run as one unit (e.g. an INSERT followed by lastInsertId(), or a SELECT followed
     * by an INSERT that depends on it). The mutex is recursive, meaning those calls can be made while the lock is held.
     *
     * @return The held lock. Keep it alive for the duration of the sequence.
     */
    [[nodiscard]] std::unique_lock<std::recursive_mutex> lock();

    /**
     * @brief Executes a SQL statement with no expected result rows
     * (CREATE TABLE, INSERT, UPDATE, etc..)
     * @param sql The SQL statement to execute.
     * @param params (optional) SQL statement parameter values
     * @return Nothing on success, or a DbError on failure.
     */
    std::expected<void, DbError> exec(const std::string& sql, const std::vector<std::string>& params = {});

    /**
     * @brief Runs a SELECT query and returns the result set as JSON.
     * One array element per row, each row a JSON object mapping column name to value.
     * @param sql The SELECT statement to run.
     * @return  The result rows as a JSON array, or a DbError on failure.
     */
    [[nodiscard]] std::expected<nlohmann::json, DbError> query(const std::string& sql, const std::vector<std::string>& params = {});

    /**
     * Returns the row id of the most recent successful INSERT on this connection.
     * @return The row id, or 0 if no INSERT has happened yet.
     */
    [[nodiscard]] int64_t lastInsertId() const;

    /**
     * @brief Exposes the raw sqlite3 handle, for code that needs lower-level access
     * (e.g. prepared statements for SELECT queries).
     * @returns The underlying sqlite handle
     */
    [[nodiscard]] sqlite3* handle() const;

    // --------- TRANSACTIONS -------------

    /**
     * @brief Begins a SQL transaction.
     * @return Nothing on success, DbError on failure.
     */
    std::expected<void, DbError> beginTx();

    /**
     * @brief Commits the current transaction.
     * @return Nothing on success, DbError on failure.
     */
    std::expected<void, DbError> commitTx();

    /**
     * @brief Rolls back the current transaction, undoing any changes made since beginTransaction().
     * @return
     */
    std::expected<void, DbError> rollbackTx();

private:
    explicit Db(sqlite3* db);
    sqlite3* db_ = nullptr;
    mutable std::recursive_mutex mutex_;
};

}