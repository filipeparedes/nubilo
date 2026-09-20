#pragma once

#include "storage/DbError.h"
#include "storage/SqliteDb.h"

#include <expected>
#include <string>
#include <vector>

namespace nubilo {

/**
 * @struct DbMigration
 * @brief A single schema change: a version number and the SQL to apply it.
 */
struct DbMigration {
    int version;
    std::string sql;
};

/**
 * @class DbMigrator
 * @brief Applies a list of Migrations to a SqliteDb, keeping track of which versions
 * have already run so each migration is applied exactly once.
 */
class DbMigrator {
public:
    explicit DbMigrator(SqliteDb& db);

    /**
     * @brief Applies any migrations not yet recorded as run, in version order.
     * @param migrations The full list of migrations the schema should have.
     * @return Nothing on success, or a DbError on the first failure encountered.
     */
std::expected<void, DbError> run(const std::vector<DbMigration>& migrations);

private:
    /**
     * @brief Creates the schema_migrations tracking table if it doesn't exist yet.
     * @return Nothing on success, or a DbError on failure.
     */
    std::expected<void, DbError> ensureMigrationsTableExists();

    /**
     * @brief Reads which migration versions have already been applied.
     * @return The list of applied version numbers, or a DbError on failure.
     */
    std::expected<std::vector<int>, DbError> getAppliedVersions();

    /**
     * @brief Runs a single migration's SQL and records it as applied.
     * @param migration The migration to apply.
     * @return Nothing on success, or a DbError on failure.
     */
    std::expected<void, DbError> applyMigration(const DbMigration& migration);

    SqliteDb& db_;
};

}