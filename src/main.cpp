#include "api/HttpServer.h"
#include "api/Router.h"
#include "api/routes/authRoutes.h"
#include "api/routes/fileRoutes.h"
#include "api/routes/healthRoutes.h"
#include "blob/BlobStore.h"
#include "storage/Db.h"
#include "storage/DbMigrations.h"
#include "storage/DbMigrator.h"
#include "version.h"

#include <print>

/**
 * @brief Starts the Nubilo backend server.
 * @return 0 on normal exit.
 */
int main() {
    std::println("Nubilo backend {} starting...", nubilo::getVersion());

    // ------ DATABASE -----

    auto dbResult = nubilo::Db::open("nubilo.db");
    if (!dbResult) {
        std::println(stderr, "Failed to open database: {}", dbResult.error().msg);
        return 1;
    }
    nubilo::Db db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    auto migrateResult = migrator.run(nubilo::migrations);
    if (!migrateResult) {
        std::println(stderr, "Migration failed: {}", migrateResult.error().msg);
        return 1;
    }

    std::println("Database loaded successfully.");

    // ---- STORES ---
    auto blobStoreResult = nubilo::BlobStore::open("blobs");
    if (!blobStoreResult) {
        std::println(stderr, "Failed to open blob store: {}", blobStoreResult.error().message);
    }
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    // ---- HTTP SERVER ----

    nubilo::HttpServer server(8080);
    nubilo::Router router;

    nubilo::registerHealthRoutes(router);
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);

    router.applyTo(server.getServer());
    server.run();

    return 0;
}
