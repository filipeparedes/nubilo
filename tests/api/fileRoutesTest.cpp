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

nubilo::Db makeMigratedDb(const std::string& path) {
    auto dbResult = nubilo::Db::open(path);
    nubilo::Db db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    auto res = migrator.run(nubilo::migrations);

    return db;
}

// A server with the auth and file routes running, for the folder route tests.
struct RoutesEnv {
    explicit RoutesEnv(int port)
        : dbPath(tempDbPath()),
          db(makeMigratedDb(dbPath)),
          blobRoot(tempBlobRoot()),
          blobStore(std::move(nubilo::BlobStore::open(blobRoot).value())),
          server(port) {
        nubilo::registerAuthRoutes(router, db);
        nubilo::registerFileRoutes(router, db, blobStore);
        router.applyTo(server.getServer());
        serverThread = std::thread([this]() { server.run(); });
    }

    ~RoutesEnv() {
        server.stop();
        serverThread.join();
        std::filesystem::remove(dbPath);
        std::filesystem::remove_all(blobRoot);
    }

    std::string dbPath;
    nubilo::Db db;
    std::filesystem::path blobRoot;
    nubilo::BlobStore blobStore;
    nubilo::HttpServer server;
    nubilo::Router router;
    std::thread serverThread;
};

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

TEST(FileRoutesTest, DeleteRemovesFile) {
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

    constexpr int testPort = 8111;
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
    std::string path = "/files/" + std::to_string(nlohmann::json::parse(uploadResult->body)["id"].get<int64_t>());

    auto deleteResult = client.Delete(path, headers);
    auto downloadResult = client.Get(path, headers);

    server.stop();
    serverThread.join();

    ASSERT_TRUE(deleteResult);
    EXPECT_EQ(deleteResult->status, 204);

    ASSERT_TRUE(downloadResult);
    EXPECT_EQ(downloadResult->status, 404);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, RestoreCreatesNewVersionWithPreviousContent) {
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

    constexpr int testPort = 8112;
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

    httplib::MultipartFormDataItems firstItems = {{"file", "first version", "notes.txt", "text/plain"}};
    httplib::MultipartFormDataItems secondItems = {{"file", "second version", "notes.txt", "text/plain"}};
    auto firstUpload = client.Post("/files", headers, firstItems);
    auto secondUpload = client.Post("/files", headers, secondItems);
    ASSERT_TRUE(firstUpload);
    ASSERT_TRUE(secondUpload);
    int64_t fileId = nlohmann::json::parse(firstUpload->body)["id"].get<int64_t>();
    ASSERT_EQ(nlohmann::json::parse(secondUpload->body)["id"].get<int64_t>(), fileId);

    std::string restorePath = "/files/" + std::to_string(fileId) + "/versions/1/restore";
    auto restoreResult = client.Post(restorePath, headers, "", "text/plain");
    auto downloadResult = client.Get("/files/" + std::to_string(fileId), headers);

    server.stop();
    serverThread.join();

    ASSERT_TRUE(restoreResult);
    EXPECT_EQ(restoreResult->status, 201);
    auto body = nlohmann::json::parse(restoreResult->body);
    EXPECT_EQ(body["id"].get<int64_t>(), fileId);
    EXPECT_EQ(body["version"].get<int64_t>(), 3);

    ASSERT_TRUE(downloadResult);
    EXPECT_EQ(downloadResult->status, 200);
    EXPECT_EQ(downloadResult->body, "first version");

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, RestoreRejectsMissingToken) {
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

    constexpr int testPort = 8113;
    nubilo::HttpServer server(testPort);
    nubilo::Router router;
    nubilo::registerAuthRoutes(router, db);
    nubilo::registerFileRoutes(router, db, blobStore);
    router.applyTo(server.getServer());

    std::thread serverThread([&server]() { server.run(); });

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    auto result = client.Post("/files/1/versions/1/restore", "", "text/plain");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, RestoreReturnsNotFoundForNonexistentVersion) {
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

    constexpr int testPort = 8114;
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

    httplib::MultipartFormDataItems items = {{"file", "hello, nubilo", "notes.txt", "text/plain"}};
    auto uploadResult = client.Post("/files", headers, items);
    ASSERT_TRUE(uploadResult);
    int64_t fileId = nlohmann::json::parse(uploadResult->body)["id"].get<int64_t>();

    auto result = client.Post("/files/" + std::to_string(fileId) + "/versions/99/restore", headers, "", "text/plain");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, RestoreReturnsNotFoundForNonexistentFile) {
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

    constexpr int testPort = 8115;
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
    auto result = client.Post("/files/9999/versions/1/restore", headers, "", "text/plain");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FileRoutesTest, RestoreReturnsNotFoundForFileOwnedByAnotherUser) {
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

    constexpr int testPort = 8116;
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

    httplib::MultipartFormDataItems items = {{"file", "private content", "secret.txt", "text/plain"}};
    httplib::Headers ownerHeaders = {{"Authorization", "Bearer " + ownerToken}};
    auto uploadResult = client.Post("/files", ownerHeaders, items);
    ASSERT_TRUE(uploadResult);
    int64_t fileId = nlohmann::json::parse(uploadResult->body)["id"].get<int64_t>();

    httplib::Headers otherHeaders = {{"Authorization", "Bearer " + otherToken}};
    auto result = client.Post("/files/" + std::to_string(fileId) + "/versions/1/restore", otherHeaders, "", "text/plain");

    server.stop();
    serverThread.join();

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 404);

    //the owner's file was not touched
    auto versions = db.query("SELECT COUNT(*) AS count FROM file_versions;");
    ASSERT_TRUE(versions);
    EXPECT_EQ((*versions)[0]["count"].get<int64_t>(), 1);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(FolderRoutesTest, CreateFolderReturnsCreatedAndCreatesParents) {
    constexpr int testPort = 8119;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto result = client.Post("/folders", headers, R"({"path":"docs/2026"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 201);
    EXPECT_TRUE(nlohmann::json::parse(result->body).contains("id"));

    auto folders = env.db.query("SELECT path FROM folders ORDER BY path;");
    ASSERT_TRUE(folders);
    ASSERT_EQ(folders->size(), 2);
    EXPECT_EQ((*folders)[0]["path"].get<std::string>(), "docs");
    EXPECT_EQ((*folders)[1]["path"].get<std::string>(), "docs/2026");
}

TEST(FolderRoutesTest, CreateFolderRejectsInvalidPath) {
    constexpr int testPort = 8120;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto result = client.Post("/folders", headers, R"({"path":"../secret"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 400);
    EXPECT_EQ(nlohmann::json::parse(result->body)["error"]["code"], "INVALID_PATH");
}

TEST(FolderRoutesTest, CreateFolderRejectsMissingPathField) {
    constexpr int testPort = 8121;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto result = client.Post("/folders", headers, R"({"name":"docs"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 400);
    EXPECT_EQ(nlohmann::json::parse(result->body)["error"]["code"], "INVALID_REQUEST");
}

TEST(FolderRoutesTest, FolderRoutesRejectMissingToken) {
    constexpr int testPort = 8122;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    auto create = client.Post("/folders", R"({"path":"docs"})", "application/json");
    auto list = client.Get("/folders");

    ASSERT_TRUE(create);
    ASSERT_TRUE(list);
    EXPECT_EQ(create->status, 401);
    EXPECT_EQ(list->status, 401);
}

TEST(FolderRoutesTest, ListRootShowsFoldersAndFiles) {
    constexpr int testPort = 8123;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    client.Post("/folders", headers, R"({"path":"docs"})", "application/json");
    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    client.Post("/files", headers, items);

    auto result = client.Get("/folders", headers);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->status, 200);
    auto body = nlohmann::json::parse(result->body);
    ASSERT_EQ(body["folders"].size(), 1);
    EXPECT_EQ(body["folders"][0]["path"].get<std::string>(), "docs");
    ASSERT_EQ(body["files"].size(), 1);
    EXPECT_EQ(body["files"][0]["path"].get<std::string>(), "notes.txt");
}

TEST(FolderRoutesTest, ListFolderByIdShowsItsDirectChildren) {
    constexpr int testPort = 8124;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto created = client.Post("/folders", headers, R"({"path":"docs"})", "application/json");
    ASSERT_TRUE(created);
    int64_t docsId = nlohmann::json::parse(created->body)["id"].get<int64_t>();

    client.Post("/folders", headers, R"({"path":"docs/sub"})", "application/json");
    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    client.Post("/files?path=docs/a.txt", headers, items);

    auto result = client.Get("/folders/" + std::to_string(docsId), headers);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->status, 200);
    auto body = nlohmann::json::parse(result->body);
    ASSERT_EQ(body["folders"].size(), 1);
    EXPECT_EQ(body["folders"][0]["path"].get<std::string>(), "docs/sub");
    ASSERT_EQ(body["files"].size(), 1);
    EXPECT_EQ(body["files"][0]["path"].get<std::string>(), "docs/a.txt");
}

TEST(FolderRoutesTest, ListFolderReturnsNotFoundForMissingOrForeignFolder) {
    constexpr int testPort = 8125;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string otherToken = registerAndLoginAs(client, "other@example.com");
    httplib::Headers otherHeaders = {{"Authorization", "Bearer " + otherToken}};
    auto created = client.Post("/folders", otherHeaders, R"({"path":"private"})", "application/json");
    ASSERT_TRUE(created);
    int64_t foreignId = nlohmann::json::parse(created->body)["id"].get<int64_t>();

    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto foreign = client.Get("/folders/" + std::to_string(foreignId), headers);
    auto missing = client.Get("/folders/9999", headers);

    ASSERT_TRUE(foreign);
    ASSERT_TRUE(missing);
    EXPECT_EQ(foreign->status, 404);
    EXPECT_EQ(missing->status, 404);
}

TEST(FolderRoutesTest, DeleteRemovesFolderAndEverythingInsideIt) {
    constexpr int testPort = 8127;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto created = client.Post("/folders", headers, R"({"path":"docs"})", "application/json");
    ASSERT_TRUE(created);
    int64_t id = nlohmann::json::parse(created->body)["id"].get<int64_t>();
    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "notes.txt", "text/plain"}
    };
    client.Post("/files?path=docs/sub/a.txt", headers, items);

    auto result = client.Delete("/folders/" + std::to_string(id), headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 204);
    auto files = env.db.query("SELECT id FROM files;");
    auto folders = env.db.query("SELECT id FROM folders;");
    ASSERT_TRUE(files);
    ASSERT_TRUE(folders);
    EXPECT_TRUE(files->empty());
    EXPECT_TRUE(folders->empty());
}

TEST(FolderRoutesTest, DeleteRemovesEmptyFolder) {
    constexpr int testPort = 8126;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto created = client.Post("/folders", headers, R"({"path":"docs"})", "application/json");
    ASSERT_TRUE(created);
    int64_t id = nlohmann::json::parse(created->body)["id"].get<int64_t>();

    auto result = client.Delete("/folders/" + std::to_string(id), headers);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 204);
    auto rows = env.db.query("SELECT id FROM folders;");
    ASSERT_TRUE(rows);
    EXPECT_TRUE(rows->empty());
}

TEST(FolderRoutesTest, DeleteRejectsMissingToken) {
    constexpr int testPort = 8128;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    auto result = client.Delete("/folders/1");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(FolderRoutesTest, DeleteReturnsNotFoundForMissingOrForeignFolder) {
    constexpr int testPort = 8129;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string otherToken = registerAndLoginAs(client, "other@example.com");
    httplib::Headers otherHeaders = {{"Authorization", "Bearer " + otherToken}};
    auto created = client.Post("/folders", otherHeaders, R"({"path":"private"})", "application/json");
    ASSERT_TRUE(created);
    int64_t foreignId = nlohmann::json::parse(created->body)["id"].get<int64_t>();

    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto foreign = client.Delete("/folders/" + std::to_string(foreignId), headers);
    auto missing = client.Delete("/folders/9999", headers);

    ASSERT_TRUE(foreign);
    ASSERT_TRUE(missing);
    EXPECT_EQ(foreign->status, 404);
    EXPECT_EQ(missing->status, 404);
}

TEST(FolderRoutesTest, CreateFolderReturnsConflictWhenFileHasThatPath) {
    constexpr int testPort = 8130;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "docs", "text/plain"}
    };
    client.Post("/files", headers, items);

    auto result = client.Post("/folders", headers, R"({"path":"docs"})", "application/json");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 409);
    EXPECT_EQ(nlohmann::json::parse(result->body)["error"]["code"], "PATH_CONFLICT");
}

TEST(FolderRoutesTest, UploadReturnsConflictWhenFolderHasThatPath) {
    constexpr int testPort = 8131;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    client.Post("/folders", headers, R"({"path":"docs"})", "application/json");
    httplib::MultipartFormDataItems items = {
        {"file", "hello, nubilo", "docs", "text/plain"}
    };

    auto result = client.Post("/files", headers, items);

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 409);
    EXPECT_EQ(nlohmann::json::parse(result->body)["error"]["code"], "PATH_CONFLICT");
}

TEST(VersionRoutesTest, ListReturnsVersionsNewestFirst) {
    constexpr int testPort = 8132;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems firstItems = {{"file", "first", "notes.txt", "text/plain"}};
    httplib::MultipartFormDataItems secondItems = {{"file", "second version", "notes.txt", "text/plain"}};
    auto upload = client.Post("/files", headers, firstItems);
    client.Post("/files", headers, secondItems);
    ASSERT_TRUE(upload);
    int64_t fileId = nlohmann::json::parse(upload->body)["id"].get<int64_t>();

    auto result = client.Get("/files/" + std::to_string(fileId) + "/versions", headers);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->status, 200);
    auto body = nlohmann::json::parse(result->body);
    ASSERT_EQ(body.size(), 2);
    EXPECT_EQ(body[0]["version"].get<int64_t>(), 2);
    EXPECT_EQ(body[0]["size"].get<int64_t>(), 14);
    EXPECT_EQ(body[0]["contentType"].get<std::string>(), "text/plain");
    EXPECT_TRUE(body[0].contains("createdAt"));
    EXPECT_EQ(body[1]["version"].get<int64_t>(), 1);
    EXPECT_EQ(body[1]["size"].get<int64_t>(), 5);
}

TEST(VersionRoutesTest, ListShowsVersionCreatedByRestore) {
    constexpr int testPort = 8133;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    httplib::MultipartFormDataItems firstItems = {{"file", "first", "notes.txt", "text/plain"}};
    httplib::MultipartFormDataItems secondItems = {{"file", "second", "notes.txt", "text/plain"}};
    auto upload = client.Post("/files", headers, firstItems);
    client.Post("/files", headers, secondItems);
    ASSERT_TRUE(upload);
    std::string base = "/files/" + std::to_string(nlohmann::json::parse(upload->body)["id"].get<int64_t>());
    client.Post(base + "/versions/1/restore", headers, "", "text/plain");

    auto result = client.Get(base + "/versions", headers);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->status, 200);
    auto body = nlohmann::json::parse(result->body);
    ASSERT_EQ(body.size(), 3);
    EXPECT_EQ(body[0]["version"].get<int64_t>(), 3);
}

TEST(VersionRoutesTest, ListRejectsMissingToken) {
    constexpr int testPort = 8134;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);

    auto result = client.Get("/files/1/versions");

    ASSERT_TRUE(result);
    EXPECT_EQ(result->status, 401);
}

TEST(VersionRoutesTest, ListReturnsNotFoundForMissingOrForeignFile) {
    constexpr int testPort = 8135;
    RoutesEnv env(testPort);

    httplib::Client client("localhost", testPort);
    client.set_connection_timeout(2);
    std::string otherToken = registerAndLoginAs(client, "other@example.com");
    httplib::Headers otherHeaders = {{"Authorization", "Bearer " + otherToken}};
    httplib::MultipartFormDataItems items = {{"file", "secret", "private.txt", "text/plain"}};
    auto upload = client.Post("/files", otherHeaders, items);
    ASSERT_TRUE(upload);
    int64_t foreignId = nlohmann::json::parse(upload->body)["id"].get<int64_t>();

    std::string token = registerAndLogin(client);
    httplib::Headers headers = {{"Authorization", "Bearer " + token}};

    auto foreign = client.Get("/files/" + std::to_string(foreignId) + "/versions", headers);
    auto missing = client.Get("/files/9999/versions", headers);

    ASSERT_TRUE(foreign);
    ASSERT_TRUE(missing);
    EXPECT_EQ(foreign->status, 404);
    EXPECT_EQ(missing->status, 404);
}