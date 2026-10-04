#include "api/routes/fileRoutes.h"
#include "api/HttpServer.h"
#include "api/Router.h"
#include "api/routes/authRoutes.h"
#include "blob/blobService.h"
#include "storage/Db.h"
#include "storage/DbMigrations.h"
#include "storage/DbMigrator.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <thread>

namespace {

std::string tempDbPath() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_file_routes_test.db";
    std::filesystem::remove(path);
    return path.string();
}

std::filesystem::path tempBlobRoot() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_file_routes_test_blobs";
    std::filesystem::remove_all(path);
    return path;
}

// Registers a user and logs in, returning a valid session token.
std::string registerAndLogin(httplib::Client& client) {
    client.Post("/auth/register", R"({"email":"user@example.com","password":"password123"})", "application/json");
    auto loginResult = client.Post("/auth/login", R"({"email":"user@example.com","password":"password123"})", "application/json");
    return nlohmann::json::parse(loginResult->body)["token"];
}

}

TEST(FileRoutesTest, UploadReturnsCreatedOnSuccess) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::Db::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::Db db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    constexpr int testPort = 8100;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    std::string token = registerAndLogin(client);

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    auto result = client.Post("/files", headers, items);

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 201);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, UploadRejectsMissingFile) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::Db::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::Db db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    constexpr int testPort = 8101;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    std::string token = registerAndLogin(client);

    httplib::MultipartFormDataItems items = {
        {"path", "notes.txt", "", ""}
    };
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    auto result = client.Post("/files", headers, items);

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 400);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, UploadRejectsMissingToken) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::Db::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::Db db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    constexpr int testPort = 8102;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    auto result = client.Post("/files", items);

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, UploadRejectsInvalidToken) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::Db::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::Db db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    constexpr int testPort = 8103;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    httplib::Headers headers = {{"Authorization", "Bearer not-a-real-token"}};
    auto result = client.Post("/files", headers, items);

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, UploadUsesFilenameWhenPathNotProvided) {
    std::string dbPath = tempDbPath();
    auto dbResult = nubilo::Db::open(dbPath);
    ASSERT_TRUE(dbResult);
    nubilo::Db db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(nubilo::migrations));

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    constexpr int testPort = 8104;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    std::string token = registerAndLogin(client);

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    auto result = client.Post("/files", headers, items);

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    ASSERT_EQ(result->status, 201);

    auto queryResult = db.query("SELECT path FROM files;");
    ASSERT_TRUE(queryResult);
    ASSERT_EQ(queryResult->size(), 1);
    EXPECT_EQ((*queryResult)[0]["path"].get<std::string>(), "notes.txt");

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}