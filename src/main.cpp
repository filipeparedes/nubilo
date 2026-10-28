#include "api/HttpServer.h"
#include "api/Router.h"
#include "api/routes/authRoutes.h"
#include "api/routes/fileRoutes.h"
#include "api/routes/healthRoutes.h"
#include "blob/BlobStore.h"
#include "concurrency/ThreadPool.h"
#include "config.h"
#include "storage/Db.h"
#include "storage/DbMigrations.h"
#include "storage/DbMigrator.h"

#include <memory>
#include <print>
#include <string>

/**
 * @brief Starts the Nubilo backend server.
 * @param argc The number of command line arguments.
 * @param argv The command line arguments. The first one, if given, is the path of the
 * configuration file (config.json by default).
 * @return 0 on normal exit.
 */
int main(int argc, char** argv) {
    std::println("Nubilo server starting...");

    // ---- CONFIGURATION ----

    const std::string configPath = argc > 1 ? argv[1] : "config.json";
    auto configResult = nubilo::loadConfig(configPath);
    if (!configResult) {
        std::println(stderr, "Failed to load configuration: {} \n", configResult.error().message);
        return 1;
    }
    const auto& config = *configResult;
    const std::string host = config["server"]["host"].get<std::string>();
    const int port = config["server"]["port"].get<int>();
    const std::string dbPath = config["storage"]["dbPath"].get<std::string>();
    const std::string blobDir = config["storage"]["blobDir"].get<std::string>();
    const size_t threads = config["storage"]["threads"].get<size_t>();
    const int sessionDays = config["auth"]["sessionDays"].get<int>();
    const std::string corsOrigin = config["server"]["corsOrigin"].get<std::string>();

    std::println("Configuration loaded from {}.", configPath);

    // ------ DATABASE -----

    auto dbResult = nubilo::Db::open(dbPath);
    if (!dbResult) {
        std::println(stderr, "Failed to open database: {} \n", dbResult.error().msg);
        return 1;
    }
    nubilo::Db db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    auto migrateResult = migrator.run(nubilo::migrations);
    if (!migrateResult) {
        std::println(stderr, "Migration failed: {} \n", migrateResult.error().msg);
        return 1;
    }

    std::println("Database loaded successfully.");

    // ---- STORES ---
    auto blobStoreResult = nubilo::BlobStore::open(blobDir);
    if (!blobStoreResult) {
        std::println(stderr, "Failed to open blob store: {} \n", blobStoreResult.error().message);
        return 1;
    }
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    // --- THREAD POOL ---

    std::unique_ptr<nubilo::ThreadPool> pool;
    if (auto poolRes = nubilo::ThreadPool::create(threads))
        pool = std::move(*poolRes);
    else
        std::println(stderr, "Failed to create thread pool, uploads will store chunks sequentially: {} \n", poolRes.error().msg);

    // ---- HTTP SERVER ----

    nubilo::HttpServer server(port, host);
    nubilo::Router router;

    nubilo::registerHealthRoutes(router);
    nubilo::registerAuthRoutes(router, db, sessionDays);
    nubilo::registerFileRoutes(router, db, blobStore, pool.get());

    router.applyTo(server.getServer());
    server.enableCors(corsOrigin);
    server.run();

    return 0;
}