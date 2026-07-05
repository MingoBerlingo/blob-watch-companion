# Driver Folder

This folder contains the native Waveshare board drivers, organized by hardware responsibility.

- `board/`: Board-level HAL (GPIO, SPI, I2C, ADC, backlight, delays)
- `display/`: Display driver and native drawing stack
- `sensors/`: Sensor drivers grouped by chip

Application code should not implement low-level peripheral access directly; use these drivers through a platform/app abstraction.
