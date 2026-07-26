# Blob Native App Architecture

This folder contains the blob demo split into focused modules.

## File Map

- `blob_native_app.cpp`
  - High-level orchestrator (`setup` and `loop`)
  - Runs physics update, chooses render path, and presents final window

- `blob_native_state.h` / `blob_native_state.cpp`
  - Shared constants, color/style settings, and runtime state
  - Global state holder: `g_blob_state`

- `blob_native_blob.h` / `blob_native_blob.cpp`
  - Blob geometry generation
  - Blob outline and glow draw functions
  - Blob and glow bounds helpers

- `blob_native_face.h` / `blob_native_face.cpp`
  - Face rendering (eyes and mouth)
  - Blink and idle motion helpers
  - Face bounds helpers

- `blob_native_framebuffer.h` / `blob_native_framebuffer.cpp`
  - Dirty-rectangle helpers (`Rect`, union, width/height)
  - Save/restore background region for fast redraw

## Render Paths

There are two render paths:

1. Save/restore path (preferred)
   - Restore previous background region
   - Save current background region
   - Draw current blob + face
   - Present union of previous and current dirty rectangles

2. Geometry-erase fallback
   - Used if backup buffer allocation fails
   - Erases previous shape by overdrawing in background color
   - Draws current shape
   - Presents computed dirty rectangle

## Per-Frame Flow

1. Read IMU tilt
2. Update target and velocity
3. Update blob center
4. Compute new blob contour (`POINTS` vertices)
5. Compute current dirty rectangle
6. Render through save/restore path or fallback path
7. Store current contour as previous frame state
8. Advance phase timer (`phase_t`)

## Face Orientation

Face orientation is intentionally fixed with:

- `FACE_DIR_X`
- `FACE_DIR_Y`

This keeps eyes/mouth facing one direction regardless of blob movement.

## Main Tuning Knobs

Shape and motion:

- `BLOB_RADIUS`
- `POINTS`
- velocity blend factors in `blob_native_app.cpp`

Glow:

- `GLOW_LAYER_COUNT`
- `GLOW_LAYER_SCALE[]`
- `GLOW_LAYER_COLOR[]`

Face:

- `EYE_RADIUS`
- `EYE_FORWARD`
- `EYE_SIDE`
- `MOUTH_FORWARD`
- `MOUTH_HALF_LEN`
- `MOUTH_SMILE_DEPTH`

Idle/blink:

- `FACE_IDLE_SPEED_MAX`
- `FACE_IDLE_EYE_BOB`
- `FACE_IDLE_MOUTH_BOB`
- `FACE_IDLE_BLINK_RATE`
- `FACE_IDLE_BLINK_THRESHOLD`

Dirty area and backup:

- `DIRTY_MARGIN`
- `BACKUP_SIDE`
- `BACKUP_PIXELS`

## Notes for Future Changes

- If you add new animated facial features, update both draw and bounds helpers in `blob_native_face.cpp`.
- If new visuals extend beyond current geometry, ensure dirty rectangle calculation includes them.
- Keep module boundaries strict (app flow vs geometry vs face vs framebuffer) to avoid file growth again.
