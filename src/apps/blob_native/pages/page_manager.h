#ifndef BLOB_NATIVE_PAGE_MANAGER_H
#define BLOB_NATIVE_PAGE_MANAGER_H

#include <stdint.h>

namespace blob_native
{

    void page_manager_init();
    void page_manager_loop();
    void page_manager_show_blob();

} // namespace blob_native

#endif
