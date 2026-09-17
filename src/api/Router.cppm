module;
/**
 * @file Router.cppm
 * @brief Central registry of HTTP routes, applied to an httplib::Server.
 *
 * @author Filipe Paredes (filipeparedes3@gmail.com)
 *
 * @copyright Copyright (c) 2026
 */

#include <httplib.h>
#include <functional>
#include <print>
#include <string>
#include <vector>

export module Router;

/**
 * @class Router
 * @brief Central registry of HTTP routes, applied to an httplib::Server.
 *
 * Modules register their routes here instead of touching httplib::Server directly, keeping them decoupled
 * from the underlying HTTP library and giving a single place to add cross-cutting behavior (e.g. logging).
 */

export namespace nubilo {

using RouteCallback = std::function<void(const httplib::Request&, httplib::Response&)>;

struct Route {
    std::string method;
    std::string path;
    RouteCallback callback;
};

class Router {
public:
    /**
     * @brief Registers a route to be applied later via applyTo().
     * @param method HTTP method, e.g "GET", "POST"
     * @param path Route path, e.g. "/health"
     * @param callback Function called when a request matches method+path
     */
    void addRoute(std::string method, std::string path, RouteCallback callback) {
        // Note to self: passing by value & moving is better than passing by reference since it avoids copies on writing to routes_
        // Not to mistake with passing by values on reading, which is a lot worse
        routes_.push_back({std::move(method), std::move(path), std::move(callback)});
    }

    /**
     * @brief Applies all registered routes to the given httplib server.
     * @param server The server to register routes on.
     */
    void applyTo(httplib::Server& server) const {
        for (const auto& route : routes_) {
            std::println("Registering {} {}", route.method, route.path);

            if (route.method == "GET")
                server.Get(route.path, route.callback);
            else if (route.method == "POST")
                server.Post(route.path, route.callback);
            else if (route.method == "PUT")
                server.Put(route.path, route.callback);
            else if (route.method == "DELETE")
                server.Delete(route.path, route.callback);
            else
                std::println(stderr, "Unsupported HTTP method: {}", route.method);
        }
    }

private:
    std::vector<Route> routes_;
};

}
