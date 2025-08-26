#include <led_strip.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>

#include <HX711.hpp>
#include <SoftAPSetup.hpp>

#include "colors.hpp"
#include "led.hpp"
#include "server/WebServer.hpp"


// Declare and define a custom event base for scale-related events
ESP_EVENT_DECLARE_BASE(SCALE_EVENT);
ESP_EVENT_DEFINE_BASE(SCALE_EVENT);

static led_strip_handle_t g_led_strip = nullptr;
// Use a dedicated event loop for scale-related events (avoid default loop where possible)
static esp_event_loop_handle_t g_scale_loop = nullptr;

static void on_ip_got(void *arg, esp_event_base_t event_base, int32_t event_id, [[maybe_unused]] void *event_data) {
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI("net", "Got IP, starting server");
        auto *server = static_cast<WebServer *>(arg);
        // Start server when station gets an IP
        server->start();
    }
}

static void on_wifi_disconnected(void *arg,
                                 esp_event_base_t event_base,
                                 int32_t event_id,
                                 [[maybe_unused]] void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGE("net", "Disconnected from AP");
        auto *server = static_cast<WebServer *>(arg);
        // Stop server when disconnected
        server->stop();
        // Restart
        //esp_restart();
    }
}


static void on_scale_init(void *arg,
                          esp_event_base_t event_base,
                          [[maybe_unused]] int32_t event_id,
                          [[maybe_unused]] void *event_data) {
    if (event_base == SCALE_EVENT) {
        auto *sc = static_cast<HX711 *>(arg);
        ESP_LOGI("scale", "Initializing HX711 scale in worker task");
        set_led_color(g_led_strip, COLOR_ORANGE);
        sc->tare(10);
        set_led_color(g_led_strip, COLOR_NONE);
        ESP_LOGI("scale", "Scale initialized and tared");
    }
}


extern "C" void app_main(void) {
    // Reset LED
    configure_led(&g_led_strip);
    set_led_color(g_led_strip, COLOR_NONE);

    // Init nvs & net stack
    nvs_flash_init();
    esp_netif_init();

    // Create the default event loop
    esp_event_loop_create_default();
    // Create a dedicated event loop for scale events (avoid default loop where possible)
    constexpr esp_event_loop_args_t scale_loop_args = {
        .queue_size = 8,
        .task_name = "scale_evt",
        .task_priority = 5,
        .task_stack_size = 4096,
        .task_core_id = 1,
    };
    ESP_ERROR_CHECK(esp_event_loop_create(&scale_loop_args, &g_scale_loop));


    auto setup = SoftAPSetup("CatFountain-Setup");
    setup.start();

    static auto *server = new WebServer();
    static auto *scale = new HX711(GPIO_NUM_1, GPIO_NUM_2, GAIN_128);

    // Register Wi-Fi/IP event handlers to control the web server lifecycle
    esp_event_handler_instance_t got_ip_instance;
    esp_event_handler_instance_t disconnected_instance;
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_ip_got, server, &got_ip_instance));
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnected, server,
            &disconnected_instance));

    // Register a scale initialization event handler and post the init event with the scale as an argument
    enum { SCALE_EVENT_INIT = 0 };
    esp_event_handler_instance_t scale_init_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(
        g_scale_loop, SCALE_EVENT, SCALE_EVENT_INIT, &on_scale_init, scale, &scale_init_instance));


    // Send a scale init event right after to trigger it
    ESP_ERROR_CHECK(
    esp_event_post_to(g_scale_loop, SCALE_EVENT, SCALE_EVENT_INIT, nullptr, 0, portMAX_DELAY));

    server
            ->registerUri("/", HTTP_GET, [](httpd_req_t *req) {
                httpd_resp_sendstr(req, "Hello World");
                return ESP_OK;
            })
            .registerUri("/scale", HTTP_GET, [](httpd_req_t *req) {
                const auto scale_value = scale->get_units();
                const auto resp = R"({"value": )" + std::to_string(scale_value) + R"(})";
                httpd_resp_set_type(req, "application/json");
                httpd_resp_send(req, resp.c_str(), HTTPD_RESP_USE_STRLEN);
                return ESP_OK;
            });

    vTaskDelay(portMAX_DELAY);
}