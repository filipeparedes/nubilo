module;
/**
 * @file HttpServer.cppm
 * @brief HTTP server setup and routing for the Nubilo api.
 *
 * @author Filipe Paredes (filipeparedes3@gmail.com)
 *
 * @copyright Copyright (c) 2026
 */

#include <httplib.h>
#include <print>

export module HttpServer;

export namespace nubilo {
/**
 * @class HttpServer
 * @brief Wraps a cpp-httplib server, exposing the API's HTTP endpoints.
 *
 * owns the underlying httplib::Server instance and registers all routes on construction.
 * Call run() to start listening for requests (blocking).
 */
class HttpServer {
public:
    /**
     * @brief Constructs the server and registers all routes.
     * @param port The TCP port to listen on once run() is called.
     */
    explicit HttpServer(int port) : port_(port) {
        setupRoutes();
    }

    /**
     * @brief Starts the server and blocks, listening for incoming requests.
     */
    void run() {
        std::println("Nubilo server listening on port {}...", port_);
        server_.listen("0.0.0.0", port_);
    }

private:
    /**
     * @brief Registers all HTTP routes on the underlying server;
     */
    void setupRoutes() {
        server_.Get("/health", [](const httplib::Request& req, httplib::Response& res) {
            res.set_content(R"({"status":"ok"})", "application/json");
        });
    }

    httplib::Server server_;
    int port_;
};

}