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

};
