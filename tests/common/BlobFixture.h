#pragma once

#include "testUtils.h"

#include "blob/BlobStore.h"
#include "blob/blobService.h"
#include "storage/Db.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

// Gives each test a migrated temporary database (db_) and a blob store on a temporary directory (blobStore_).
// Shared by the blobService tests: files, versioning and folders.
class BlobFixture : public ::testing::Test {
protected:
    void SetUp() override {
        dbPath_ = nubilo::test::tempDbPath();
        db_ = std::make_unique<nubilo::Db>(nubilo::test::makeMigratedDb(dbPath_));

        blobRoot_ = nubilo::test::tempBlobRoot();
        auto store = nubilo::BlobStore::open(blobRoot_);
        ASSERT_TRUE(store.has_value());
        blobStore_ = std::make_unique<nubilo::BlobStore>(std::move(*store));
    }

    void TearDown() override {
        db_.reset();
        std::filesystem::remove(dbPath_);
        std::filesystem::remove_all(blobRoot_);
    }

    // stores a small text file, so the tests can build a tree of files quickly
    void storeText(int64_t ownerId, const std::string& path, const std::string& content = "x") {
        nubilo::File file{path, content, "text/plain"};
        ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, ownerId, file).has_value());
    }

    // the contents of a folder as "folder:<path>" and "file:<path>" strings
    static std::vector<std::string> paths(const nubilo::FolderContents& contents) {
        std::vector<std::string> result;
        for (const auto& folder : contents.folders)
            result.push_back("folder:" + folder.path);
        for (const auto& file : contents.files)
            result.push_back("file:" + file.path);
        return result;
    }

    std::string dbPath_;
    std::filesystem::path blobRoot_;
    std::unique_ptr<nubilo::Db> db_;
    std::unique_ptr<nubilo::BlobStore> blobStore_;
};