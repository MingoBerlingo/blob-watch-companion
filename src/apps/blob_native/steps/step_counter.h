#ifndef BLOB_NATIVE_STEP_COUNTER_H
#define BLOB_NATIVE_STEP_COUNTER_H
#include <stdint.h>
namespace blob_native {
constexpr uint32_t STEP_GOAL=10000;
struct StepCounter {
 uint32_t total=0,last_sample_ms=0,peak_start_ms=0,last_peak_ms=0,last_interval_ms=0;
 float baseline=1,filtered=0;
 uint8_t streak=0;
 bool initialized=false,peak_active=false,armed=true;
};
void step_counter_update(StepCounter &state,uint32_t now_ms,float ax_g,float ay_g,float az_g);
}
#endif
