#include <version.h>

#include <gtest/gtest.h>

TEST(VersionTest, ReturnsExpectedVersion) {
    EXPECT_EQ(nubilo::getVersion(), "0.0.1-dev");
}