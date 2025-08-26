//
// Created on 25/08/2025.
// Copyright (c) 2025 smart-fountain.
//

#include "SoftAPSetup.hpp"

// Simple in-place URL decoder for application/x-www-form-urlencoded
// - Converts %HH to the byte value
// - Converts '+' to ' '
// Safe to call on NUL-terminated C strings
static void url_decode_inplace(char* s) {
    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    const char* r = s; // read
    char* w = s; // write
    while (*r) {
        if (*r == '%') {
            const int hi = hex(*(r + 1));
            if (const int lo = hex(*(r + 2)); hi >= 0 && lo >= 0) {
                *w++ = static_cast<char>((hi << 4) | lo);
                r += 3;
            } else {
                // Malformed percent sequence, copy as-is
                *w++ = *r++;
            }
        } else if (*r == '+') {
            *w++ = ' ';
            r++;
        } else {
            *w++ = *r++;
        }
    }
    *w = '\0';
}


void SoftAPSetup::start() {
    if (is_creds_saved()) {
        ESP_LOGI(TAG, "Credentials saved, connect to WIFI");
        connect_to_wifi();
    } else {
        // No credentials saved, start AP
        ESP_LOGI(TAG, "No credentials saved, start AP");
        start_ap();
    }
}

esp_err_t SoftAPSetup::root_get_handler(httpd_req_t *req) {
    const char *resp = R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>Wi‑Fi Setup</title>
<style>
  :root {
    --bg: #0f172a;        /* slate-900 */
    --card: #111827;      /* gray-900 */
    --muted: #94a3b8;     /* slate-400 */
    --text: #e5e7eb;      /* gray-200 */
    --accent: #22c55e;    /* green-500 */
    --accent-700: #15803d;/* green-700 */
    --ring: rgba(34,197,94,.45);
    --error: #ef4444;
    --shadow: 0 10px 30px rgba(0,0,0,.35);
  }
  * { box-sizing: border-box; }
  html, body {
    height: 100%;
    margin: 0;
    color: var(--text);
    font: 400 16px/1.5 system-ui, -apple-system, Segoe UI, Roboto, Ubuntu, Cantarell, Noto Sans, Arial, "Apple Color Emoji","Segoe UI Emoji";
    background:
      radial-gradient(1200px 600px at 10% 10%, rgba(34,197,94,.12), transparent 60%),
      radial-gradient(900px 500px at 100% 0%, rgba(59,130,246,.10), transparent 60%),
      var(--bg);
  }
  .wrap {
    min-height: 100%;
    display: grid;
    place-items: center;
    padding: 24px;
  }
  .card {
    width: 100%;
    max-width: 420px;
    background: linear-gradient(180deg, rgba(255,255,255,.04), transparent 18%) var(--card);
    border: 1px solid rgba(148,163,184,.15);
    border-radius: 16px;
    box-shadow: var(--shadow);
    padding: 22px;
  }
  .brand {
    display: flex;
    align-items: center;
    gap: 10px;
    margin-bottom: 8px;
  }
  .brand-badge {
    width: 36px; height: 36px; border-radius: 10px;
    display: grid; place-items: center;
    background: linear-gradient(135deg, rgba(34,197,94,.25), rgba(59,130,246,.25));
    border: 1px solid rgba(148,163,184,.2);
  }
  h1 {
    font-size: 18px; margin: 0;
    letter-spacing: .2px;
  }
  p.desc {
    margin: 4px 0 18px; color: var(--muted); font-size: 14px;
  }
  form { display: grid; gap: 14px; }
  .field { display: grid; gap: 8px; }
  label { font-size: 13px; color: var(--muted); }
  .input {
    display: flex; align-items: center; gap: 8px;
    background: rgba(2,6,23,.5);
    border: 1px solid rgba(148,163,184,.2);
    border-radius: 10px; padding: 12px 12px;
  }
  .input:focus-within { border-color: var(--accent); box-shadow: 0 0 0 3px var(--ring); }
  .input input {
    width: 100%; border: 0; outline: 0; background: transparent; color: var(--text);
    font-size: 16px;
  }
  .toggle {
    cursor: pointer; user-select: none; color: var(--muted); font-size: 12px;
    padding: 4px 8px; border-radius: 8px; border: 1px solid rgba(148,163,184,.25);
    background: rgba(15,23,42,.35);
  }
  .btn {
    width: 100%;
    padding: 12px 14px;
    background: linear-gradient(180deg, var(--accent), var(--accent-700));
    border: 0; border-radius: 12px;
    color: white; font-weight: 600; letter-spacing: .3px;
    box-shadow: 0 6px 18px rgba(34,197,94,.35);
    transition: transform .05s ease;
  }
  .btn:active { transform: translateY(1px); }
  .hint { color: var(--muted); font-size: 12px; margin-top: -4px; }
  .footer {
    margin-top: 14px; text-align: center; color: var(--muted); font-size: 12px;
  }
</style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <div class="brand">
        <div class="brand-badge" aria-hidden="true">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round">
            <path d="M12 3v3" /><path d="M12 18v3" /><path d="M3 12h3" /><path d="M18 12h3" />
            <circle cx="12" cy="12" r="5" />
          </svg>
        </div>
        <h1>Wi‑Fi Setup</h1>
      </div>
      <p class="desc">Connect your device to a Wi‑Fi network.</p>

      <form action="/setwifi" method="post" autocomplete="on">
        <div class="field">
          <label for="ssid">Network name (SSID)</label>
          <div class="input">
            <input id="ssid" name="ssid" type="text" inputmode="text" autocomplete="ssid"
                   placeholder="e.g. MyHomeWiFi" required maxlength="32" />
          </div>
          <div class="hint">Case‑sensitive. Hidden networks supported.</div>
        </div>

        <div class="field">
          <label for="pass">Password</label>
          <div class="input">
            <input id="pass" name="pass" type="password" autocomplete="current-password"
                   placeholder="Enter Wi‑Fi password" minlength="8" maxlength="63" />
            <button type="button" class="toggle" id="togglePass" aria-controls="pass" aria-pressed="false">Show</button>
          </div>
          <div class="hint">Use 8–63 characters (WPA/WPA2).</div>
        </div>

        <button class="btn" type="submit">Connect</button>
      </form>

      <div class="footer">Device is in Access Point mode.</div>
    </div>
  </div>

<script>
  (function(){
    var btn = document.getElementById('togglePass');
    var input = document.getElementById('pass');
    if (btn && input) {
      btn.addEventListener('click', function(){
        var isPw = input.type === 'password';
        input.type = isPw ? 'text' : 'password';
        btn.textContent = isPw ? 'Hide' : 'Show';
        btn.setAttribute('aria-pressed', String(isPw));
      });
    }
  })();
</script>
</body>
</html>)";

    ESP_LOGI(TAG, "Accessed WiFi Setup Page");
    ESP_LOGI(TAG, "Responding with %d bytes", strlen(resp));
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t SoftAPSetup::set_wifi_post_handler(httpd_req_t *req) {
    char buf[128];
    const int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = 0;

    // crude parser: "ssid=MyWiFi&pass=MyPassword"
    char ssid[32] = {0}, pass[64] = {0};
    sscanf(buf, "ssid=%31[^&]&pass=%63s", ssid, pass);

    ESP_LOGI(TAG, "Got SSID: %s  PASS: %s", ssid, pass);

    // Save to NVS
    nvs_handle_t nvs;
    nvs_open("wifi", NVS_READWRITE, &nvs);
    nvs_set_str(nvs, "ssid", ssid);
    nvs_set_str(nvs, "pass", pass);
    nvs_commit(nvs);
    nvs_close(nvs);

    httpd_resp_sendstr(req, "Credentials saved! Rebooting...");

    // small delay then restart
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_restart();
}

bool SoftAPSetup::is_creds_saved() {
    nvs_handle_t nvs;
    nvs_open("wifi", NVS_READONLY, &nvs);

    char *ssid = nullptr;
    char *pass = nullptr;

    // Get size
    size_t ssid_size;
    size_t pass_size;
    nvs_get_str(nvs, "ssid", nullptr, &ssid_size);
    nvs_get_str(nvs, "pass", nullptr, &pass_size);

    // Allocate memory & read
    ssid = static_cast<char *>(malloc(ssid_size));
    pass = static_cast<char *>(malloc(pass_size));
    nvs_get_str(nvs, "ssid", ssid, &ssid_size);
    nvs_get_str(nvs, "pass", pass, &pass_size);

    nvs_close(nvs);

    if (ssid && pass) {
        url_decode_inplace(pass);
        m_connect_ssid = ssid;
        m_connect_password = pass;
        ESP_LOGI(TAG, "SSID: %s PASS: %s", ssid, pass);
        free(ssid);
        free(pass);
        return true;
    }

    ESP_LOGI(TAG, "No credentials saved");
    return false;
}

void SoftAPSetup::start_ap() {
    // Init net stack & wifi
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // Start AP
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &m_ap_config);
    esp_wifi_start();

    if (!m_password.empty())
        ESP_LOGI(TAG, "AP started. SSID:%s password:%s", m_ssid.c_str(), m_password.c_str());
    else
        ESP_LOGI(TAG, "AP started. SSID:%s", m_ssid.c_str());

    constexpr httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = nullptr;

    if (httpd_start(&server, &config) == ESP_OK) {
        constexpr httpd_uri_t root = {"/", HTTP_GET, root_get_handler, nullptr};
        httpd_register_uri_handler(server, &root);

        httpd_uri_t setwifi = {"/setwifi", HTTP_POST, set_wifi_post_handler, nullptr};
        httpd_register_uri_handler(server, &setwifi);
    }
    m_server = server;
}

void SoftAPSetup::connect_to_wifi() const {
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t sta_config = {};
    memcpy(sta_config.sta.ssid, m_connect_ssid.c_str(), m_connect_ssid.length() + 1);
    memcpy(sta_config.sta.password, m_connect_password.c_str(), m_connect_password.length() + 1);

    esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    esp_wifi_start();
    esp_wifi_connect();
}