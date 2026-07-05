# RP2040 Touch LCD Native Blob Demo

This repository contains a PlatformIO project for the Waveshare RP2040-Touch-LCD-1.28 board, focused on native-driver rendering performance.

## Root Layout

- `driver/`: Native Waveshare driver code, organized by hardware part/chip
- `examples/`: Reference examples, including the stock Waveshare demo
- `src/`: Main application entry point and app-level logic
- `platformio.ini`: Build/upload configuration

## Driver Layout

- `driver/board/`: Board HAL (`DEV_Config`, GPIO/SPI/I2C/ADC/PWM/timing)
- `driver/display/`: LCD + native paint stack
- `driver/display/fonts/`: Font tables used by the native display stack
- `driver/sensors/imu_qmi8658/`: QMI8658 IMU driver
- `driver/sensors/touch_cst816s/`: CST816S touch driver

Each driver section has a short local README with details.

## Source Layout

- `src/touch_lcd_demo.ino`: Main firmware entry point
- `src/platform/waveshare_native_board.*`: Platform adapter layer that wraps driver calls
- `src/apps/blob_native/blob_native_app.*`: Blob application logic

This keeps hardware-facing code separated from app logic so future sketches can reuse the same platform layer.

## Example Layout

- `examples/waveshare_native/waveshare_stock_demo.ino`: Stock-style Waveshare Arduino demo reference
- `examples/waveshare_native/LCD_Test.h`: Original demo helper header

## Requirements

- PlatformIO CLI (`pio`) or PlatformIO IDE extension
- USB data cable
- Waveshare RP2040-Touch-LCD-1.28 board

## Build

```bash
pio run
```

## Upload

```bash
pio run -t upload
```

If needed, enter BOOTSEL mode manually before upload:

1. Hold `BOOTSEL`
2. Plug USB
3. Release `BOOTSEL`
4. Run upload again

## Serial Monitor

```bash
pio device monitor -b 115200
```
