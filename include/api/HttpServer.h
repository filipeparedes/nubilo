#pragma once

#include <httplib.h>

namespace nubilo {

/**
 * @class HttpServer
 * @brief Wraps a cpp-httplib server.
 * Call run() to start listening for requests (blocking).
 *
 * Route registration happens externally via Router,
 * this class only owns the server and controls its lifecycle.
 */
class HttpServer {
public:
    explicit HttpServer(int port);

    /**
     * @brief Exposes the underlying httplib server, for Router::applyTo();
     * @return A reference to the underlying httplib::Server
     */
    httplib::Server& getServer();

    /**
     * @brief Starts the server and blocks, listening for incoming requests.
     */
    void run();

    /**
     * @brief Stops the server, unblocking a prior call to run().
     */
    void stop();

private:
    httplib::Server server_;
    int port_;
};

}