#ifndef BLOB_NATIVE_BLOB_PAGE_H
#define BLOB_NATIVE_BLOB_PAGE_H

#include <stdint.h>

namespace blob_native
{

    void blob_page_reset();
    void blob_page_loop(uint32_t frame_start_us);

} // namespace blob_native

#endif
