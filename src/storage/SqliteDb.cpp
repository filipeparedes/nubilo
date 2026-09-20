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

std::expected<void, DbError> SqliteDb::exec(const std::string& sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        DbError error{errMsg ? errMsg : "unknown error"};
        sqlite3_free(errMsg);
        return std::unexpected(error);
    }
    return {};
}

sqlite3* SqliteDb::handle() const {
    return db_;
}

}