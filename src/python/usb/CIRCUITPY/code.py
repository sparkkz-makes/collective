# import board support libraries, including HID.
import board
import digitalio
import analogio
import usb_hid

from time import sleep

# Libraries for communicating as a Keyboard device
from adafruit_hid.keyboard import Keyboard
from adafruit_hid.keyboard_layout_us import KeyboardLayoutUS
from adafruit_hid.keycode import Keycode

from hid_collective import Collective

from adafruit_hid.mouse import Mouse

mouse = Mouse(usb_hid.devices)

keyboard = Keyboard(usb_hid.devices)
layout = KeyboardLayoutUS(keyboard)
collective = Collective(usb_hid.devices)

# Setup for Analog Joystick as X and Y
ax = analogio.AnalogIn(board.GP27)
ay = analogio.AnalogIn(board.GP26)

btn = digitalio.DigitalInOut(board.GP28)
btn.direction = digitalio.Direction.INPUT
btn.pull = digitalio.Pull.UP

algpwr = digitalio.DigitalInOut(board.GP22)
algpwr.direction = digitalio.Direction.OUTPUT
algpwr.value = True

print("Starting")

while True:
    for i in range(0,32):
        collective.release_all_buttons();
        collective.press_buttons(i+1)

        w = i-16

        x = w * 1985
        y = (w+15)%16 * 1985
        z = - x
        rz = - y

        collective.move_axis()

        joyX = (ax.value - 0x7FFF)
        joyY = (ay.value - 0x7FFF)
        print(f"{x}, {y}, {z}, {rz}")

        collective.move_axis(x,y,z,rz)


        sleep(0.2)

