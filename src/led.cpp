//
// Created on 26/08/2025.
// Copyright (c) 2025 untitled2.
//

#include "led.hpp"

void configure_led(led_strip_handle_t *ledStrip) {
    // LED strip initialization with the GPIO and pixels number
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = GPIO_NUM_48;
    strip_config.max_leds = 1;
    led_strip_spi_config_t spi_config = {};
    spi_config.spi_bus = SPI2_HOST;
    spi_config.flags.with_dma = false;

    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, ledStrip));
    led_strip_clear(*ledStrip);
}

void set_led_color(const led_strip_handle_t ledStrip, const COLOR color) {
    led_strip_set_pixel(ledStrip, 0, color.r, color.g, color.b);
    led_strip_refresh(ledStrip);
}