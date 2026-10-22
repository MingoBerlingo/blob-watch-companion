# Blob Native App Architecture

This folder contains LVGL-based blob and timer screens plus a page manager that switches between the two.

## File Map

- `app/app.cpp` / `app/app.h`
  - App entry points: init board, LVGL, and page manager; run the loop.

- `lv_port/`
  - `lv_conf.h` - LVGL v8.3 config (RGB565 + color swap, 240x240, minimal).
  - `lv_port.h` / `lv_port.cpp` - display flush_cb, touch input driver, tick.

- `pages/`
  - `page_manager.h` / `page_manager.cpp` - page registry + swipe Left/Right
    navigation between the blob and timer pages.
  - `blob/blob_page.h` / `blob/blob_page.cpp` - blob LVGL screen (custom draw event + animation timer).
  - `timer/timer_page.h` / `timer_page.cpp` - LVGL timer setup, running, paused and ended views.

- `blob/`
  - `shape.h` / `shape.cpp` - blob contour generation + glow scaling.
  - `face.h` / `face.cpp` - eye/mouth positions and blink amount.

- `timer/`
  - `view.h` / `logic.cpp` / `internal.h` - timer state machine + accessors;
    rendering is handled by the LVGL page.
  - `runtime_draw.h` / `runtime_draw.cpp` - continuous progress ring and geometric action icons.

- `shared_state.h`
  - Shared geometry/color constants.

## Render Path

Both pages are LVGL screens. `page_manager_loop` drives `lv_timer_handler`;
LVGL renders into a partial buffer, `flush_cb` copies it into the native
framebuffer (byte-swapped RGB565) and pushes dirty windows over SPI.

- Blob: a 33 ms LVGL timer advances the animation and invalidates a full-screen
  object; its `LV_EVENT_DRAW_MAIN` handler draws glow/outline/face with LVGL
  draw primitives (reusing the geometry in `blob/`).
- Timer: LVGL labels/buttons, a continuous LVGL progress arc and primitive icon drawing.

## Main Tuning Knobs

- Geometry: `BLOB_RADIUS`, `POINTS`, wave amplitudes
- Glow: `GLOW_LAYER_COUNT`, `GLOW_LAYER_SCALE[]`, `GLOW_LAYER_COLOR[]`
- Face: `EYE_*`, `MOUTH_*`, `FACE_DIR_X`, `FACE_DIR_Y`
- Idle animation: `FACE_IDLE_*`

## Notes

- Colors are specified as the project's native RGB565 values and converted via
  `lv_rgb565()`; `LV_COLOR_16_SWAP 1` matches the panel's BGR + byte-swap order.

## Timer Runtime Flow

The duration-selection screen is unchanged. After Start, the running view shows
remaining time, a continuous progress ring using the existing theme violet, and an orange
pause button. Pause samples the exact remaining duration and freezes the timer.
Its three actions are gray back to Blob (retain pause), red cancel to setup, and
green resume. Reentering a paused timer restores its action view.

At expiry, the ring/actions are replaced by the original duration, `Timer ended`,
a gray back-to-setup button and an orange repeat button. Expiry while visiting
another page does not steal focus; reentering Timer shows completion.

Icons are code-native LVGL primitives: pause bars, a play triangle, chevron,
cross, and repeat arc/arrow. They need no bitmap assets, icon font or SVG decoder.
Use images for detailed artwork; basic shapes keep these symbols small and sharp.

Action colors are named RGB565 constants in `shared_state.h`, selected from
https://github.com/newdigate/rgb565_colors/blob/main/README.md : Orange (web),
Screamin Green, Fiery Rose, Silver and Davy's gray. The ring reuses the current
primary theme color; black/white reuse the existing palette.

The desktop simulator runs this same code. Its timer scenarios cover precise
pause/resume, back/return, cancel, expiry/repeat and the ended back action;
`tests/test_timer_logic.py` also checks millis rollover and background expiry.
