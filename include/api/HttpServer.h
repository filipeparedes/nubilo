#pragma once

#include <httplib.h>
#include <string>

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
    explicit HttpServer(int port, const std::string& host = "0.0.0.0");

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

    /**
     * @brief Allows a browser page served from the given origin to call the API (CORS).
     * Answers the preflight OPTIONS requests and adds the CORS headers to the responses to that origin.
     * Must be called before run().
     * @param allowedOrigin The origin to allow, e.g. "http://localhost:5173"
     */
    void enableCors(const std::string& allowedOrigin);

private:
    httplib::Server server_;
    int port_;
    std::string host_;
};

}