#pragma once

namespace nubilo {

class Router;
class SqliteDb;

/**
 * @brief Registers the Authentication routes on the given router.
 * @param router The router to register the route on.
 * @param db A reference to the Sqlite DB
 */
void registerAuthRoutes(Router& router, SqliteDb& db);

}