#include "apps/blob_native/blob_native_state.h"

namespace blob_native
{

    BlobState g_blob_state = {
        CENTER_X,
        CENTER_Y,
        0.0f,
        0.0f,
        FACE_DIR_X,
        FACE_DIR_Y,
        0.0f,
        0.0f,
        {EyesAnimState::Idle, MouthAnimState::Neutral, 0, 0, FACE_BLINK_INTERVAL_MIN_MS, false},
        nullptr,
        {0, 0, -1, -1},
        false,
        false,
        {0},
        {0},
        true};

    int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi)
    {
        if (v < lo)
            return lo;
        if (v > hi)
            return hi;
        return v;
    }

} // namespace blob_native