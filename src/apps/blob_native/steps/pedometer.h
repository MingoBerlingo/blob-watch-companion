#ifndef BLOB_NATIVE_PEDOMETER_H
#define BLOB_NATIVE_PEDOMETER_H
#include <stdint.h>
namespace blob_native {
void pedometer_init(uint32_t now_ms);
void pedometer_tick(uint32_t now_ms);
uint32_t pedometer_total();
bool pedometer_reset(uint32_t now_ms);
}
#endif
