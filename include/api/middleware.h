#pragma once

#include "api/Router.h"

namespace nubilo {

class SqliteDb;

/**
 * Middleware wrapper for RouteCallback in cases where the route
 * requires a valid user session.
 * @param db The database where the session is stored
 * @param handler The original handler to call
 * @return A RouteCallback, with the verified session
 */
RouteCallback requireAuth(SqliteDb& db, AuthRouteCallback handler);

}