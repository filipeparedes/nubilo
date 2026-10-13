#pragma once

#include "storage/DbMigrator.h"

#include <vector>

namespace nubilo {

/**
 * @brief The full list of schema migrations, in version order.
 *
 * New migrations are always appended to the end. never edit or remove an entry once it has run in any environment,
 * since schema_migrations tracks applied versions by number.
 */
inline const std::vector<DbMigration> migrations = {
    {1,
     "CREATE TABLE users ("
     "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
     "  email TEXT NOT NULL UNIQUE"
     ");"},
    {2,
     "CREATE TABLE files ("
     "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
     "  owner_id INTEGER NOT NULL,"
     "  path TEXT NOT NULL,"
     "  FOREIGN KEY (owner_id) REFERENCES users(id)"
     ");"},
    {
    3,
    "ALTER TABLE users ADD COLUMN password_hash TEXT NOT NULL DEFAULT '';"
    },
    {4,
        "CREATE TABLE sessions ("
         "  token TEXT PRIMARY KEY,"
         "  user_id INTEGER NOT NULL,"
         "  created_at TEXT NOT NULL DEFAULT (datetime('now')),"
         "  FOREIGN KEY (user_id) REFERENCES users(id)"
         ");"
    },
    {5,
    "CREATE TABLE file_metadata ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  file_id INTEGER NOT NULL UNIQUE,"
    "  size INTEGER NOT NULL DEFAULT 0,"
    "  content_type TEXT,"
    "  created_at TEXT NOT NULL DEFAULT (datetime('now')),"
    "  updated_at TEXT NOT NULL DEFAULT (datetime('now')),"
    "  FOREIGN KEY (file_id) REFERENCES files(id)"
    ");"
    },
    {6,
    "CREATE TABLE sync_state ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  file_id INTEGER NOT NULL UNIQUE,"
    "  last_synced_at TEXT,"
    "  version INTEGER NOT NULL DEFAULT 1,"
    "  FOREIGN KEY (file_id) REFERENCES files(id)"
    ");"
    },
    {7,
    "DROP TABLE file_metadata;"
    "CREATE TABLE blobs ("
    "  content_hash TEXT PRIMARY KEY,"
    "  size INTEGER NOT NULL,"
    "  content_type TEXT"
    ");"
    "ALTER TABLE files ADD COLUMN content_hash TEXT NOT NULL DEFAULT '' REFERENCES blobs(content_hash);"
    "ALTER TABLE files ADD COLUMN created_at TEXT NOT NULL DEFAULT (datetime('now'));"
    "ALTER TABLE files ADD COLUMN updated_at TEXT NOT NULL DEFAULT (datetime('now'));"
    },
{8,
"ALTER TABLE sessions ADD COLUMN expires_at TEXT NOT NULL DEFAULT '';"
    },
    {9,
    "CREATE TABLE file_chunks ("
    "  file_id INTEGER NOT NULL,"
    "  position INTEGER NOT NULL,"
    "  chunk_hash TEXT NOT NULL,"
    "  PRIMARY KEY (file_id, position),"
    "  FOREIGN KEY (file_id) REFERENCES files(id),"
    "  FOREIGN KEY (chunk_hash) REFERENCES blobs(content_hash)"
    ");"
    "CREATE INDEX idx_file_chunks_chunk_hash ON file_chunks(chunk_hash);"
    "INSERT INTO file_chunks (file_id, position, chunk_hash)"
    "  SELECT id, 0, content_hash FROM files WHERE content_hash != '';"
    },
    {10,
    "CREATE TABLE files_new ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  owner_id INTEGER NOT NULL,"
    "  path TEXT NOT NULL,"
    "  content_type TEXT,"
    "  created_at TEXT NOT NULL DEFAULT (datetime('now')),"
    "  updated_at TEXT NOT NULL DEFAULT (datetime('now')),"
    "  FOREIGN KEY (owner_id) REFERENCES users(id)"
    ");"
    "INSERT INTO files_new (id, owner_id, path, content_type, created_at, updated_at)"
    "  SELECT f.id, f.owner_id, f.path, b.content_type, f.created_at, f.updated_at"
    "  FROM files f LEFT JOIN blobs b ON b.content_hash = f.content_hash;"
    "UPDATE sqlite_sequence SET seq = (SELECT seq FROM sqlite_sequence WHERE name = 'files')"
    "  WHERE name = 'files_new';"
    "DROP TABLE files;"
    "ALTER TABLE files_new RENAME TO files;"
    "ALTER TABLE blobs DROP COLUMN content_type;"
    },
    {11,
    "CREATE TABLE file_versions ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  file_id INTEGER NOT NULL,"
    "  version INTEGER NOT NULL,"
    "  size INTEGER NOT NULL,"
    "  content_type TEXT,"
    "  created_at TEXT NOT NULL DEFAULT (datetime('now')),"
    "  UNIQUE (file_id, version),"
    "  FOREIGN KEY (file_id) REFERENCES files(id)"
    ");"
    },
    {12,
    //every existing file becomes version 1
    "INSERT INTO file_versions (file_id, version, size, content_type, created_at)"
    "  SELECT f.id, 1,"
    "    COALESCE((SELECT SUM(b.size) FROM file_chunks fc"
    "              JOIN blobs b ON b.content_hash = fc.chunk_hash"
    "              WHERE fc.file_id = f.id), 0),"
    "    f.content_type, f.created_at"
    "  FROM files f;"
    },
    {13,
    //the manifest now belongs to a version
    "CREATE TABLE file_chunks_new ("
    "  version_id INTEGER NOT NULL,"
    "  position INTEGER NOT NULL,"
    "  chunk_hash TEXT NOT NULL,"
    "  PRIMARY KEY (version_id, position),"
    "  FOREIGN KEY (version_id) REFERENCES file_versions(id),"
    "  FOREIGN KEY (chunk_hash) REFERENCES blobs(content_hash)"
    ");"
    "INSERT INTO file_chunks_new (version_id, position, chunk_hash)"
    "  SELECT v.id, fc.position, fc.chunk_hash"
    "  FROM file_chunks fc JOIN file_versions v ON v.file_id = fc.file_id AND v.version = 1;"
    "DROP TABLE file_chunks;"
    "ALTER TABLE file_chunks_new RENAME TO file_chunks;"
    "CREATE INDEX idx_file_chunks_chunk_hash ON file_chunks(chunk_hash);"
    },
    };

}