#include "blob/path.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(PathTest, ValidatePathAcceptsFileAndFolderPaths) {
    EXPECT_TRUE(nubilo::validatePath("notes.txt").has_value());
    EXPECT_TRUE(nubilo::validatePath("docs").has_value());
    EXPECT_TRUE(nubilo::validatePath("docs/notes.txt").has_value());
    EXPECT_TRUE(nubilo::validatePath("a/b/c/d.txt").has_value());
}

TEST(PathTest, ValidatePathAcceptsDotsInsideNames) {
    EXPECT_TRUE(nubilo::validatePath("archive.tar.gz").has_value());
    EXPECT_TRUE(nubilo::validatePath(".gitignore").has_value());
    EXPECT_TRUE(nubilo::validatePath("a/.hidden").has_value());
    EXPECT_TRUE(nubilo::validatePath("a..b").has_value());
}

TEST(PathTest, ValidatePathAcceptsSpacesAndNonAsciiCharacters) {
    EXPECT_TRUE(nubilo::validatePath("my notes (1).txt").has_value());
    EXPECT_TRUE(nubilo::validatePath("relat\xC3\xB3rio/ano 2026.pdf").has_value());
}

TEST(PathTest, ValidatePathRejectsEmptyPath) {
    EXPECT_FALSE(nubilo::validatePath("").has_value());
}

TEST(PathTest, ValidatePathRejectsLeadingOrTrailingSlash) {
    EXPECT_FALSE(nubilo::validatePath("/a").has_value());
    EXPECT_FALSE(nubilo::validatePath("a/").has_value());
    EXPECT_FALSE(nubilo::validatePath("/").has_value());
}

TEST(PathTest, ValidatePathRejectsBackslash) {
    EXPECT_FALSE(nubilo::validatePath("a\\b").has_value());
}

TEST(PathTest, ValidatePathRejectsEmptySegments) {
    EXPECT_FALSE(nubilo::validatePath("a//b").has_value());
}

TEST(PathTest, ValidatePathRejectsDotAndDotDotSegments) {
    EXPECT_FALSE(nubilo::validatePath(".").has_value());
    EXPECT_FALSE(nubilo::validatePath("..").has_value());
    EXPECT_FALSE(nubilo::validatePath("a/./b").has_value());
    EXPECT_FALSE(nubilo::validatePath("a/../b").has_value());
    EXPECT_FALSE(nubilo::validatePath("../a").has_value());
    EXPECT_FALSE(nubilo::validatePath("a/..").has_value());
}

TEST(PathTest, ValidatePathRejectsControlCharacters) {
    EXPECT_FALSE(nubilo::validatePath("a\nb").has_value());
    EXPECT_FALSE(nubilo::validatePath("a\tb").has_value());
    EXPECT_FALSE(nubilo::validatePath(std::string("a\0b", 3)).has_value());
    EXPECT_FALSE(nubilo::validatePath(std::string("a") + '\x7F' + "b").has_value());
}

TEST(PathTest, ValidatePathRejectsPathsLongerThanTheLimit) {
    EXPECT_TRUE(nubilo::validatePath(std::string(1024, 'a')).has_value());
    EXPECT_FALSE(nubilo::validatePath(std::string(1025, 'a')).has_value());
}

TEST(PathTest, ValidatePathExplainsWhyAPathIsRejected) {
    auto result = nubilo::validatePath("a//b");

    ASSERT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().message.empty());
}

TEST(PathTest, ParentPathsListsFoldersOutermostFirst) {
    EXPECT_EQ(nubilo::parentPaths("a/b/c.txt"), (std::vector<std::string>{"a", "a/b"}));
    EXPECT_EQ(nubilo::parentPaths("a/b/c/d"), (std::vector<std::string>{"a", "a/b", "a/b/c"}));
}

TEST(PathTest, ParentPathsOfAFolderDoesNotIncludeItself) {
    EXPECT_EQ(nubilo::parentPaths("a/b"), (std::vector<std::string>{"a"}));
}

TEST(PathTest, ParentPathsIsEmptyForAPathWithoutSlashes) {
    EXPECT_TRUE(nubilo::parentPaths("notes.txt").empty());
}