#include "storage/SqliteDb.h"

#include <stdexcept>

namespace nubilo {

SqliteDb::SqliteDb(sqlite3* db) : db_(db) {}

std::expected<SqliteDb, DbError> SqliteDb::open(const std::string& path) {
    sqlite3* db = nullptr;
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        DbError error{sqlite3_errmsg(db)};
        sqlite3_close(db);
        return std::unexpected(error);
    }
    return SqliteDb(db);
}

SqliteDb::~SqliteDb() {
    sqlite3_close(db_);
}

SqliteDb::SqliteDb(SqliteDb&& other) noexcept : db_(other.db_) {
    other.db_ = nullptr;
}

SqliteDb& SqliteDb::operator=(SqliteDb&& other) noexcept {
    if (this != &other) {
        sqlite3_close(db_);
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

std::expected<void, DbError> SqliteDb::exec(const std::string& sql, const std::vector<std::string>& params) {
    char* errMsg = nullptr;

    // no params -> run sql directly
    if (params.empty()) {
        if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
            DbError error{errMsg ? errMsg : "unknown error"};
            sqlite3_free(errMsg);
            return std::unexpected(error);
        }
    } else {
        // has params -> prepared statement to avoid SQL injection
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
            return std::unexpected(DbError{sqlite3_errmsg(db_)});

        //bind each param to its '?' placeholder
        for (size_t i=0; i<params.size(); ++i)
            sqlite3_bind_text(stmt, static_cast<int>(i+1), params[i].c_str(), -1, SQLITE_TRANSIENT);

        int stepRes = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (stepRes != SQLITE_DONE)
            return std::unexpected(DbError{sqlite3_errmsg(db_)});
    }

    return {};
}

std::expected<nlohmann::json, DbError> SqliteDb::query(const std::string& sql, const std::vector<std::string>& params) {
    sqlite3_stmt* stmt = nullptr;

    //compile sql into a prepared statement
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return std::unexpected(DbError{sqlite3_errmsg(db_)});

    //bind each param to its '?' placeholder (SQLite params are 1-indexed)
    for (size_t i = 0; i < params.size(); ++i)
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_TRANSIENT);

    nlohmann::json rows = nlohmann::json::array();
    int stepResult;

    //step through each result row
    while ((stepResult = sqlite3_step(stmt)) == SQLITE_ROW) {
        nlohmann::json row = nlohmann::json::object();
        int columnCount = sqlite3_column_count(stmt);

        //read every column in this row, keyed by column name
        for (int i=0; i<columnCount; ++i) {
            std::string columnName = sqlite3_column_name(stmt, i);

            switch (sqlite3_column_type(stmt, i)) {
                case SQLITE_INTEGER:
                    row[columnName] = sqlite3_column_int64(stmt, i);
                    break;
                case SQLITE_FLOAT:
                    row[columnName] = sqlite3_column_double(stmt, i);
                    break;
                case SQLITE_TEXT:
                    row[columnName] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                    break;
                case SQLITE_NULL:
                    row[columnName] = nullptr;
                    break;
                default:
                    row[columnName] = nullptr; //blob and unexpected types unsupported for now
                    break;
            }
        }

        rows.push_back(row);
    }

    //always release statement, success or failure
    sqlite3_finalize(stmt);

    // SQLITE_DONE -> loop ended normally
    if (stepResult != SQLITE_DONE)
        return std::unexpected(DbError{sqlite3_errmsg(db_)});

    return rows;
}

int64_t SqliteDb::lastInsertId() const {
    return sqlite3_last_insert_rowid(db_);
}

sqlite3* SqliteDb::handle() const {
    return db_;
}

// ----- TRANSACTIONS --------

std::expected<void, DbError> SqliteDb::beginTx() {
    auto result = exec("BEGIN;");
    if (!result)
        return std::unexpected(DbError{"Failed to begin transaction: " + result.error().msg});

    return result;
}

std::expected<void, DbError> SqliteDb::commitTx() {
    auto result = exec("COMMIT;");
    if (!result)
        return std::unexpected(DbError{"Failed to commit transaction: " + result.error().msg});

    return result;
}

std::expected<void, DbError> SqliteDb::rollbackTx() {
    auto result = exec("ROLLBACK;");
    if (!result)
        return std::unexpected(DbError{"Failed to rollback transaction: " + result.error().msg});

    return result;
}

}