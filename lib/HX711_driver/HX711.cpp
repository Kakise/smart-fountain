//
// Created on 22/08/2025.
//

#include "HX711.hpp"

#include <esp_log.h>
#include <esp_rom_sys.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <iostream>

// Sets delay to 1µs clock for robustness
#define HX711_DELAY esp_rom_delay_us(1);


HX711::HX711(const gpio_num_t m_clk, const gpio_num_t m_data, const HX711_GAIN m_gain) : m_clk(m_clk),
    m_data(m_data),
    m_gain{m_gain},
    m_tare{0},
    m_scale{1.0f} {
    ESP_LOGI("HX711", "Init scale");

    gpio_config_t io_conf{};

    io_conf.pin_bit_mask = 1ULL << m_clk;
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = 1ULL << m_data;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io_conf);

    clk_low();

    power_up();

    set_gain(m_gain);

    ESP_LOGI("HX711", "Gain: %d", static_cast<int>(m_gain));
    ESP_LOGI("HX711", "Tare: %d", m_tare);
    ESP_LOGI("HX711", "Scale: %f", m_scale);
}

bool HX711::is_ready() const {
    return gpio_get_level(m_data) == 0;
}

bool HX711::wait_ready_timeout(uint16_t timeout_ms) const {
    const TickType_t start = xTaskGetTickCount();
    const TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    // Poll with cooperative yield so IDLE task can run and feed WDT
    while (!is_ready()) {
        if ((xTaskGetTickCount() - start) > timeout_ticks) {
            return false;
        }
        // Yield at least one tick to let lower-priority tasks (IDLE) run
        vTaskDelay(1);
    }
    return true;
}

int32_t HX711::read_raw() {
    if (!wait_ready_timeout(1000)) {
        // Timeout: return last offset to avoid extreme outliers (or could throw/errno)
        return 0;
    }
    return read_once();
}

int32_t HX711::read_average(const uint8_t times) {
    if (times == 0) return 0;
    int32_t sum = 0;
    for (uint8_t i = 0; i < times; ++i) {
        sum += read_raw();
    }
    return sum / times;
}

void HX711::tare(const uint16_t times) {
    m_tare = read_average(times);
    ESP_LOGI("HX711", "Tare: %d", m_tare);
}


void HX711::set_scale(const float scale) {
    if (scale == 0.0f) {
        m_scale = 1.0f;
    } else {
        m_scale = scale;
    }
}

float HX711::get_units() {
    return get_units(10);
}

float HX711::get_units(uint8_t times) {
    const int32_t raw = read_average(times);
    const int32_t net = raw - m_tare;
    return static_cast<float>(net) / m_scale;
}

void HX711::set_gain(uint8_t gain) {
    // After changing m_gain, we need to sync HX711 mode by performing a read
    // (The gain selection is applied on the extra pulses at the end of a read)
    if (is_ready()) {
        (void) read_once();
    } else {
        // Try to set as soon as it’s ready (non-blocking quick attempt)
        if (wait_ready_timeout(5)) {
            (void) read_once();
        }
    }
}

void HX711::power_down() {
    // To enter power-down mode, pull CLK high for >60 us while DOUT is high
    // Ensure DOUT is high (not ready). If currently ready, perform a dummy read to move forward.
    if (is_ready()) {
        (void) read_once();
    }
    clk_high();
    esp_rom_delay_us(80); // >60us requirement
}

void HX711::power_up() {
    // Bring clock low to wake up; first reading is invalid and should be discarded per datasheet
    clk_low();
    esp_rom_delay_us(100);
    // Discard one reading to re-sync gain/channel
    if (wait_ready_timeout(100)) {
        (void) read_once();
    }
}

int32_t HX711::read_once() {
    // Read 24 bits MSB-first. On the 25th-27th pulse, we set gain/channel.
    uint32_t value = 0;

    // Ensure CLK low before starting
    clk_low();
    HX711_DELAY;
    ESP_LOGD("HX711", "Clock LOW");

    portDISABLE_INTERRUPTS();

    // Read 24 bits
    for (int i = 0; i < 24; ++i) {
        clk_high();
        HX711_DELAY;
        value = (value << 1) | (gpio_get_level(m_data) & 0x01);
        clk_low();
        HX711_DELAY;
    }
    portENABLE_INTERRUPTS();
    value = value ^ 0x800000;

    // Sign extend if MSB (bit 23) is set
    // if (value & 0x800000) {
    //     value |= 0xFF000000;
    // }

    // Apply gain selection pulses (1..3 extra pulses)
    apply_gain_pulses();

    return static_cast<int32_t>(value);
}

void HX711::apply_gain_pulses() const {
    // Pulses after 24th bit select the next conversion's gain/channel:
    // 1 pulse -> Channel A, gain 128
    // 2 pulses -> Channel B, gain 32
    // 3 pulses -> Channel A, gain 64
    for (int i = 0; i < m_gain; ++i) {
        clk_high();
        HX711_DELAY;
        clk_low();
        HX711_DELAY;
    }
}

inline void HX711::clk_high() const {
    gpio_set_level(m_clk, 1);
}

inline void HX711::clk_low() const {
    gpio_set_level(m_clk, 0);
}


