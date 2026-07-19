# Driver Folder

This folder contains the native Waveshare board drivers, organized by hardware responsibility.

- [board/README.md](board/README.md): Board-level HAL (GPIO, SPI, I2C, ADC, backlight, delays)
- [display/README.md](display/README.md): Display driver and native drawing stack
- [sensors/imu_qmi8658/README.md](sensors/imu_qmi8658/README.md): QMI8658 IMU driver
- [sensors/touch_cst816s/README.md](sensors/touch_cst816s/README.md): CST816S touch driver

Application code should not implement low-level peripheral access directly; use these drivers through a platform/app abstraction.
