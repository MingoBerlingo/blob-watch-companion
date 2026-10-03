#ifndef BLOB_NATIVE_TIMER_PAGE_H
#define BLOB_NATIVE_TIMER_PAGE_H

#include <stdint.h>

namespace blob_native
{

    void timer_page_init();
    void timer_page_show();
    void timer_page_hide();
    void timer_page_tick(uint32_t now_ms);

} // namespace blob_native

#endif
