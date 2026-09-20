#include "storage/SqliteDb.h"

#include <stdexcept>

namespace nubilo {

SqliteDb::SqliteDb(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK)
        throw std::runtime_error("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
}

SqliteDb::~SqliteDb() {
    sqlite3_close(db_);
}

void SqliteDb::exec(const std::string& sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to execute SQL: " + std::string(sqlite3_errmsg(db_)));
    }
}

sqlite3* SqliteDb::handle() const {
    return db_;
}

}