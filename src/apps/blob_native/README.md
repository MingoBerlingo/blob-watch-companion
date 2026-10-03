# Blob Native App Architecture

This folder contains the raw blob renderer plus an LVGL-based timer screen and a page manager that switches between the two.

## File Map

- `app/app.cpp` / `app/app.h`
  - App entry points: init board, LVGL, and page manager; run the loop.

- `lv_port/`
  - `lv_conf.h` - LVGL v8.3 config (RGB565 + color swap, 240x240, minimal).
  - `lv_port.h` / `lv_port.cpp` - display flush_cb, touch input driver, tick.

- `pages/`
  - `page_manager.h` / `page_manager.cpp` - page registry + swipe Left/Right
    navigation between the blob and timer pages.
  - `blob_page.h` / `blob_page.cpp` - wraps the raw blob render loop.
  - `timer/timer_page.h` / `timer_page.cpp` - LVGL screen (arc, labels, buttons).

- `shared_state.h` / `shared_state.cpp`
  - Shared constants and lightweight runtime state

- `blob/view.h`, `blob/logic.cpp`, `blob/render.cpp`
  - Blob screen frame preparation and rendering

- `blob/shape.h` / `blob/shape.cpp`
  - Blob contour generation
  - Glow, fill, outline drawing
  - Blob bounds helpers

- `blob/face.h` / `blob/face.cpp`
  - Eyes and mouth drawing
  - Face bounds helpers

- `blob/overlay.h` / `blob/overlay.cpp`
  - Optional performance overlay (FPS + dirty window size)
  - Drawn only on overlay refresh ticks

- `timer/view.h`, `timer/logic.cpp`, `timer/internal.h`
  - Timer state machine + accessors; rendering is handled by the LVGL page.

- `ui/draw.h` / `ui/draw.cpp`
  - Direct raster primitives used by the blob renderer.

## Render Path

- Blob page: raw framebuffer rendering (unchanged), capped at ~60 fps.
- Timer page: `page_manager_loop` drives `lv_timer_handler`; LVGL renders into
  a partial buffer, `flush_cb` copies it into the native framebuffer
  (byte-swapped RGB565) and pushes dirty windows over SPI.

## Main Tuning Knobs

- Geometry: `BLOB_RADIUS`, `POINTS`, wave amplitudes
- Glow: `GLOW_LAYER_COUNT`, `GLOW_LAYER_SCALE[]`, `GLOW_LAYER_COLOR[]`
- Face: `EYE_*`, `MOUTH_*`, `FACE_DIR_X`, `FACE_DIR_Y`
- Idle animation: `FACE_IDLE_*`
- Dirty update: `DIRTY_MARGIN`

## Notes

- Colors are specified as the project's native RGB565 values and converted via
  `lv_rgb565()`; `LV_COLOR_16_SWAP 1` matches the panel's BGR + byte-swap order.
- The blob screen remains raw for now and can be migrated to a custom LVGL draw
  event in a future change.
