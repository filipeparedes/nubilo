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

// Registers a user with the given email and logs in, returning a valid session token.
std::string registerAndLoginAs(httplib::Client& client, const std::string& email) {
    nlohmann::json body = {{"email", email}, {"password", "password123"}};
    client.Post("/auth/register", body.dump(), "application/json");
    auto loginResult = client.Post("/auth/login", body.dump(), "application/json");
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

TEST(FileRoutesTest, DownloadReturnsUploadedContent) {
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

    constexpr int testPort = 8105;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    auto uploadResult = client.Post("/files", headers, items);
    ASSERT_TRUE(uploadResult);
    ASSERT_EQ(uploadResult->status, 201);
    int64_t fileId = nlohmann::json::parse(uploadResult->body)["id"].get<int64_t>();

    auto downloadResult = client.Get("/files/" + std::to_string(fileId), headers);

    server.stop();
    serverThread.join();

    ASSERT_TRUE(downloadResult);
    EXPECT_EQ(downloadResult->status, 200);
    EXPECT_EQ(downloadResult->body, "hello, nubilo");
    EXPECT_EQ(downloadResult->get_header_value("Content-Type"), "text/plain");
    EXPECT_NE(downloadResult->get_header_value("Content-Disposition").find("notes.txt"), std::string::npos);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, DownloadRejectsMissingToken) {
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

    constexpr int testPort = 8106;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    auto result = client.Get("/files/1");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, DownloadRejectsInvalidToken) {
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

    constexpr int testPort = 8107;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    httplib::Headers headers = {{"Authorization", "Bearer not-a-real-token"}};
    auto result = client.Get("/files/1", headers);

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, DownloadReturnsNotFoundForNonexistentFile) {
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

    constexpr int testPort = 8108;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};
    auto result = client.Get("/files/9999", headers);

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, DownloadReturnsNotFoundForFileOwnedByAnotherUser) {
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

    constexpr int testPort = 8109;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    std::string ownerToken = registerAndLoginAs(client, "owner@example.com");
    std::string otherToken = registerAndLoginAs(client, "other@example.com");

    httplib::MultipartFormDataItems items = {
        {"file", "private content", "secret.txt", "text/plain"}
    };
    httplib::Headers ownerHeaders = {{"Authorization", "Bearer " + ownerToken}};
    auto uploadResult = client.Post("/files", ownerHeaders, items);
    ASSERT_TRUE(uploadResult);
    ASSERT_EQ(uploadResult->status, 201);
    int64_t fileId = nlohmann::json::parse(uploadResult->body)["id"].get<int64_t>();

    httplib::Headers otherHeaders = {{"Authorization", "Bearer " + otherToken}};
    auto result = client.Get("/files/" + std::to_string(fileId), otherHeaders);

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, ListReturnsUploadedFiles) {
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

    constexpr int testPort = 8110;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    auto uploadResult = client.Post("/files", headers, items);
    ASSERT_TRUE(uploadResult);
    ASSERT_EQ(uploadResult->status, 201);
    int64_t fileId = nlohmann::json::parse(uploadResult->body)["id"].get<int64_t>();

    auto listResult = client.Get("/files", headers);

    server.stop();
    serverThread.join();

    ASSERT_TRUE(listResult);
    ASSERT_EQ(listResult->status, 200);

    auto body = nlohmann::json::parse(listResult->body);
    ASSERT_TRUE(body.is_array());
    ASSERT_EQ(body.size(), 1);
    EXPECT_EQ(body[0]["id"].get<int64_t>(), fileId);
    EXPECT_EQ(body[0]["path"].get<std::string>(), "notes.txt");
    EXPECT_EQ(body[0]["size"].get<int64_t>(), 13);
    EXPECT_EQ(body[0]["contentType"].get<std::string>(), "text/plain");

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}