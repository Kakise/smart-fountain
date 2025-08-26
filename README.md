# Open Smart Cat Fountain (ESP32‑S3)

An ESP32‑S3–powered smart cat fountain that:
- Controls a water pump
- Measures water level/weight via an HX711 load cell amplifier
- Serves a local web dashboard
- Exposes runtime metrics via a web API and a Prometheus `/metrics` endpoint

The project is written in C++23 and includes two reusable libraries:
- `setup`: a `SoftAPSetup` C++ class that provides a captive-portal–style Wi‑Fi provisioning flow
- `hx711`: a C++ driver for the HX711 24‑bit ADC used with a load cell

## Features

- ESP32‑S3 microcontroller
- Pump control (with support for on/off and optional duty‑cycle based throttling)
- Load‑cell sensing through HX711 for water level/weight estimation
- Network connectivity via Wi‑Fi
- Local web dashboard for live status
- Prometheus‑compatible metrics endpoint for observability
- Pluggable Wi‑Fi setup flow (SoftAP captive portal)
- Designed with safety in mind for continuous, unattended operation

## Hardware Overview

- ESP32‑S3 development board
- HX711 module + load cell (placed under the reservoir or integrated into the base)
- Pump driver:
  - Logic‑level MOSFET for DC pumps or a suitable driver/relay module
  - Flyback diode for inductive loads (mandatory for DC pump motors)
- Power supply sized for the pump (e.g., 5–12V depending on pump) and the ESP32‑S3 (regulated 5V/3.3V as required)
- Optional:
  - Flow sensor (e.g., Hall sensor) for additional metrics
  - Temperature or water quality sensors if desired
- Enclosure and waterproofing
- Ground‑fault protection (GFCI) recommended when operating near water

Always validate electrical ratings, wiring polarity, and isolation. Keep mains voltages fully isolated from low‑voltage electronics.

## Software Overview

- Language/Standard: C++23
- Target MCU: ESP32‑S3
- Metrics and APIs:
  - Web UI: `GET /` (status dashboard)
  - Prometheus: `GET /metrics` (text exposition format)
  - JSON metrics: `GET /api/metrics` (machine‑readable snapshot)
  - Health: `GET /api/health` → HTTP 200 OK when healthy
  - Control endpoints (optional, if enabled): e.g., `POST /api/pump` to set on/off or duty cycle
- Wi‑Fi provisioning:
  - On first boot (or when not configured), device starts a SoftAP (e.g., `CatFountain-XXXX`)
  - Captive portal guides the user to enter Wi‑Fi credentials
  - Credentials are stored securely on the device for subsequent boots

## Getting Started

### 1) Build and Flash

You can use either PlatformIO or ESP‑IDF toolchains. The general flow is:

- Install PlatformIO (VS Code extension) or set up ESP‑IDF
- Select an ESP32‑S3 board configuration
- Build the firmware
- Flash the device over USB
- Open a serial monitor to view logs

Tip: If you are using PlatformIO, choose an ESP32‑S3 devkit environment and ensure the correct USB/serial port is selected.

### 2) First Boot and Wi‑Fi Setup

- Power the device; if no Wi‑Fi credentials are stored, it will start a SoftAP:
  - SSID: `CatFountain-Setup`
  - Password: shown in the serial logs or device label (if configured)
- Connect from your phone/laptop and open the captive portal (usually auto‑redirects; if not, visit `http://192.168.4.1`)
- Enter your home Wi‑Fi credentials
- The device reboots and joins your network

If you need to reset Wi‑Fi settings later, use the device’s reset procedure (e.g., a long‑press button or a config flag) and repeat the provisioning.

### 3) HX711 Calibration

To get accurate water level/weight readings:

1. Tare:
   - Ensure the reservoir is empty and the load cell is installed as it will be used
   - Perform a tare operation to zero the scale

2. Scale factor:
   - Place a known weight on the reservoir/platform
   - Adjust the calibration factor until the reading matches the known weight

3. Verify:
   - Test multiple weights or water volumes to confirm linearity and stability

The calibration values are stored for persistence across reboots.

## Web Interface and APIs

- Dashboard: `GET /`
  - Displays current status such as pump state, water level/weight, and uptime

- JSON Metrics: `GET /api/metrics`
  - Example fields (may vary by build): water_level_weight, pump_state, pump_duty, uptime_seconds, wifi_rssi

- Health Check: `GET /api/health`
  - Returns HTTP 200 OK with a simple status body when the device is healthy

- Prometheus Metrics: `GET /metrics`
  - Standard text exposition format
  - Designed for Prometheus/Grafana observability pipelines

### Prometheus Scrape Example

```yaml 
scrape_configs:
- job_name: 'cat_fountain' 
  metrics_path: /metrics 
  static_configs:
    - targets: ['cat-fountain.local:80']
```


Adjust the hostname/IP and port to match your network.

## Safety Notes

- Use an appropriately rated power supply and wiring
- Isolate and protect electronics from water ingress
- Use GFCI outlets when operating near water
- Ensure the pump driver and heat dissipation are adequate for continuous duty
- Provide strain relief for cables; avoid pinch points
- Keep firmware up to date and test fail‑safe behaviors (e.g., shut off pump on fault)

## Project Structure (high‑level)

- Application sources (firmware entry point and services)
- Web assets for the dashboard (served from flash filesystem or embedded)
- Libraries:
    - `setup`: `SoftAPSetup` class for Wi‑Fi provisioning via SoftAP/captive portal
    - `hx711`: HX711 driver for load cell integration
- Configuration and calibration storage (non‑volatile)

## Development Tips

- Start with serial logging enabled to observe boot, Wi‑Fi, and sensor initialization
- Validate HX711 wiring (E+, E-, A+, A-) and ensure stable 3.3V power; keep load cell cables short
- Debounce or filter sensor readings; consider averaging and outlier rejection
- Consider adding brown‑out detection, watchdog, and safe defaults for the pump
- If adding flow or temperature sensors, expose them through `/api/metrics` and `/metrics`

## Roadmap Ideas

- OTA firmware updates
- Pump scheduling and dry‑run protection
- Alerting when water level is low or pump current spikes
- Advanced metrics (flow rate, refill detection)
- Home automation integrations (e.g., MQTT, Home Assistant)
- Secure configuration endpoints with authentication

## License

GPL 3.0

## Contributing

Contributions are welcome! Please open an issue to discuss ideas or submit a pull request for improvements and fixes.

## Acknowledgments

- ESP32‑S3 ecosystem and community
- HX711 documentation and community examples
- Prometheus/Grafana for observability