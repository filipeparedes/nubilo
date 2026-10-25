#pragma once

#include "storage/Db.h"
#include "storage/DbMigrations.h"
#include "storage/DbMigrator.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <string>

namespace nubilo::test {

//must match DEFAULT_CHUNK_SIZE in blobService.cpp
inline constexpr size_t TEST_CHUNK_SIZE = 4 * 1024 * 1024;

// A name for temporary files that is unique to the running test, so tests that run in parallel never share files.
inline std::string uniqueTestName() {
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    return std::string("nubilo_") + info->test_suite_name() + "_" + info->name();
}

// The path of a temporary database for the running test. Any file already there is removed.
inline std::string tempDbPath() {
    auto path = std::filesystem::temp_directory_path() / (uniqueTestName() + ".db");
    std::filesystem::remove(path);
    return path.string();
}

// The path of a temporary blob directory for the running test. Any directory already there is removed.
inline std::filesystem::path tempBlobRoot() {
    auto path = std::filesystem::temp_directory_path() / (uniqueTestName() + "_blobs");
    std::filesystem::remove_all(path);
    return path;
}

// Opens a database at the given path and applies every migration.
inline Db makeMigratedDb(const std::string& path) {
    auto dbResult = Db::open(path);
    Db db = std::move(*dbResult);

    DbMigrator migrator(db);
    migrator.run(migrations);

    return db;
}

}
