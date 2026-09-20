#include "storage/DbMigrator.h"
#include "storage/DbTransaction.h"

#include <algorithm>
#include <print>

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

std::expected<void, DbError> DbMigrator::applyMigration(const Migration &migration) {
    auto txResult = DbTransaction::begin(db_);
    if (!txResult)
        return std::unexpected(txResult.error());

    DbTransaction tx = std::move(*txResult);

    auto execResult = db_.exec(migration.sql);
    if (!execResult)
        return execResult;

    std::string insertSql =
        "INSERT INTO schema_migrations (version) VALUES (" + std::to_string(migration.version) + ");";
    auto insertResult = db_.exec(insertSql);
    if (!insertResult)
        return insertResult;

    return tx.commit();
}

std::expected<void, DbError> DbMigrator::run(const std::vector<Migration>& migrations) {
    auto tableResult = ensureMigrationsTableExists();
    if (!tableResult)
        return tableResult;

    auto appliedResult = getAppliedVersions();
    if (!appliedResult)
        return std::unexpected(appliedResult.error());

    const std::vector<int>& applied = *appliedResult;

    for (const auto& migration : migrations) {
        bool alreadyApplied = std::find(applied.begin(), applied.end(), migration.version) != applied.end();
        if (alreadyApplied)
            continue;

        std::println("Applying migration {}...", migration.version);
        auto applyResult = applyMigration(migration);
        if (!applyResult)
            return applyResult;
    }

    return {};
}

// namespace nubilo
}