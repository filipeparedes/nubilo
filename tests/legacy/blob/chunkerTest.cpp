#include "blob/chunker.h"

#include <gtest/gtest.h>

#include <string>

namespace nubilo {

namespace {

std::string join(const std::vector<std::string_view>& chunks) {
    std::string result;
    for (auto chunk : chunks)
        result += chunk;
    return result;
}

}

TEST(ChunkerTest, EmptyContentProducesNoChunks) {
    auto result = chunkContent("", 4);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST(ChunkerTest, ContentSmallerThanChunkSizeProducesOneChunk) {
    auto result = chunkContent("abc", 4);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1);
    EXPECT_EQ((*result)[0], "abc");
}

TEST(ChunkerTest, ContentEqualToChunkSizeProducesOneChunk) {
    auto result = chunkContent("abcd", 4);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1);
    EXPECT_EQ((*result)[0], "abcd");
}

TEST(ChunkerTest, ExactMultipleProducesOnlyFullChunks) {
    auto result = chunkContent("ABCDEFGH", 4);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 2);
    EXPECT_EQ((*result)[0], "ABCD");
    EXPECT_EQ((*result)[1], "EFGH");
}

TEST(ChunkerTest, LastChunkHoldsTheRemainder) {
    auto result = chunkContent("ABCDEFGHIJ", 4);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3);
    EXPECT_EQ((*result)[0], "ABCD");
    EXPECT_EQ((*result)[1], "EFGH");
    EXPECT_EQ((*result)[2], "IJ");
}

TEST(ChunkerTest, JoiningChunksReturnsOriginalBinaryContent) {
    //explicit length, a string literal would truncate at the embedded \0
    const std::string content("ab\0cd\0ef\0gh", 11);
    auto result = chunkContent(content, 3);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(join(result.value()), content);
}

TEST(ChunkerTest, ChunksPointIntoTheOriginalContent) {
    const std::string content = "ABCDEFGHIJ";
    auto result = chunkContent(content, 4);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ((*result)[0].data(), content.data());
    EXPECT_EQ((*result)[1].data(), content.data() + 4);
}

TEST(ChunkerTest, ZeroChunkSizeReturnsError) {
    auto result = chunkContent("abc", 0);

    EXPECT_FALSE(result.has_value());
}

}