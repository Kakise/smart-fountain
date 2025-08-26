//
// Created on 26/08/2025.
// Copyright (c) 2025 untitled2.
//

#ifndef SMART_FOUNTAIN_LED_HPP
#define SMART_FOUNTAIN_LED_HPP
#include <led_strip.h>
#include "colors.hpp"

void configure_led(led_strip_handle_t *ledStrip);

void set_led_color(led_strip_handle_t ledStrip, COLOR color);

#endif