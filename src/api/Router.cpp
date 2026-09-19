#include <print>
#include "api/Router.h"

namespace nubilo {

void Router::addRoute(std::string method, std::string path, RouteCallback callback) {
    // Note to self: passing by value & moving is better than passing by reference since it avoids copies on writing to routes_
    // Not to mistake with passing by values on reading, which is a lot worse
    routes_.push_back({std::move(method), std::move(path), std::move(callback)});
}

void Router::applyTo(httplib::Server& server) const {
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

}
