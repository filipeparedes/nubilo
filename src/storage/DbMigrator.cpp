#include "storage/DbMigrator.h"

namespace nubilo {

DbMigrator::DbMigrator(SqliteDb& db) : db_(db) {}

std::expected<void, DbError> DbMigrator::ensureMigrationsTableExists() {
    return db_.exec(
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        " version INTEGER PRIMARY KEY,"
        " applied_at TEXT NOT NULL DEFAULT (datetime('now'))"
        ");"
    );
}

std::expected<std::vector<int>, DbError> DbMigrator::getAppliedVersions() {
    auto result = db_.query("SELECT version FROM schema_migrations;");
    if (!result)
        return std::unexpected(result.error());
    
    std::vector<int> versions;
    for (const auto& row : *result)
        versions.push_back(row["version"].get<int>());

    return versions;
 }

}