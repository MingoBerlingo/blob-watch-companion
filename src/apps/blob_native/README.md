# Blob Native App Architecture

This folder contains the simplified, stable blob renderer.

## File Map

- `blob_native_app.cpp`
  - App setup + frame loop
  - IMU-driven motion update (optional)
  - Dirty-window clear/draw/present

- `blob_native_state.h` / `blob_native_state.cpp`
  - Shared constants and lightweight runtime state

- `blob_native_blob.h` / `blob_native_blob.cpp`
  - Blob contour generation
  - Glow, fill, outline drawing
  - Blob bounds helpers

- `blob_native_face.h` / `blob_native_face.cpp`
  - Eyes and mouth drawing
  - Face bounds helpers

- `blob_native_draw.h` / `blob_native_draw.cpp`
  - Direct raster primitives on framebuffer

- `blob_native_overlay.h` / `blob_native_overlay.cpp`
  - Optional performance overlay (FPS + dirty window size)
  - Drawn only on overlay refresh ticks

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
