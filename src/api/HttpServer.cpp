#include "api/HttpServer.h"
#include <print>

namespace nubilo {

HttpServer::HttpServer(int port) : port_(port) {}

httplib::Server& HttpServer::getServer() {
    return server_;
}

void HttpServer::run() {
    std::println("Nubilo server listening on port {}...", port_);
    server_.listen("0.0.0.0", port_);
}

void HttpServer::stop() {
    server_.stop();
}

};
