#ifndef BLOB_NATIVE_SCREEN_MANAGER_H
#define BLOB_NATIVE_SCREEN_MANAGER_H

#include <stdint.h>

namespace blob_native
{

    void blob_screen_manager_reset();
    void blob_screen_manager_loop(uint32_t frame_start_us, uint32_t now_ms);

} // namespace blob_native

#endif
