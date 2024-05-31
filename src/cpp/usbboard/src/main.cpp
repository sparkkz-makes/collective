#include <Arduino.h>
#include <ArduinoLog.h>
#include <PicoGamepad.h>
#include <Wire.h>

#define SERIAL_BAUD 115200
#define STARTUP_LOG_LEVEL LOG_LEVEL_SILENT
#define RUN_LOG_LEVEL LOG_LEVEL_SILENT

static const uint8_t i2c_base_address = 8;
static const uint8_t num_io_boards = 2;
static const uint8_t data_packet_size = 8;
static const uint8_t buttons_per_board = 9;
static const uint8_t local_adc = A1;
static const uint8_t local_adc_power = 26;
uint8_t io_board_data[num_io_boards][data_packet_size]; // 2 io boards, 8 bytes per packet

MbedI2C i2c_controller(0, 1);
PicoGamepad gamepad;

void ReadIOBoards();
void ReadLocalInputs();
void CheckSerial();
void set_button(uint8_t button, uint8_t board, uint8_t byte, uint8_t bit);
int bytes_to_int(uint8_t msb, uint8_t lsb);
void PeriodicFlash(uint8_t flashes);

void setup()
{
    Serial.begin(SERIAL_BAUD);
    Log.begin(STARTUP_LOG_LEVEL, &Serial, false);

    Log.trace("Starting board..." CR);

    // enable power for analog inputs (uses a high digital output as a 3V3 source)
    pinMode(local_adc_power, OUTPUT);
    digitalWrite(local_adc_power, HIGH);

    i2c_controller.begin();
    i2c_controller.setTimeout(100);

    Log.notice("Change log level with [s]ilent, [f]atal, [e]rror, [w]arning, [n]otice, [t]race, [v]erbose" CR);
    Log.setLevel(RUN_LOG_LEVEL);
}

void loop()
{
    // read local adc
    int local_adc_val = map(analogRead(local_adc), 0, 1023, -32767, 32767);

    ReadIOBoards();

    // set axes from io boards
    gamepad.SetY(bytes_to_int(io_board_data[1][4], io_board_data[1][5]));
    gamepad.SetX(bytes_to_int(io_board_data[1][6], io_board_data[1][7]));
    gamepad.SetZ(local_adc_val);
    gamepad.SetThrottle(bytes_to_int(io_board_data[0][4], io_board_data[0][5]));

    // set buttons from io boards
    set_button(0, 0, 2, 0); // SWA-Dn
    set_button(1, 0, 2, 1); // SWA-Up
    set_button(2, 0, 2, 2); // SWB-Dn
    set_button(3, 0, 2, 3); // SWB-Dn
    set_button(4, 0, 2, 4); // SWC-Dn
    set_button(5, 0, 2, 5); // SWC-Up
    set_button(6, 1, 2, 4); // SWD-Dn
    set_button(7, 1, 2, 5); // SWD-Up
    set_button(8, 1, 2, 6); // SWE-Dn
    set_button(9, 1, 2, 7); // SWE-Up
    set_button(10, 0, 2, 6); // Top Button
    set_button(11, 1, 2, 2); // Bottom Button
    set_button(12, 1, 2, 0); // Trig-1
    set_button(13, 1, 2, 1); // Trig-2
    set_button(14, 0, 1, 4); // Left Hat U
    set_button(15, 0, 1, 1); // Left hat R
    set_button(16, 0, 1, 2); // Left Hat D
    set_button(17, 0, 1, 3); // Left Hat L
    set_button(18, 0, 1, 0); // Left Hat Push
    set_button(19, 1, 1, 4); // Right Hat U
    set_button(20, 1, 1, 1); // Right Hat R
    set_button(21, 1, 1, 2); // Right Hat D
    set_button(22, 1, 1, 3); // Right Hat L
    set_button(23, 1, 1, 0); // Right Hat Push
    set_button(24, 1, 0, 4); // Thumb Hat U
    set_button(25, 1, 0, 1); // Thumb Hat R
    set_button(26, 1, 0, 2); // Thumb Hat D
    set_button(27, 1, 0, 3); // Thumb Hat L
    set_button(28, 1, 0, 0); // Thumb Hat Push
    set_button(29, 0, 0, 4); // Pinky Hat CW
    set_button(30, 0, 0, 1); // Pinky Hat CCW
    set_button(31, 0, 0, 0); // Pinky Hat Push
    set_button(32, 1, 3, 0); // Thumbstick Push


    gamepad.send_update();
    delay(1);
    CheckSerial();
    PeriodicFlash(3);
}

void set_button(uint8_t button, uint8_t board, uint8_t byte, uint8_t bit) {
    gamepad.SetButton(button, io_board_data[board][byte] & (1 << bit));
}

void ReadIOBoards()
{
    for (int board = 0; board < num_io_boards; board++)
    {
        Log.trace("Requesting from board %d...", board);
        i2c_controller.requestFrom(i2c_base_address + board, data_packet_size);
        Log.trace(" Done" CR);
        for (int i = 0; i < data_packet_size; i++)
        {
            if (i2c_controller.available())
            {
                io_board_data[board][i] = i2c_controller.read();
            }
            else
            {
                Log.error("No I2C data available for board %d byte %d" CR, board, i);
            }
        }
    }
}

int bytes_to_int(uint8_t msb, uint8_t lsb)
{
    int padding = msb & 0x80 ? (int)0xFFFF0000 : 0x00000000;
    return padding | (msb << 8) | lsb;
}

void CheckSerial()
{
    if (Serial.available() > 0)
    {
        char c = Serial.read();
        switch (c)
        {
        case 's':
            Log.setLevel(LOG_LEVEL_SILENT);
            break;
        case 'f':
            Log.setLevel(LOG_LEVEL_FATAL);
            break;
        case 'e':
            Log.setLevel(LOG_LEVEL_ERROR);
            break;
        case 'w':
            Log.setLevel(LOG_LEVEL_WARNING);
            break;
        case 'n':
            Log.setLevel(LOG_LEVEL_NOTICE);
            break;
        case 't':
            Log.setLevel(LOG_LEVEL_TRACE);
            break;
        case 'v':
            Log.setLevel(LOG_LEVEL_VERBOSE);
            break;
        default:
            return;
        }
        Log.fatal("Log level: %c" CR, c);
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
