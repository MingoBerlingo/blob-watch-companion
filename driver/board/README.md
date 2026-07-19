# Board Layer (HAL)

This part wraps RP2040 board peripherals used by higher-level drivers:

- Pin configuration
- SPI bus setup and transfer helpers
- I2C bus setup and register read/write helpers
- ADC readout
- PWM backlight control
- Timing utilities

Files here are hardware-facing and shared by display/touch/IMU drivers.

Related driver docs:

- [../display/README.md](../display/README.md): LCD stack and drawing primitives
- [../sensors/imu_qmi8658/README.md](../sensors/imu_qmi8658/README.md): QMI8658 IMU driver
- [../sensors/touch_cst816s/README.md](../sensors/touch_cst816s/README.md): CST816S touch driver
