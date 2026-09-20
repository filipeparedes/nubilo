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
inline const std::vector<Migration> migrations = {
    // TODO: Populate
};

}