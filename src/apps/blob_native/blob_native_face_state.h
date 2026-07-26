#ifndef BLOB_NATIVE_FACE_STATE_H
#define BLOB_NATIVE_FACE_STATE_H

#include <stdint.h>

#include "apps/blob_native/blob_native_state.h"

namespace blob_native
{

    void face_anim_reset(FaceAnimState *state);
    void face_anim_update(FaceAnimState *state, bool shake_event, bool click_event, uint16_t dt_ms);

} // namespace blob_native

#endif
