#include "api/HttpServer.h"
#include <print>

namespace nubilo {

HttpServer::HttpServer(int port, const std::string& host) : port_(port), host_(host) {}

httplib::Server& HttpServer::getServer() {
    return server_;
}

void HttpServer::run() {
    std::println("Nubilo server listening on {}:{}...", host_, port_);
    server_.listen(host_, port_);
}

void HttpServer::stop() {
    server_.stop();
}

void HttpServer::enableCors(const std::string& allowedOrigin) {
    //answers the preflight request the browser sends before a request with an Authorization header
    server_.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    //adds the headers to every response, but only for the allowed origin
    server_.set_post_routing_handler([allowedOrigin](const httplib::Request& req, httplib::Response& res) {
        if (req.get_header_value("Origin") != allowedOrigin)
            return;

        res.set_header("Access-Control-Allow-Origin", allowedOrigin);
        res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Authorization, Content-Type");
        res.set_header("Access-Control-Expose-Headers", "Content-Disposition");
        res.set_header("Access-Control-Max-Age", "600");
    });
}

};
