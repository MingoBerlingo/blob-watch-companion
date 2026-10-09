#ifndef BLOB_NATIVE_STATE_H
#define BLOB_NATIVE_STATE_H

#include <stdint.h>

#include "GUI_Paint.h"

namespace blob_native
{

    // Display and blob geometry constants.
    constexpr int SCREEN_W = 240;
    constexpr int SCREEN_H = 240;
    constexpr float CENTER_X = SCREEN_W / 2.0f;
    constexpr float CENTER_Y = SCREEN_H / 2.0f;
    constexpr float BLOB_RADIUS = 25.0f;
    constexpr int POINTS = 48;

    // Blob contour wave amplitudes (main body curvature tuning).
    constexpr float BLOB_WAVE_AMP_1 = 2.1f;
    constexpr float BLOB_WAVE_AMP_2 = 1.8f;
    constexpr float BLOB_WAVE_AMP_3 = 1.5f;

    // Background color (RGB565). Accents come from the LVGL theme.
    constexpr uint16_t BG_COLOR = BLACK;

    // Respond to IMU tilt.
    constexpr bool BLOB_USE_IMU = true;

    // Glow styling (disabled by default for a crisper, faster outline).
    constexpr bool BLOB_GLOW_ENABLED = false;
    constexpr int GLOW_LAYER_COUNT = 3;
    constexpr float GLOW_LAYER_SCALE[GLOW_LAYER_COUNT] = {1.3f, 1.2f, 1.1f};
    constexpr uint16_t GLOW_LAYER_COLOR[GLOW_LAYER_COUNT] = {0x4008, 0x8010, 0xC018};

    // Lightweight performance overlay (FPS).
    constexpr bool BLOB_PERF_OVERLAY_ENABLED = false;
    constexpr int16_t BLOB_PERF_OVERLAY_X = 4;
    constexpr int16_t BLOB_PERF_OVERLAY_Y = 4;
    constexpr uint16_t BLOB_PERF_OVERLAY_UPDATE_MS = 250;

    // Face styling.
    constexpr int16_t EYE_RADIUS = 1;
    constexpr float EYE_FORWARD = BLOB_RADIUS * 0.28f;
    constexpr float EYE_SIDE = BLOB_RADIUS * 0.15f;
    constexpr float MOUTH_FORWARD = -BLOB_RADIUS * 0.08f;
    constexpr float MOUTH_HALF_LEN = BLOB_RADIUS * 0.16f;
    constexpr float MOUTH_SMILE_DEPTH = BLOB_RADIUS * 0.05f;

    // Face is locked to a fixed orientation (no rotation with motion).
    constexpr float FACE_DIR_X = 0.0f;
    constexpr float FACE_DIR_Y = -1.0f;

    // Idle animation tuning.
    constexpr float FACE_IDLE_SPEED_MAX = 3.0f;
    constexpr float FACE_IDLE_EYE_BOB = BLOB_RADIUS * 0.03f;
    constexpr float FACE_IDLE_MOUTH_BOB = BLOB_RADIUS * 0.04f;
    constexpr float FACE_IDLE_BLINK_RATE = 0.85f;
    constexpr float FACE_IDLE_BLINK_THRESHOLD = 0.965f;

} // namespace blob_native

#endif
