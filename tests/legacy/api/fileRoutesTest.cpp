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