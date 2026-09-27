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
};

}