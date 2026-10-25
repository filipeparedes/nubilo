#include "blob/path.h"

#include <gtest/gtest.h>

#include <string>

TEST(UtPathValidate, AcceptsFileAndFolderPaths) {
    EXPECT_TRUE(nubilo::validatePath("notes.txt").has_value());
    EXPECT_TRUE(nubilo::validatePath("docs").has_value());
    EXPECT_TRUE(nubilo::validatePath("docs/notes.txt").has_value());
    EXPECT_TRUE(nubilo::validatePath("a/b/c/d.txt").has_value());
}

TEST(UtPathValidate, AcceptsDotsInsideNames) {
    EXPECT_TRUE(nubilo::validatePath("archive.tar.gz").has_value());
    EXPECT_TRUE(nubilo::validatePath(".gitignore").has_value());
    EXPECT_TRUE(nubilo::validatePath("a/.hidden").has_value());
    EXPECT_TRUE(nubilo::validatePath("a..b").has_value());
}

TEST(UtPathValidate, AcceptsSpacesAndNonAsciiCharacters) {
    EXPECT_TRUE(nubilo::validatePath("my notes (1).txt").has_value());
    EXPECT_TRUE(nubilo::validatePath("relat\xC3\xB3rio/ano 2026.pdf").has_value());
}

TEST(UtPathValidate, RejectsEmptyPath) {
    EXPECT_FALSE(nubilo::validatePath("").has_value());
}

TEST(UtPathValidate, RejectsLeadingOrTrailingSlash) {
    EXPECT_FALSE(nubilo::validatePath("/a").has_value());
    EXPECT_FALSE(nubilo::validatePath("a/").has_value());
    EXPECT_FALSE(nubilo::validatePath("/").has_value());
}

TEST(UtPathValidate, RejectsBackslash) {
    EXPECT_FALSE(nubilo::validatePath("a\\b").has_value());
}

TEST(UtPathValidate, RejectsEmptySegments) {
    EXPECT_FALSE(nubilo::validatePath("a//b").has_value());
}

TEST(UtPathValidate, RejectsDotAndDotDotSegments) {
    EXPECT_FALSE(nubilo::validatePath(".").has_value());
    EXPECT_FALSE(nubilo::validatePath("..").has_value());
    EXPECT_FALSE(nubilo::validatePath("a/./b").has_value());
    EXPECT_FALSE(nubilo::validatePath("a/../b").has_value());
    EXPECT_FALSE(nubilo::validatePath("../a").has_value());
    EXPECT_FALSE(nubilo::validatePath("a/..").has_value());
}

TEST(UtPathValidate, RejectsControlCharacters) {
    EXPECT_FALSE(nubilo::validatePath("a\nb").has_value());
    EXPECT_FALSE(nubilo::validatePath("a\tb").has_value());
    EXPECT_FALSE(nubilo::validatePath(std::string("a\0b", 3)).has_value());
    EXPECT_FALSE(nubilo::validatePath(std::string("a") + '\x7F' + "b").has_value());
}

TEST(UtPathValidate, RejectsPathsLongerThanTheLimit) {
    EXPECT_TRUE(nubilo::validatePath(std::string(1024, 'a')).has_value());
    EXPECT_FALSE(nubilo::validatePath(std::string(1025, 'a')).has_value());
}

TEST(UtPathValidate, ExplainsWhyAPathIsRejected) {
    auto result = nubilo::validatePath("a//b");

    ASSERT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().message.empty());
}