#ifndef IOBOARD
#define IOBOARD

#include <Arduino.h>
#include "Hatswitch.h"

// -- Hat Switches --
static const hat_pins hat_1_pins = {0, 1, 2, 3, 4};
static const hat_pins hat_2_pins = {5, 6, 7, 8, 9};
static const Hatswitch hats[] = {
    Hatswitch(hat_1_pins),
    Hatswitch(hat_2_pins),
};
static const uint8_t num_hats = sizeof(hats) / sizeof(Hatswitch);

// -- Buttons --
static const uint8_t buttons[] = {10, 11, 12, 13, 14, 15, 16, 17, 28};
static const uint8_t num_buttons = sizeof(buttons) / sizeof(uint8_t);
static const uint8_t buttons_bytes = num_buttons / 8 + 1;
static const uint8_t buttons_buffer_offset = num_hats;

// -- Axes --
static const uint8_t axes[] = { A0, A1 };
static const uint8_t num_axes = sizeof(axes) / sizeof(uint8_t);
static const uint8_t axes_bytes = num_axes * 2;
static const uint8_t axes_buffer_offset = buttons_buffer_offset + buttons_bytes;
static const uint8_t analogue_power = 22;

// -- I2C Comms --
static const uint8_t i2c_base_address = 8;
static const uint8_t i2c_address_lsb = 18;
static const uint8_t i2c_address_msb = 19;
static const uint8_t i2c_sda = 20;
static const uint8_t i2c_scl = 21;
static const uint8_t i2c_buffer_length = num_hats + buttons_bytes + axes_bytes;


#endif