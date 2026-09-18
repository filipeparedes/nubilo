module;
/**
 * @file HealthRoutes.cppm
 * @brief Registers the /health endpoint, used to confirm the server is up.
 *
 * @author Filipe Paredes (filipeparedes3@gmail.com)
 */
#include <httplib.h>

export module HealthRoutes;

import Router;

export namespace nubilo {

/**
 * @brief Registers the GET /health route on the given router.
 * @param router The router to register the route on.
 */
void registerHealthRoutes(Router& router) {
    router.addRoute("GET", "/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(R"({"status":"ok"})", "application/json");
    });
}

}