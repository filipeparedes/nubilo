#include "blob/blobService.h"

#include "httplib.h"
#include "storage/Db.h"
#include "storage/DbMigrations.h"
#include "storage/DbMigrator.h"

#include <filesystem>
#include <gtest/gtest.h>

namespace {

std::string tempDbPath() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_blob_service_test.db";
    std::filesystem::remove(path);
    return path.string();
}

std::filesystem::path tempBlobRoot() {
    auto path = std::filesystem::temp_directory_path() / "nubilo_blob_service_test_blobs";
    std::filesystem::remove_all(path);
    return path;
}

nubilo::Db makeMigratedDb(const std::string& path) {
    auto dbResult = nubilo::Db::open(path);
    nubilo::Db db = std::move(*dbResult);

    nubilo::DbMigrator migrator(db);
    migrator.run(nubilo::migrations);

    return db;
}

}

TEST(BlobServiceTest, StoreFileSucceedsWithNewContent) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto result = nubilo::storeFile(db, blobStore, 1, file);

    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result.value(), 0);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileCreatesBlobRowForNewContent) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto result = nubilo::storeFile(db, blobStore, 1, file);
    ASSERT_TRUE(result.has_value());

    auto queryResult = db.query("SELECT size, content_type FROM blobs;");
    ASSERT_TRUE(queryResult);
    ASSERT_EQ(queryResult->size(), 1);

    EXPECT_EQ((*queryResult)[0]["size"].get<int64_t>(), static_cast<int64_t>(file.content.size()));
    EXPECT_EQ((*queryResult)[0]["content_type"].get<std::string>(), file.contentType);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileCreatesFileRowForOwner) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    constexpr int64_t ownerId = 42;
    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto result = nubilo::storeFile(db, blobStore, ownerId, file);
    ASSERT_TRUE(result.has_value());

    auto queryResult = db.query("SELECT owner_id, path FROM files WHERE id = ?;", {std::to_string(result.value())});
    ASSERT_TRUE(queryResult);
    ASSERT_EQ(queryResult->size(), 1);

    EXPECT_EQ((*queryResult)[0]["owner_id"].get<int64_t>(), ownerId);
    EXPECT_EQ((*queryResult)[0]["path"].get<std::string>(), file.path);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileWithIdenticalContentDoesNotDuplicateBlob) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File first{"a.txt", "same content", "text/plain"};
    nubilo::File second{"b.txt", "same content", "text/plain"};

    auto firstResult = nubilo::storeFile(db, blobStore, 1, first);
    auto secondResult = nubilo::storeFile(db, blobStore, 1, second);

    ASSERT_TRUE(firstResult.has_value());
    ASSERT_TRUE(secondResult.has_value());
    EXPECT_NE(firstResult.value(), secondResult.value());

    auto blobCount = db.query("SELECT COUNT(*) as count FROM blobs;");
    ASSERT_TRUE(blobCount);
    EXPECT_EQ((*blobCount)[0]["count"].get<int64_t>(), 1);

    auto fileCount = db.query("SELECT COUNT(*) as count FROM files;");
    ASSERT_TRUE(fileCount);
    EXPECT_EQ((*fileCount)[0]["count"].get<int64_t>(), 2);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, StoreFileWithIdenticalContentDifferentOwnersBothSucceed) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File fileA{"shared.txt", "shared content", "text/plain"};
    nubilo::File fileB{"shared.txt", "shared content", "text/plain"};

    auto resultA = nubilo::storeFile(db, blobStore, 1, fileA);
    auto resultB = nubilo::storeFile(db, blobStore, 2, fileB);

    ASSERT_TRUE(resultA.has_value());
    ASSERT_TRUE(resultB.has_value());
    EXPECT_NE(resultA.value(), resultB.value());

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, ReadFileReturnsStoredContent) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto storeResult = nubilo::storeFile(db, blobStore, 1, file);
    ASSERT_TRUE(storeResult.has_value());

    auto readResult = nubilo::readFile(db, blobStore, storeResult.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, file.content);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, ReadFileReturnsPathAndContentType) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File file{"docs/notes.txt", "hello, nubilo", "text/plain"};
    auto storeResult = nubilo::storeFile(db, blobStore, 1, file);
    ASSERT_TRUE(storeResult.has_value());

    auto readResult = nubilo::readFile(db, blobStore, storeResult.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->path, file.path);
    EXPECT_EQ(readResult->contentType, file.contentType);

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, ReadFileReturnsErrorForNonexistentId) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    auto readResult = nubilo::readFile(db, blobStore, 9999);
    EXPECT_FALSE(readResult.has_value());

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, ReadFileWorksForEachOwnerOfSharedContent) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File fileA{"a.txt", "shared content", "text/plain"};
    nubilo::File fileB{"b.txt", "shared content", "text/plain"};

    auto idA = nubilo::storeFile(db, blobStore, 1, fileA);
    auto idB = nubilo::storeFile(db, blobStore, 2, fileB);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());

    auto readA = nubilo::readFile(db, blobStore, idA.value());
    auto readB = nubilo::readFile(db, blobStore, idB.value());

    ASSERT_TRUE(readA.has_value());
    ASSERT_TRUE(readB.has_value());
    EXPECT_EQ(readA->content, "shared content");
    EXPECT_EQ(readB->content, "shared content");
    EXPECT_EQ(readA->path, "a.txt");
    EXPECT_EQ(readB->path, "b.txt");

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}

TEST(BlobServiceTest, ListFilesReturnsOnlyFilesOwnedByUser) {
    std::string dbPath = tempDbPath();
    nubilo::Db db = makeMigratedDb(dbPath);

    auto blobRootPath = tempBlobRoot();
    auto blobStoreResult = nubilo::BlobStore::open(blobRootPath);
    ASSERT_TRUE(blobStoreResult.has_value());
    nubilo::BlobStore blobStore = std::move(*blobStoreResult);

    nubilo::File fileA{"a.txt", "content a", "text/plain"};
    nubilo::File fileB{"b.txt", "content b", "text/plain"};
    nubilo::File fileC{"c.txt", "content c", "text/plain"};

    auto idA = nubilo::storeFile(db, blobStore, 1, fileA);
    auto idB = nubilo::storeFile(db, blobStore, 2, fileB);
    auto idC = nubilo::storeFile(db, blobStore, 1, fileC);
    ASSERT_TRUE(idA.has_value());
    ASSERT_TRUE(idB.has_value());
    ASSERT_TRUE(idC.has_value());

    auto listResult = nubilo::listFiles(db, 1);
    ASSERT_TRUE(listResult.has_value());
    ASSERT_EQ(listResult->size(), 2);

    EXPECT_EQ((*listResult)[0].id, idA.value());
    EXPECT_EQ((*listResult)[0].path, "a.txt");
    EXPECT_EQ((*listResult)[0].size, static_cast<int64_t>(fileA.content.size()));
    EXPECT_EQ((*listResult)[0].contentType, "text/plain");
    EXPECT_EQ((*listResult)[1].id, idC.value());
    EXPECT_EQ((*listResult)[1].path, "c.txt");

    std::filesystem::remove(dbPath);
    std::filesystem::remove_all(blobRootPath);
}