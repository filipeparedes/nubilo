#pragma once

#include <httplib.h>
#include <functional>
#include <string>
#include <vector>

namespace nubilo {

// signature every route callback/handler must match
using RouteCallback = std::function<void(const httplib::Request&, httplib::Response&)>;

//for routes that require authentication
//assume that the token is valid
using AuthRouteCallback = std::function<void(const httplib::Request&, httplib::Response&, const std::string& token)>;

struct Route {
    std::string method;
    std::string path;
    RouteCallback callback;
};

/**
 * @class Router
 * @brief Central registry of HTTP routes, applied to an httplib::Server.
 *
 * Modules register their routes here instead of touching httplib::Server directly, keeping them decoupled
 * from the underlying HTTP library and giving a single place to add cross-cutting behavior (e.g. logging).
 */
class Router {
public:
    /**
     * @brief Registers a route to be applied later via applyTo().
     * @param method HTTP method, e.g "GET", "POST"
     * @param path Route path, e.g. "/health"
     * @param callback Function called when a request matches method+path
     */
    void addRoute(std::string method, std::string path, RouteCallback callback);

    /**
     * @brief Applies all registered routes to the given httplib server.
     * @param server The server to register routes on.
     */
    void applyTo(httplib::Server& server) const;

private:
    std::vector<Route> routes_;
};

}