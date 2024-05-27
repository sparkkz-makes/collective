#include <Arduino.h>
#include <ArduinoLog.h>
#include <PicoGamepad.h>
#include <Wire.h>

#define SERIAL_BAUD 115200
#define STARTUP_LOG_LEVEL LOG_LEVEL_VERBOSE
#define RUN_LOG_LEVEL LOG_LEVEL_SILENT

static const uint8_t i2c_base_address = 8;
static const uint8_t num_io_boards = 2;
static const uint8_t data_packet_size = 8;

void ReadIOBoards();
void ReadLocalInputs();



void setup()
{
    Serial.begin(SERIAL_BAUD);
    while (!Serial);
    Log.begin(STARTUP_LOG_LEVEL, &Serial, false);

    Wire.begin();
}

void loop()
{
    ReadIOBoards();
    delay(200);
}

void ReadIOBoards() {
    Wire.requestFrom(i2c_base_address, data_packet_size);
    for (int i=0; i<data_packet_size; i++) {
        if (Wire.available()) {
            uint8_t data = Wire.read();
            Log.trace("%X ", data);
        }
        else {
            Log.error("No I2C data available (byte %d)" CR, i);
        }
    }
    Log.trace(CR);

}