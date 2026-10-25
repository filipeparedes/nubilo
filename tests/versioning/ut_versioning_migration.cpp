#include "../common/testUtils.h"

#include "storage/Db.h"
#include "storage/DbMigrations.h"
#include "storage/DbMigrator.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(UtVersioningMigration, MigrationsKeepExistingFilesAsVersionOne) {
    std::string dbPath = nubilo::test::tempDbPath();
    auto dbResult = nubilo::Db::open(dbPath);
    ASSERT_TRUE(dbResult.has_value());
    nubilo::Db db = std::move(*dbResult);

    //schema as it was before the versions
    std::vector<nubilo::DbMigration> beforeVersions;
    for (const auto& migration : nubilo::migrations)
        if (migration.version <= 10)
            beforeVersions.push_back(migration);

    nubilo::DbMigrator migrator(db);
    ASSERT_TRUE(migrator.run(beforeVersions));

    ASSERT_TRUE(db.exec("INSERT INTO blobs (content_hash, size) VALUES ('h1', 10);"));
    ASSERT_TRUE(db.exec("INSERT INTO blobs (content_hash, size) VALUES ('h2', 5);"));
    ASSERT_TRUE(db.exec("INSERT INTO files (owner_id, path, content_type) VALUES (1, 'a.txt', 'text/plain');"));
    ASSERT_TRUE(db.exec("INSERT INTO files (owner_id, path, content_type) VALUES (1, 'b.pdf', 'application/pdf');"));
    ASSERT_TRUE(db.exec("INSERT INTO file_chunks (file_id, position, chunk_hash) VALUES (1, 0, 'h1');"));
    ASSERT_TRUE(db.exec("INSERT INTO file_chunks (file_id, position, chunk_hash) VALUES (1, 1, 'h2');"));
    ASSERT_TRUE(db.exec("INSERT INTO file_chunks (file_id, position, chunk_hash) VALUES (2, 0, 'h1');"));

    ASSERT_TRUE(migrator.run(nubilo::migrations));

    //each file became version 1, with the size summed from its chunks
    auto versions = db.query("SELECT file_id, version, size, content_type FROM file_versions ORDER BY file_id;");
    ASSERT_TRUE(versions);
    ASSERT_EQ(versions->size(), 2);
    EXPECT_EQ((*versions)[0]["file_id"].get<int64_t>(), 1);
    EXPECT_EQ((*versions)[0]["version"].get<int64_t>(), 1);
    EXPECT_EQ((*versions)[0]["size"].get<int64_t>(), 15);
    EXPECT_EQ((*versions)[0]["content_type"].get<std::string>(), "text/plain");
    EXPECT_EQ((*versions)[1]["size"].get<int64_t>(), 10);
    EXPECT_EQ((*versions)[1]["content_type"].get<std::string>(), "application/pdf");

    //the manifest moved to the versions, in order
    auto chunks = db.query(
        "SELECT file_versions.file_id, file_chunks.position, file_chunks.chunk_hash FROM file_chunks "
        "JOIN file_versions ON file_versions.id = file_chunks.version_id "
        "ORDER BY file_versions.file_id, file_chunks.position;");
    ASSERT_TRUE(chunks);
    ASSERT_EQ(chunks->size(), 3);
    EXPECT_EQ((*chunks)[1]["chunk_hash"].get<std::string>(), "h2");
    EXPECT_EQ((*chunks)[2]["file_id"].get<int64_t>(), 2);

    //files kept their ids, lost content_type, and new ids continue the sequence
    EXPECT_FALSE(db.query("SELECT content_type FROM files;"));
    ASSERT_TRUE(db.exec("INSERT INTO files (owner_id, path) VALUES (1, 'c.txt');"));
    EXPECT_EQ(db.lastInsertId(), 3);

    std::filesystem::remove(dbPath);
}