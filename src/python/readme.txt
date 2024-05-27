Python no longer used for this project - just leaving this here for reference.

This demonstrates how to set up the Pi Pico as a USB joystick device using Python. However Python just makes life difficult doing lower level stuff like this. In particular, support for I2C comminucation (particularly as a slave device) is severely lacking.

Hence pivot back to developing with C++ using Arduino framework and Platform IO