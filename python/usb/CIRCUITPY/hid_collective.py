import struct
import time

from adafruit_hid import find_device


class Collective:

    def __init__(self, devices):
        self._collective_device = find_device(devices, usage_page=0x1, usage=0x04)

        # Reuse this bytearray to send mouse reports.
        # Typically controllers start numbering buttons at 1 rather than 0.
        # report[0] buttons 1-8 (LSB is button 1)
        # report[1] buttons 9-16
        # report[2] buttons 17-24
        # report[3] buttons 24-32
        # report[4] buttons 33 + 7 bits padding
        # report[5,6] thumbstick - X: -32768 to 32767
        # report[7,8] thumbstick - Y: -32768 to 32767
        # report[9,10] collective - Z: -32768 to 32767
        # report[11,12] throttle - Rz: -32768 to 32767
        self._report = bytearray(13)

        # Remember the last report as well, so we can avoid sending
        # duplicate reports.
        self._last_report = bytearray(13)

        # Store settings separately before putting into report. Saves code
        # especially for buttons.
        self._buttons_state = 0
        self._x_axis = 0
        self._y_axis = 0
        self._z_axis = 0
        self._rz_axis = 0

        # Send an initial report to test if HID device is ready.
        # If not, wait a bit and try once more.
        try:
            self.reset_all()
        except OSError:
            time.sleep(1)
            self.reset_all()

    def press_buttons(self, *buttons):
        """Press and hold the given buttons."""
        for button in buttons:
            self._buttons_state |= 1 << self._validate_button_number(button) - 1
        self._send()

    def release_buttons(self, *buttons):
        """Release the given buttons."""
        for button in buttons:
            self._buttons_state &= ~(1 << self._validate_button_number(button) - 1)
        self._send()

    def release_all_buttons(self):
        """Release all the buttons."""

        self._buttons_state = 0
        self._send()

    def click_buttons(self, *buttons):
        """Press and release the given buttons."""
        self.press_buttons(*buttons)
        self.release_buttons(*buttons)

    def move_axis(self, x=None, y=None, z=None, rz=None):
        if x is not None:
            self._x_axis = self._validate_axis_value(x)
        if y is not None:
            self._y_axis = self._validate_axis_value(y)
        if z is not None:
            self._z_axis = self._validate_axis_value(z)
        if rz is not None:
            self._rz_axis = self._validate_axis_value(rz)
        self._send()

    def reset_all(self):
        """Release all buttons and set joysticks to zero."""
        self._buttons_state = 0
        self._x_axis = 0
        self._y_axis = 0
        self._z_axis = 0
        self._rz_axis = 0
        self._send(always=True)

    def _send(self, always=False):
        """Send a report with all the existing settings.
        If ``always`` is ``False`` (the default), send only if there have been changes.
        """
        buttons = self._buttons_state.to_bytes(5, 'little')
        struct.pack_into(
            "<BBBBBhhhh",
            self._report,
            0,
            buttons[0],
            buttons[1],
            buttons[2],
            buttons[3],
            buttons[4],
            self._x_axis,
            self._y_axis,
            self._z_axis,
            self._rz_axis
        )

        if always or self._last_report != self._report:
            self._collective_device.send_report(self._report)
            # Remember what we sent, without allocating new storage.
            self._last_report[:] = self._report

    @staticmethod
    def _validate_button_number(button):
        if not 1 <= button <= 33:
            raise ValueError("Button number must in range 1 to 16")
        return button

    @staticmethod
    def _validate_axis_value(value):
        if not -32768 <= value <= 32767:
            raise ValueError("Axis value must be in range -32768 to 32767")
        return value