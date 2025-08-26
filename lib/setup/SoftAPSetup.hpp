//
// Created on 25/08/2025.
// Copyright (c) 2025 smart-fountain.
//

#ifndef SMART_FOUNTAIN_SOFTAPSETUP_HPP
#define SMART_FOUNTAIN_SOFTAPSETUP_HPP

#include <esp_http_server.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <nvs.h>
#include <string>
#include <utility>

static const auto TAG = "wifi_setup";

/**
 * @brief This class defines a "SoftAP" setup flow.
 * It exposes a wifi hotspot where the user can find a form to setup the creds of a WiFi router.
 * To use it, simply instantiate a SoftAPSetup object and call .setup().
 *
 * After creating an AP and setting up creds, the device will restart and connect to the router.
 */
class SoftAPSetup {
public:
    /**
     * @brief Initialize a SoftAPSetup flow, requires nvs_flash to be started
     * @param ssid SSID of the AP
     * @param password Password of the AP
     */
    explicit SoftAPSetup(std::string ssid = "ESP32_Setup", std::string password = "") : m_ssid{std::move(ssid)},
        m_password{std::move(password)} {
        // Create ap_config
        m_ap_config.ap.authmode = WIFI_AUTH_OPEN;
        /// Set SSID and Password
        size_t ssid_len = strnlen(m_ssid.c_str(), sizeof(m_ap_config.ap.ssid) - 1);
        size_t password_len = strnlen(m_password.c_str(), sizeof(m_ap_config.ap.password) - 1);
        memcpy(m_ap_config.ap.ssid, m_ssid.c_str(), ssid_len);
        m_ap_config.ap.ssid[ssid_len] = '\0';
        if (password_len > 0) {
            memcpy(m_ap_config.ap.password, m_password.c_str(), password_len);
            m_ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
            m_ap_config.ap.password[password_len] = '\0';
        }

        /// Config ap
        m_ap_config.ap.ssid_len = 0;
        m_ap_config.ap.max_connection = 4;
        m_ap_config.ap.channel = 1;
        m_ap_config.ap.beacon_interval = 100;
    }

    ~SoftAPSetup() = default;

    /**
     * @brief Start the HTTP server and returns its handle
     * @return Handle of the http server
     */
    void start();

private:
    std::string m_ssid;
    std::string m_password;
    wifi_config_t m_ap_config = {};
    httpd_handle_t m_server = nullptr;
    std::string m_connect_ssid;
    std::string m_connect_password;

    /**
     * @brief Callback for GET / endpoint
     */
    static esp_err_t root_get_handler(httpd_req_t *req);

    /**
     * @brief Callback for POST /setwifi endpoint
     */
    static esp_err_t set_wifi_post_handler(httpd_req_t *req);

    /**
     * @brief Check if WiFi creds were previously saved, if that's the case, save the creds in mem
     * @return True if setup was done
     */
    bool is_creds_saved();

    /**
     * @brief Logic to start an AP to setup the WiFi creds
     */
    void start_ap();

    /**
     * @brief Connects to a WiFi hotspot using previously setup credentials
     */
    void connect_to_wifi() const;
};


#endif //SMART_FOUNTAIN_SOFTAPSETUP_HPP