#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>
#include "IOboard.h"
#include "Hatswitch.h"

#define SERIAL_BAUD 115200
#define STARTUP_LOG_LEVEL LOG_LEVEL_VERBOSE
#define RUN_LOG_LEVEL LOG_LEVEL_SILENT

uint8_t i2c_addr = i2c_base_address;
uint8_t i2c_buffer[i2c_buffer_length] = {0};
int log_delay = 300;


void I2CRequestISR();
void UpdateBufferBit(int bitNumber, bool value);
void CheckSerial();
void PeriodicLog(int delay);
void LogData();

void setup()
{
    // setup serial and logging
    Serial.begin(SERIAL_BAUD);
    while (!Serial);
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
    i2c_addr = i2c_base_address + (!digitalRead(i2c_address_msb) << 1) + !digitalRead(i2c_address_lsb);

    // setup I2C
    Wire.setSDA(i2c_sda);
    Wire.begin(i2c_addr);
    Wire.onRequest(I2CRequestISR);

    Log.trace("Starting IO board on address %d" CR, i2c_addr);
    Log.notice("Change log level with [s]ilent, [f]atal, [e]rror, [w]arning, [n]otice, [t]race, [v]erbose" CR);
    //Log.setLevel(RUN_LOG_LEVEL);
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

    // // ##### temp output values to serial #####
    // for (int i = (num_hats + buttons_bytes) * 8 -1; i >= 0; i--) {
    //     int byte = i/8; int bit = i % 8;
    //     bool set = i2c_buffer[byte] & (1 << bit);
    //     Log.trace("%b", set);
    // }
    // for (int i = 0; i < num_axes; i++) {
    //     int hi = i2c_buffer[axes_buffer_offset + i*2];
    //     int low = i2c_buffer[axes_buffer_offset + i*2 + 1];
    //     int padding = hi & 0x80 ? (int)0xFFFF0000 : 0x00000000;
    //     int value = padding | (hi << 8) | low;
    //     Log.trace(" %d", value);
    // }
    // Log.trace(CR);
    // //Log.trace("%b"CR, digitalRead(28));
    // delay(300);
    PeriodicLog(300);
    CheckSerial();
}

void I2CRequestISR()
{
    Wire.write(i2c_buffer, i2c_buffer_length);
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

void LogData() {
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
    Log.trace(CR);
}