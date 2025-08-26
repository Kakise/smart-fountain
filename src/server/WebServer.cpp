#include "server/WebServer.hpp"

static auto TAG = "WebServer";

esp_err_t health_handler(httpd_req_t *req) {
    const auto resp = R"({"status": "ok", "version": "1.0"})";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

WebServer::WebServer() : m_server(nullptr) {
    registerUri("/health", HTTP_GET, health_handler);
}

WebServer::~WebServer() { stop(); }

WebServer &WebServer::registerUri(const char *uri,
                                  httpd_method_t method,
                                  const std::function<esp_err_t(httpd_req_t *)> &handler) {
    ESP_LOGI(TAG, "Registering URI: %s", uri);
    handlers[uri] = {handler, method};
    if (m_server) {
        const httpd_uri_t ep = {
            .uri = uri,
            .method = method,
            .handler = dispatch_handler,
            .user_ctx = nullptr
        };
        httpd_register_uri_handler(m_server, &ep);
    }
    ESP_LOGI(TAG, "Registered URI: %s", uri);
    return *this;
}

WebServer &WebServer::start() {
    if (m_server) {
        // Already started; no-op
        return *this;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    if (const esp_err_t err = httpd_start(&m_server, &config); err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WebServer: %s", esp_err_to_name(err));
        m_server = nullptr;
        return *this;
    }

    ESP_LOGI(TAG, "WebServer started");

    auto register_handlers = [this]() {
        for (const auto &[uri, handler] : handlers) {
            httpd_uri_t ep = {
                .uri = uri.c_str(),
                .method = handler.method,
                .handler = dispatch_handler,
                .user_ctx = nullptr
            };
            ESP_LOGI(TAG, "Exposing URI: %s", uri.c_str());
            httpd_register_uri_handler(m_server, &ep);
        }
    };
    register_handlers();

    return *this;
}


WebServer &WebServer::stop() {
    if (m_server) {
        httpd_stop(m_server);
        m_server = nullptr;
    }
    return *this;
}

std::map<std::string, HANDLER > WebServer::handlers;

esp_err_t WebServer::dispatch_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Dispatching URI: %s", req->uri);
    const char *uri = req->uri;
    const auto it = handlers.find(uri);
    if (it == handlers.end()) {
        // No handler found for this URI
        return ESP_FAIL;
    }
    const auto &fn = it->second.fn;
    if (!fn) {
        return ESP_FAIL;
    }
    return fn(req);
}
