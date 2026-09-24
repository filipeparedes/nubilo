#pragma once

namespace nubilo {

class Router;

/**
 * @brief Registers the GET /health route on the given router.
 * @param router The router to register the route on.
 */
void registerHealthRoutes(Router& router);

}
