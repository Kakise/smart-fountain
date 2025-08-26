//
// Created on 25/08/2025.
// Copyright (c) 2025 smart-fountain.
//

#ifndef SMART_FOUNTAIN_WEBSERVER_HPP
#define SMART_FOUNTAIN_WEBSERVER_HPP
#include <esp_http_server.h>
#include <esp_log.h>
#include <functional>
#include <map>
#include <string>

struct HANDLER {
    std::function<esp_err_t(httpd_req_t *)> fn;
    httpd_method_t method;
};

class WebServer {
public:
    WebServer();

    ~WebServer();

    WebServer &registerUri(const char *uri,
                           httpd_method_t method,
                           const std::function<esp_err_t(httpd_req_t *)> &handler);

    WebServer &start();

    WebServer &stop();

private:
    httpd_handle_t m_server;

    static std::map<std::string, HANDLER> handlers;

    static esp_err_t dispatch_handler(httpd_req_t *req);
};


#endif //SMART_FOUNTAIN_WEBSERVER_HPP