#include "api/HttpServer.h"
#include "api/Router.h"
#include "api/routes/authRoutes.h"
#include "storage/DbMigrator.h"
#include "storage/DbMigrations.h"
#include "storage/SqliteDb.h"

#include <gtest/gtest.h>
#include <httplib.h>
#include <filesystem>
#include <thread>

namespace {

std::string tempDbPath() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_auth_routes_test.db";
    std::filesystem::remove(path);
    return path.string();
}

}

TEST(AuthRoutesTest, RegisterReturnsCreatedOnSuccess) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::SqliteDb::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::SqliteDb db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    constexpr int testPort = 8082;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    auto result = client.Post("/auth/register", R"({"email":"user@example.com","password":"password123"})", "application/json");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 201);

    std::filesystem::remove(dbPath);
}

TEST(AuthRoutesTest, RegisterRejectsDuplicateEmail) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::SqliteDb::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::SqliteDb db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    constexpr int testPort = 8083;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    client.Post("/auth/register", R"({"email":"user@example.com","password":"password123"})", "application/json");
    auto second = client.Post("/auth/register", R"({"email":"user@example.com","password":"other"})", "application/json");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(second);
    EXPECT_EQ(second->status, 409);

    std::filesystem::remove(dbPath);
}

TEST(AuthRoutesTest, RegisterRejectsMissingFields) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::SqliteDb::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::SqliteDb db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    constexpr int testPort = 8084;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    auto result = client.Post("/auth/register", R"({"email":"user@example.com"})", "application/json");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 400);

    std::filesystem::remove(dbPath);
}

TEST(AuthRoutesTest, RegisterRejectsInvalidEmailFormat) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::SqliteDb::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::SqliteDb db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    constexpr int testPort = 8085;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    auto result = client.Post("/auth/register", R"({"email":"not-an-email","password":"password123"})", "application/json");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 400);

    std::filesystem::remove(dbPath);
}

TEST(AuthRoutesTest, LoginSucceedsWithCorrectCredentials) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::SqliteDb::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::SqliteDb db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    constexpr int testPort = 8086;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    client.Post("/auth/register", R"({"email":"user@example.com","password":"password123"})", "application/json");
    auto result = client.Post("/auth/login", R"({"email":"user@example.com","password":"password123"})", "application/json");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 200);

    std::filesystem::remove(dbPath);
}

TEST(AuthRoutesTest, LoginRejectsWrongPassword) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::SqliteDb::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::SqliteDb db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    constexpr int testPort = 8087;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    client.Post("/auth/register", R"({"email":"user@example.com","password":"password123"})", "application/json");
    auto result = client.Post("/auth/login", R"({"email":"user@example.com","password":"wrongPassword"})", "application/json");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);

    std::filesystem::remove(dbPath);
}

TEST(AuthRoutesTest, LoginRejectsNonexistentEmail) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::SqliteDb::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::SqliteDb db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    constexpr int testPort = 8088;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    auto result = client.Post("/auth/login", R"({"email":"nobody@example.com","password":"password123"})", "application/json");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);

    std::filesystem::remove(dbPath);
}

TEST(AuthRoutesTest, LoginRejectsMissingFields) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::SqliteDb::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::SqliteDb db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    constexpr int testPort = 8089;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    auto result = client.Post("/auth/login", R"({"email":"user@example.com"})", "application/json");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 400);

    std::filesystem::remove(dbPath);
}