#ifndef BLOB_NATIVE_STEPS_PAGE_H
#define BLOB_NATIVE_STEPS_PAGE_H
#include <stdint.h>
namespace blob_native {
void steps_page_init();
void steps_page_show();
void steps_page_tick();
void steps_page_request_reset();
bool steps_page_reset_open();
}
#endif
