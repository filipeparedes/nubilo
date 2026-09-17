/**
 * @file main.cpp
 * @brief Entry point for the Nubilo backend.
 *
 * @author Filipe Paredes (filipeparedes3@gmail.com)
 *
 * @copyright Copyright (c) 2026
 */

import version;
import HttpServer;
import Router;

#include "httplib.h"

#include <print>

/**
 * @brief Starts the Nubilo backend server.
 * @return 0 on normal exit.
 */
int main() {
    std::println("Nubilo backend {} starting...", nubilo::getVersion());

    nubilo::HttpServer server(8080);
    nubilo::Router router;

    router.addRoute("GET", "/health", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(R"({"status"}:"ok")", "application/json");
    });

    router.applyTo(server.getServer());
    server.run();

    return 0;
}
