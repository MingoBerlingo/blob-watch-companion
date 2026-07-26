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

    // Color palette (RGB565).
    constexpr uint16_t BLOB_COLOR = 0x0008;
    constexpr uint16_t OUTLINE_COLOR = BLUE;
    constexpr uint16_t BG_COLOR = BLACK;
    constexpr uint16_t GUIDE_COLOR = GRAY;
    constexpr uint16_t EYE_COLOR = CYAN;
    constexpr uint16_t MOUTH_COLOR = CYAN;

    // Feature toggle: fill blob interior in addition to outline/glow.
    constexpr bool BLOB_FILL_ENABLED = false;

    // Glow styling.
    constexpr int GLOW_LAYER_COUNT = 3;
    constexpr float GLOW_LAYER_SCALE[GLOW_LAYER_COUNT] = {1.3f, 1.2f, 1.1f};
    constexpr uint16_t GLOW_LAYER_COLOR[GLOW_LAYER_COUNT] = {0x0008, 0x082b, 0x106f};

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

    // Dirty-rect and backup sizing.
    constexpr float MAX_BLOB_EXTENT = (BLOB_RADIUS + 16.0f) * GLOW_LAYER_SCALE[0];
    constexpr int DIRTY_MARGIN = 8;
    constexpr int BACKUP_SIDE = (int)(MAX_BLOB_EXTENT * 2.0f) + DIRTY_MARGIN * 2 + 4;
    constexpr int BACKUP_PIXELS = BACKUP_SIDE * BACKUP_SIDE;

    struct Rect
    {
        int16_t x0;
        int16_t y0;
        int16_t x1;
        int16_t y1;
    };

    // Runtime state updated once per frame.
    struct BlobState
    {
        float blob_x;
        float blob_y;
        float vel_x;
        float vel_y;
        float eye_dir_x;
        float eye_dir_y;
        float face_phase_prev;
        float phase_t;

        uint16_t *saved_region;
        Rect saved_rect;
        bool has_saved_region;
        bool use_saved_region;

        int16_t px_old[POINTS];
        int16_t py_old[POINTS];
        bool first_frame;
    };

    extern BlobState g_blob_state;

    int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi);

} // namespace blob_native

#endif