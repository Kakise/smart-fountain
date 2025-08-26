//
// Created on 22/08/2025.
//

#ifndef HX711_H
#define HX711_H

#include <driver/gpio.h>

enum HX711_GAIN : uint8_t {
    GAIN_128 = 1,
    GAIN_64  = 3,
    GAIN_32  = 2,
};

class HX711 {
public:
    HX711(gpio_num_t m_clk, gpio_num_t m_data, HX711_GAIN m_gain = GAIN_128);

    int32_t read_raw();

    int32_t read_average(uint8_t n);

    /**
     * @brief Sets the current load as zero reference by taking n samples and averaging them.
     * @param times The number of samples to compute the average, defaults to 10
     */
    void tare(uint16_t times = 10);

    void set_scale(float scale);

    float get_units();

    float get_units(uint8_t times);

    void set_gain(uint8_t gain);

    void power_down();

    void power_up();

private:
    gpio_num_t m_clk;
    gpio_num_t m_data;

    HX711_GAIN m_gain;
    int32_t m_tare;
    float m_scale;

    [[nodiscard]] bool is_ready() const;

    [[nodiscard]] bool wait_ready_timeout(uint16_t timeout_ms = 1000) const;

    int32_t read_once();

    void apply_gain_pulses() const;

    inline void clk_high() const;

    inline void clk_low() const;
};


#endif //HX711_H