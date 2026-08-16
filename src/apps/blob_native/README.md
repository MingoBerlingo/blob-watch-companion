# Blob Native App Architecture

This folder contains the simplified, stable blob renderer.

## File Map

- `app/app.cpp` / `app/app.h`
  - App setup entrypoints
  - Delegates frame work to screen logic

- `screen_logic.cpp` / `screen_logic.h`
  - Section switching between blob and timer
  - Timer screen partial/full redraw policy

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

- `timer/view.h`, `timer/logic.cpp`, `timer/render.cpp`, `timer/internal.h`
  - Timer screen state, gestures, and rendering

- `ui/draw.h` / `ui/draw.cpp`
  - Direct raster primitives on framebuffer

- `ui/button.h` / `ui/button.cpp`
  - Reusable rectangular and circular button shells + hit-testing

## Render Path

Single stable path per frame:

1. Update center/velocity (IMU optional)
2. Compute blob contour points
3. Compute dirty rectangle from current + previous geometry
4. Clear dirty rectangle to background
5. Draw glow + fill + outline + face
6. Present full frame or dirty window (configurable)

## Main Tuning Knobs

- Geometry: `BLOB_RADIUS`, `POINTS`, wave amplitudes
- Glow: `GLOW_LAYER_COUNT`, `GLOW_LAYER_SCALE[]`, `GLOW_LAYER_COLOR[]`
- Face: `EYE_*`, `MOUTH_*`, `FACE_DIR_X`, `FACE_DIR_Y`
- Idle animation: `FACE_IDLE_*`
- Dirty update: `DIRTY_MARGIN`

## Notes

- Keep draw and bounds logic in sync when adding visual features.
- Keep this path branch-light; avoid reintroducing legacy fallback trees.
