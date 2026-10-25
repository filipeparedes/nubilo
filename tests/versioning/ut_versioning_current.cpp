#include "../common/BlobFixture.h"

class UtVersioningCurrent : public BlobFixture {};

TEST_F(UtVersioningCurrent, ReadFileReturnsLatestVersion) {
    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "{\"v\": 2}", "application/json"};
    auto id = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    auto readResult = nubilo::readFile(*db_, *blobStore_, id.value());
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, second.content);
    EXPECT_EQ(readResult->contentType, "application/json");
}

TEST_F(UtVersioningCurrent, ListFilesShowsLatestVersionOnce) {
    nubilo::File first{"notes.txt", "short", "text/plain"};
    nubilo::File second{"notes.txt", "a much longer second version", "text/markdown"};
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, first).has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    auto listResult = nubilo::listFiles(*db_, 1);
    ASSERT_TRUE(listResult.has_value());
    ASSERT_EQ(listResult->size(), 1);
    EXPECT_EQ((*listResult)[0].size, static_cast<int64_t>(second.content.size()));
    EXPECT_EQ((*listResult)[0].contentType, "text/markdown");
}

TEST_F(UtVersioningCurrent, DeleteFileRemovesAllVersionsAndManifests) {
    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "second version", "text/plain"};
    auto id = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    ASSERT_TRUE(nubilo::deleteFile(*db_, id.value()).has_value());

    for (const char* table : {"files", "file_versions", "file_chunks"}) {
        auto count = db_->query(std::string("SELECT COUNT(*) AS count FROM ") + table + ";");
        ASSERT_TRUE(count);
        EXPECT_EQ((*count)[0]["count"].get<int64_t>(), 0) << table;
    }

    //the chunks are kept for the future garbage collection
    auto blobs = db_->query("SELECT COUNT(*) AS count FROM blobs;");
    ASSERT_TRUE(blobs);
    EXPECT_EQ((*blobs)[0]["count"].get<int64_t>(), 2);
}