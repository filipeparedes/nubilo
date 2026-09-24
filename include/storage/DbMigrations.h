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
};

}