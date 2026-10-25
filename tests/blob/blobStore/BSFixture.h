#pragma once

#include "../../common/testUtils.h"

#include "blob/BlobStore.h"

#include <filesystem>
#include <gtest/gtest.h>

// Gives each test a clean temporary blob directory (root_), removed when the test ends.
class BSFixture : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = nubilo::test::tempBlobRoot();
    }

    void TearDown() override {
        std::filesystem::remove_all(root_);
    }

    std::filesystem::path root_;
};