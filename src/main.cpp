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

#include <print>

/**
 * @brief Starts the Nubilo backend server.
 * @return 0 on normal exit.
 */
int main() {
    std::println("Nubilo backend {} starting...", nubilo::getVersion());

    nubilo::HttpServer server(8080);
    server.run();

    return 0;
}
