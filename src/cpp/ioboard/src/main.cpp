#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>
#include "IOboard.h"
#include "Hatswitch.h"

#define SERIAL_BAUD 115200
#define STARTUP_LOG_LEVEL LOG_LEVEL_SILENT
#define RUN_LOG_LEVEL LOG_LEVEL_SILENT

MbedI2C i2c_device(i2c_sda, i2c_scl);
uint8_t i2c_addr = i2c_base_address;
uint8_t board_number;
uint8_t i2c_buffer[i2c_buffer_length] = {0};
int log_delay = 300;
int i2c_debug_count;

void I2CRequestISR();
void UpdateBufferBit(int bitNumber, bool value);
void CheckSerial();
void PeriodicLog(int delay);
void PeriodicFlash(uint8_t flashes);
void LogData();

void setup()
{
    // setup serial and logging
    Serial.begin(SERIAL_BAUD);
    Log.begin(STARTUP_LOG_LEVEL, &Serial, false);

    // setup button pin modes
    for (int button : buttons)
    {
        pinMode(button, INPUT_PULLUP);
    }
    
    // enable power for analog inputs (uses a high digital output as a 3V3 source)
    pinMode(analogue_power, OUTPUT);
    digitalWrite(analogue_power, HIGH);

    // read I2C address
    pinMode(i2c_address_lsb, INPUT_PULLUP);
    pinMode(i2c_address_msb, INPUT_PULLUP);
    board_number = (!digitalRead(i2c_address_msb) << 1) + !digitalRead(i2c_address_lsb) + 1;
    i2c_addr = i2c_base_address + board_number - 1;

    // setup I2C
    i2c_device.begin(i2c_addr);
    i2c_device.onRequest(I2CRequestISR);

    // Led
    pinMode(LED_BUILTIN, OUTPUT);

    Log.trace("Starting IO board on address %d" CR, i2c_addr);
    Log.notice("Change log level with [s]ilent, [f]atal, [e]rror, [w]arning, [n]otice, [t]race, [v]erbose" CR);
    Log.setLevel(RUN_LOG_LEVEL);
}

void loop()
{
    // Read Hatswitches
    for (int i=0; i < num_hats; i++) {
        i2c_buffer[i] = hats[i].Read();
    }

    // Read all button values into bitfields in a temp buffer
    uint8_t button_buffer[buttons_bytes] = {0};
    for (uint8_t i = 0; i < num_buttons; i++) {
        uint8_t bit = !digitalRead(buttons[i]) << (i % 8);
        button_buffer[i / 8] |= bit;
    }
    // Copy the temp buffer into the i2c output buffer
    for (uint8_t i=0; i<buttons_bytes; i++) {
        i2c_buffer[i + buttons_buffer_offset] = button_buffer[i];
    }

    // Read analogue axes values
    for (uint8_t i = 0; i < num_axes; i++) {
        int val = map(analogRead(axes[i]), 0, 1023, -32767, 32767);
        uint8_t axes_offset = i*2;
        i2c_buffer[axes_buffer_offset + axes_offset] = val >> 8; // msb
        i2c_buffer[axes_buffer_offset + axes_offset + 1] = val; // lsb
    }

    PeriodicLog(300);
    PeriodicFlash(board_number);
    CheckSerial();
}

void I2CRequestISR()
{
    i2c_device.write(i2c_buffer, i2c_buffer_length);
    i2c_debug_count++;
}

void CheckSerial() {
    if (Serial.available() > 0) {
        char c = Serial.read();
        switch (c) {
            case 's': Log.setLevel(LOG_LEVEL_SILENT);
                break;
            case 'f': Log.setLevel(LOG_LEVEL_FATAL);
                break;
            case 'e': Log.setLevel(LOG_LEVEL_ERROR);
                break;
            case 'w': Log.setLevel(LOG_LEVEL_WARNING);
                break;
            case 'n': Log.setLevel(LOG_LEVEL_NOTICE);
                break;
            case 't': Log.setLevel(LOG_LEVEL_TRACE);
                break;
            case 'v': Log.setLevel(LOG_LEVEL_VERBOSE);
                break;
            default:
                return;
        }
        Log.fatal("Log level: %c" CR, c);
    }
}

void PeriodicLog(int delay) {
    static ulong last_log_millis;
    if (millis() - last_log_millis > delay) {
        LogData();
        last_log_millis = millis();
    }
}

void PeriodicFlash(uint8_t flashes) {
    static ulong last_flash_millis;
    static uint delay;
    static bool state;
    static uint8_t flash;
    if (millis() - last_flash_millis > delay) {
        state = !state; // change state
        flash = (flash + 1) % (flashes *2);
        digitalWrite(LED_BUILTIN, state);
        delay = flash == 0 ? 2000 : 50;
        last_flash_millis = millis();
    }
}

void LogData() {
    if (Log.getLevel() == LOG_LEVEL_SILENT) {
        return;
    }
    for (int i = (num_hats + buttons_bytes) * 8 -1; i >= 0; i--) {
        int byte = i/8; int bit = i % 8;
        bool set = i2c_buffer[byte] & (1 << bit);
        Log.trace("%b", set);
    }
    for (int i = 0; i < num_axes; i++) {
        int hi = i2c_buffer[axes_buffer_offset + i*2];
        int low = i2c_buffer[axes_buffer_offset + i*2 + 1];
        int padding = hi & 0x80 ? (int)0xFFFF0000 : 0x00000000;
        int value = padding | (hi << 8) | low;
        Log.trace(" %d", value);
    }
    Log.trace(" i2c:%d" CR, i2c_debug_count);
}