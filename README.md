# RP2040 Touch LCD Demo (PlatformIO)

This repository contains a PlatformIO-ready firmware project for the Waveshare RP2040-Touch-LCD-1.28 development board.

## Project Layout

- `firmware/`: Main Arduino-based firmware sources (LCD, touch, IMU, fonts, and app logic)
- `platformio.ini`: PlatformIO build/upload configuration

## Requirements

- PlatformIO CLI (`pio`) or PlatformIO IDE extension
- USB cable with data support
- Waveshare RP2040-Touch-LCD-1.28 board

## Build

From the repository root:

```bash
pio run
```

## Upload

From the repository root:

```bash
pio run -t upload
```

If automatic reset is not available, put the board in BOOTSEL mode manually:

1. Hold the `BOOTSEL` button.
2. Plug in the USB cable.
3. Release `BOOTSEL`.
4. Run `pio run -t upload` again.

## Serial Monitor

```bash
pio device monitor -b 115200
```

## Main Entry Point

The main sketch file is:

- `firmware/touch_lcd_demo.ino`

## Notes

- This repository is optimized for Arduino + PlatformIO workflow.
- The PlatformIO configuration uses Arduino-Pico via a community RP2040 platform package for full compatibility with `SPI1` and `Wire1`.
