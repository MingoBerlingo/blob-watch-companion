# Board Layer (HAL)

This part wraps RP2040 board peripherals used by higher-level drivers:

- Pin configuration
- SPI bus setup and transfer helpers
- I2C bus setup and register read/write helpers
- ADC readout
- PWM backlight control
- Timing utilities

Files here are hardware-facing and shared by display/touch/IMU drivers.
