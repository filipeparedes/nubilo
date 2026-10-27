#pragma once

#include "testUtils.h"

#include "api/HttpServer.h"
#include "api/Router.h"
#include "api/routes/authRoutes.h"
#include "api/routes/fileRoutes.h"
#include "api/routes/healthRoutes.h"
#include "blob/BlobStore.h"

#include <filesystem>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <thread>

namespace nubilo::test {

// A running server with the health, auth and file routes, on a temporary database and blob directory.
// It is stopped and cleaned up when the object goes out of scope. Each test must use its own port.
struct RoutesEnv {
    explicit RoutesEnv(int port)
        : port(port),
          dbPath(tempDbPath()),
          db(makeMigratedDb(dbPath)),
          blobRoot(tempBlobRoot()),
          blobStore(std::move(BlobStore::open(blobRoot).value())),
          server(port) {
        registerHealthRoutes(router);
        registerAuthRoutes(router, db);
        registerFileRoutes(router, db, blobStore);
        router.applyTo(server.getServer());
        server.enableCors("http://localhost:5173");
        serverThread = std::thread([this]() { server.run(); });
        server.getServer().wait_until_ready();
    }

    ~RoutesEnv() {
        server.stop();
        serverThread.join();
        std::filesystem::remove(dbPath);
        std::filesystem::remove_all(blobRoot);
    }

    RoutesEnv(const RoutesEnv&) = delete;
    RoutesEnv& operator=(const RoutesEnv&) = delete;

    // A client connected to this server.
    httplib::Client newClient() const {
        httplib::Client client("localhost", port);
        client.set_connection_timeout(2);
        return client;
    }

    int port;
    std::string dbPath;
    Db db;
    std::filesystem::path blobRoot;
    BlobStore blobStore;
    HttpServer server;
    Router router;
    std::thread serverThread;
};

// Registers a user and logs in, returning a valid session token.
inline std::string registerAndLogin(httplib::Client& client) {
    client.Post("/auth/register", R"({"email":"user@example.com","password":"password123"})", "application/json");
    auto loginResult = client.Post("/auth/login", R"({"email":"user@example.com","password":"password123"})", "application/json");
    return nlohmann::json::parse(loginResult->body)["token"];
}

// Registers a user with the given email and logs in, returning a valid session token.
inline std::string registerAndLoginAs(httplib::Client& client, const std::string& email) {
    nlohmann::json body = {{"email", email}, {"password", "password123"}};
    client.Post("/auth/register", body.dump(), "application/json");
    auto loginResult = client.Post("/auth/login", body.dump(), "application/json");
    return nlohmann::json::parse(loginResult->body)["token"];
}

}