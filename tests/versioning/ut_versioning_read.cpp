//
// Created by Filipe Paredes on 25/10/2026.
//
#include "../common/BlobFixture.h"

class UtVersioningRead : public BlobFixture {};

TEST_F(UtVersioningRead, ReadsAnOlderVersion) {
    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "second version", "text/plain"};
    auto id = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    auto readResult = nubilo::readFile(*db_, *blobStore_, id.value(), 1);

    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, first.content);
}

TEST_F(UtVersioningRead, ReadsTheLatestVersionWhenRequestedByNumber) {
    nubilo::File first{"notes.txt", "first version", "text/plain"};
    nubilo::File second{"notes.txt", "second version", "text/plain"};
    auto id = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    auto readResult = nubilo::readFile(*db_, *blobStore_, id.value(), 2);

    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->content, second.content);
}

TEST_F(UtVersioningRead, ReturnsTheContentTypeOfThatVersion) {
    nubilo::File first{"notes.txt", "short", "text/plain"};
    nubilo::File second{"notes.txt", "{\"v\": 2}", "application/json"};
    auto id = nubilo::storeFile(*db_, *blobStore_, 1, first);
    ASSERT_TRUE(id.has_value());
    ASSERT_TRUE(nubilo::storeFile(*db_, *blobStore_, 1, second).has_value());

    auto readResult = nubilo::readFile(*db_, *blobStore_, id.value(), 1);

    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult->contentType, "text/plain");
}

TEST_F(UtVersioningRead, ReturnsErrorForNonexistentVersion) {
    nubilo::File file{"notes.txt", "hello, nubilo", "text/plain"};
    auto id = nubilo::storeFile(*db_, *blobStore_, 1, file);
    ASSERT_TRUE(id.has_value());

    EXPECT_FALSE(nubilo::readFile(*db_, *blobStore_, id.value(), 99).has_value());
}