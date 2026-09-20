#pragma once

#include <sqlite3.h>
#include <string>

namespace nubilo {

/**
 * @class SqliteDb
 * @brief RAII wrapper around a SQLite connection.
 *
 * Owns the underlying sqlite3 handle: opens it on construction, closes it on destruction.
 * Provides a minimal interface for executing raw SQL. Higher-level query building belongs
 * in the code that uses this class, not here.
 */
class SqliteDb {
public:
    /**
     * @brief Opens (or creates, if missing) the SQLite database at the given path.
     * @param path Filesystem path to the .db file
     */
    explicit SqliteDb(const std::string& path);

    /**
     * @brief Closes the underlying connection.
     */
    ~SqliteDb();

    // Non-copyable: the connection shouldn't be duplicated
    SqliteDb(const SqliteDb&) = delete;
    SqliteDb& operator=(const SqliteDb&) = delete;

    /**
     * @brief Executes a SQL statement with no expected result rows
     * (CREATE TABLE, INSERT, UPDATE, etc..)
     * @param sql The SQL statement to execute.
     * @throws std::runtime_error if execution fails.
     */
    void exec(const std::string& sql);

    /**
     * @brief Exposes the raw sqlite3 handle, for code that needs lower-level access
     * (e.g. prepared statements for SELECT queries).
     * @return
     */
    [[nodiscard]] sqlite3* handle() const;

private:
    sqlite3* db_ = nullptr;
};

}