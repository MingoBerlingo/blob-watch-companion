#include "apps/blob_native/shared_state.h"

namespace blob_native
{

    BlobState g_blob_state = {0.0f};

    int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi)
    {
        if (v < lo)
            return lo;
        if (v > hi)
            return hi;
        return v;
    }

} // namespace blob_native