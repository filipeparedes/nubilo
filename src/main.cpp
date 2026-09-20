#include "api/HttpServer.h"
#include "api/Router.h"
#include "api/routes/HealthRoutes.h"
#include "storage/DbMigrations.h"
#include "storage/DbMigrator.h"
#include "storage/SqliteDb.h"
#include "version.h"

#include <print>

/**
 * @brief Starts the Nubilo backend server.
 * @return 0 on normal exit.
 */
int main() {
    std::println("Nubilo backend {} starting...", nubilo::getVersion());

    // ------ DATABASE -----

    auto dbResult = nubilo::SqliteDb::open("nubilo.db");
    if (!dbResult) {
        std::println(stderr, "Failed to open database: {}", dbResult.error().msg);
        return 1;
    }
    nubilo::SqliteDb db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    auto migrateResult = migrator.run(nubilo::migrations);
    if (!migrateResult) {
        std::println(stderr, "Migration failed: {}", migrateResult.error().msg);
        return 1;
    }

    std::println("Database loaded successfully.");

    // ---- HTTP SERVER ----

    nubilo::HttpServer server(8080);
    nubilo::Router router;

    nubilo::registerHealthRoutes(router);

    router.applyTo(server.getServer());
    server.run();

    return 0;
}
