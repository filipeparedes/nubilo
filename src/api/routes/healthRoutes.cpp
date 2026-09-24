#include "api/routes/healthRoutes.h"
#include "api/Router.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

namespace nubilo {

void registerHealthRoutes(Router& router) {
    router.addRoute("GET", "/health", [](const httplib::Request&, httplib::Response& res) {
        nlohmann::json body = {{"status", "ok"}};
        res.set_content(body.dump(), "application/json");
    });
}

}