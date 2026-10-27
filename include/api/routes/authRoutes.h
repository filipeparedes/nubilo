#pragma once

namespace nubilo {

class Router;
class Db;

/**
 * @brief Registers the Authentication routes on the given router.
 * @param router The router to register the route on.
 * @param db A reference to the Sqlite DB
 * @param sessionDays How many days the new session lasts, default is 14
 */
void registerAuthRoutes(Router& router, Db& db, int sessionDays = 14);

}