#pragma once

#include "../common/testUtils.h"

#include "storage/Db.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <string>

// Gives each test an empty, unmigrated temporary database (db_).
class DbFixture : public ::testing::Test {
protected:
    void SetUp() override {
        dbPath_ = nubilo::test::tempDbPath();
        auto result = nubilo::Db::open(dbPath_);
        ASSERT_TRUE(result.has_value());
        db_ = std::make_unique<nubilo::Db>(std::move(*result));
    }

    void TearDown() override {
        db_.reset();
        std::filesystem::remove(dbPath_);
    }

    std::string dbPath_;
    std::unique_ptr<nubilo::Db> db_;
};