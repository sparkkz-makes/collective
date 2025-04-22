#include <Arduino.h>
#include <ArduinoLog.h>
#include <PicoGamepad.h>
#include <Wire.h>

#define SERIAL_BAUD 115200
#define STARTUP_LOG_LEVEL  LOG_LEVEL_SILENT
#define RUN_LOG_LEVEL  LOG_LEVEL_SILENT

static const uint8_t i2c_base_address = 8;
static const uint8_t num_io_boards = 2;
static const uint8_t data_packet_size = 8;
static const uint8_t buttons_per_board = 9;
static const uint8_t as5600_i2c_addr = 0x36;
static const uint8_t as5600_angle_register = 0x0e;
static const uint8_t as5600_raw_angle_register = 0x0c;
static const uint8_t as5600_zmco_register = 0x00;
static const uint8_t as5600_zpos_register = 0x01;
static const uint8_t as5600_mpos_register = 0x03;
static const uint8_t as5600_burn_register = 0xff;
static const uint8_t as5600_burn_command = 0x80;
static const uint8_t as5600_dir_pin = 22;
uint8_t io_board_data[num_io_boards][data_packet_size]; // 2 io boards, 8 bytes per packet

MbedI2C i2c_controller_0(0, 1); // sda, scl
MbedI2C i2c_controller_1(26, 27); // sda, scl
PicoGamepad gamepad;

void ReadIOBoards();
void ReadLocalInputs();
void CheckSerial();
void SetLogLevel(int logLevel, char c);
void set_button(uint8_t button, uint8_t board, uint8_t byte, uint8_t bit);
int bytes_to_int(uint8_t msb, uint8_t lsb);
void PeriodicFlash(uint8_t flashes);
void SetAS5600AngleRegister();
int ReadAs5600Angle();
void AS5600_ReadRegisters();
void AS5600_SetZero();
void AS5600_SetMax();
void AS5600_Burn();
uint16_t AS5600ReadWordRegister(uint8_t reg);
uint8_t AS5600ReadByteRegister(uint8_t reg);
void AS5600_WriteWordRegister(uint8_t reg, uint16_t value);
void AS5600_WriteByteRegister(uint8_t reg, uint8_t value);


void setup()
{
    Serial.begin(SERIAL_BAUD);
    Log.begin(STARTUP_LOG_LEVEL, &Serial, false);

    Log.trace("Starting board..." CR);

    i2c_controller_0.begin();
    i2c_controller_1.begin();
    i2c_controller_0.setTimeout(100);
    i2c_controller_1.setTimeout(100);

    // enable as5600 dir pin so we can set it either high or low
    pinMode(as5600_dir_pin, OUTPUT);
    digitalWrite(as5600_dir_pin, LOW);

    Log.notice("Change log level with [s]ilent, [f]atal, [e]rror, [w]arning, [n]otice, [t]race, [v]erbose" CR);
    Log.setLevel(RUN_LOG_LEVEL);

}

void loop()
{
    // read AS5600
    int angle = ReadAs5600Angle();

    ReadIOBoards();

    // set axes from io boards
    gamepad.SetY(bytes_to_int(io_board_data[1][4], io_board_data[1][5]));
    gamepad.SetX(bytes_to_int(io_board_data[1][6], io_board_data[1][7]));
    gamepad.SetZ(angle);
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

uint16_t AS5600ReadWordRegister(uint8_t reg){
    i2c_controller_1.beginTransmission(as5600_i2c_addr);
    i2c_controller_1.write(reg);
    i2c_controller_1.endTransmission();
    i2c_controller_1.requestFrom(as5600_i2c_addr, (uint8_t)2);
    uint16_t msb = i2c_controller_1.read();
    uint16_t lsb = i2c_controller_1.read();
    int value = (msb << 8) + lsb;
    Log.verbose("Read Register - Addr %X reg %X : %X, %X : %d" CR, as5600_i2c_addr, reg, msb, lsb, value);
    return value;
}

uint8_t AS5600ReadByteRegister(uint8_t reg){
    i2c_controller_1.beginTransmission(as5600_i2c_addr);
    i2c_controller_1.write(reg);
    i2c_controller_1.endTransmission();
    i2c_controller_1.requestFrom(as5600_i2c_addr, (uint8_t)1);
    uint8_t data = i2c_controller_1.read();
    return data;
}

void AS5600_WriteWordRegister(uint8_t reg, uint16_t value) {
    i2c_controller_1.beginTransmission(as5600_i2c_addr);
    i2c_controller_1.write(reg);
    i2c_controller_1.write(value >> 8);
    i2c_controller_1.write(value & 0xff);
    if (int e = i2c_controller_1.endTransmission() > 0) {
        Log.error("ERROR %d writing word %X to register %X" CR, e, value, reg);
    }
}

void AS5600_WriteByteRegister(uint8_t reg, uint8_t value) {
    i2c_controller_1.beginTransmission(as5600_i2c_addr);
    i2c_controller_1.write(reg);
    i2c_controller_1.write(value);
    if (int e = i2c_controller_1.endTransmission() > 0) {
        Log.error("ERROR %d writing byte %X to register %X" CR, e, value, reg);
    }
}


int ReadAs5600Angle() {
    int angle = AS5600ReadWordRegister(as5600_angle_register) & 0x0FFF;
    int scaled_angle = map(angle, 0, 4096, -32767, 32767);
    Log.verbose("Register value: %d Scaled: %d" CR, angle, scaled_angle);
    return scaled_angle;
}

void set_button(uint8_t button, uint8_t board, uint8_t byte, uint8_t bit) {
    gamepad.SetButton(button, io_board_data[board][byte] & (1 << bit));
}

void ReadIOBoards()
{
    for (int board = 0; board < num_io_boards; board++)
    {
        Log.verbose("Requesting from board %d...", board);
        i2c_controller_0.requestFrom(i2c_base_address + board, data_packet_size);
        Log.verbose(" Done" CR);
        for (int i = 0; i < data_packet_size; i++)
        {
            if (i2c_controller_0.available())
            {
                io_board_data[board][i] = i2c_controller_0.read();
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
            SetLogLevel(LOG_LEVEL_SILENT, c);
            break;
        case 'f':
            SetLogLevel(LOG_LEVEL_FATAL, c);
            break;
        case 'e':
            SetLogLevel(LOG_LEVEL_ERROR, c);
            break;
        case 'w':
            SetLogLevel(LOG_LEVEL_WARNING, c);
            break;
        case 'n':
            SetLogLevel(LOG_LEVEL_NOTICE, c);
            break;
        case 't':
            SetLogLevel(LOG_LEVEL_TRACE, c);
            break;
        case 'v':
            SetLogLevel(LOG_LEVEL_VERBOSE, c);
            break;
        case 'R':
            AS5600_ReadRegisters();
            break;
        case 'Z':
            AS5600_SetZero();
            break;
        case 'M':
            AS5600_SetMax();
            break;
        case 'B':
            AS5600_Burn();
            break;
        default:
            Log.fatal("%c", c);
            return;
        }
    }
}

void SetLogLevel(int logLevel, char c) {
    Log.setLevel(logLevel);
    Log.fatal("Log level: %c" CR, c);
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
        delay = flash == 0 ? 2000 : 100;
        last_flash_millis = millis();
    }
}

void AS5600_ReadRegisters() {
    int angle = AS5600ReadWordRegister(as5600_angle_register) & 0x0FFF;
    int raw = AS5600ReadWordRegister(as5600_raw_angle_register) & 0x0FFF;
    int zpos = AS5600ReadWordRegister(as5600_zpos_register) & 0x0FFF;
    int mpos = AS5600ReadWordRegister(as5600_mpos_register) & 0x0FFF;
    int zmco = AS5600ReadByteRegister(as5600_zmco_register) & 0x0FFF;
    Log.notice("Angle: %d Raw: %d ZPOS: %d MPOS: %d ZMCO: %d" CR, angle, raw, zpos, mpos, zmco);
}

void AS5600_SetZero() {
    int raw = AS5600ReadWordRegister(as5600_raw_angle_register) & 0x0FFF;
    AS5600_WriteWordRegister(as5600_zpos_register, raw);
    Log.notice("Written raw value %d to ZPOS" CR, raw);
}

void AS5600_SetMax() {
    int raw = AS5600ReadWordRegister(as5600_raw_angle_register) & 0x0FFF;
    AS5600_WriteWordRegister(as5600_mpos_register, raw);
    Log.notice("Written raw value %d to MPOS" CR, raw);
}

void AS5600_Burn() {
    AS5600_WriteByteRegister(as5600_burn_register, as5600_burn_command);
    Log.notice("Burn angle completed. Verifying..." CR);
    delay(2);
    AS5600_WriteByteRegister(as5600_burn_register, 0x01);
    AS5600_WriteByteRegister(as5600_burn_register, 0x11);
    AS5600_WriteByteRegister(as5600_burn_register, 0x10);
    AS5600_ReadRegisters();
}