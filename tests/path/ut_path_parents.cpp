#include "blob/path.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(UtPathParents, ListsFoldersOutermostFirst) {
    EXPECT_EQ(nubilo::parentPaths("a/b/c.txt"), (std::vector<std::string>{"a", "a/b"}));
    EXPECT_EQ(nubilo::parentPaths("a/b/c/d"), (std::vector<std::string>{"a", "a/b", "a/b/c"}));
}

TEST(UtPathParents, OfAFolderDoesNotIncludeItself) {
    EXPECT_EQ(nubilo::parentPaths("a/b"), (std::vector<std::string>{"a"}));
}

TEST(UtPathParents, IsEmptyForAPathWithoutSlashes) {
    EXPECT_TRUE(nubilo::parentPaths("notes.txt").empty());
}