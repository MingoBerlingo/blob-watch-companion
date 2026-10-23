#include "apps/blob_native/steps/pedometer.h"
#include "apps/blob_native/blob/physics_settings.h"
#include "platform/waveshare_native_board.h"
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace blob_native;
namespace {
uint8_t storage[256];uint32_t now=0;bool walking=false,writable=true;int writes=0;
void check(bool ok,const char *message){if(!ok){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
}
bool waveshare_native_read_acceleration(float *x,float *y,float *z){*x=*y=0;*z=walking ? 1+0.25f*std::sin(now*0.001f*12.5663706f):1;return true;}
bool waveshare_native_load_step_data(void *data,size_t size){memcpy(data,storage+128,size);return true;}
bool waveshare_native_save_step_data(const void *data,size_t size){if(!writable)return false;memcpy(storage+128,data,size);++writes;return true;}
bool waveshare_native_load_settings(void *data,size_t size){memcpy(data,storage,size);return true;}
bool waveshare_native_save_settings(const void *data,size_t size){memcpy(storage,data,size);return true;}
int main(){
 memset(storage,0xff,sizeof(storage));pedometer_init(now);check(pedometer_total()==0,"Empty flash starts at zero");
 physics_settings_reset();physics_settings_set(4,20);check(physics_settings_save(),"Physics settings fixture saves");
 for(int i=0;i<100;++i){now+=20;pedometer_tick(now);}
 walking=true;
 for(int i=0;i<1500;++i){now+=20;pedometer_tick(now);}
 walking=false;for(int i=0;i<100;++i){now+=20;pedometer_tick(now);}
 const auto counted=pedometer_total();check(counted>=58 && counted<=61,"Background runtime counts walking samples");
 check(writes==0,"Flash is not written for every step");
 now+=300001;pedometer_tick(now);check(writes==1,"Dirty total checkpoints after five minutes");
 pedometer_init(now);check(pedometer_total()==counted,"A checkpoint survives restart");
 physics_settings_reset();check(physics_settings_load() && physics_settings_value(4)==20,"Step checkpoints preserve physics settings");
 writable=false;check(!pedometer_reset(now) && pedometer_total()==counted,"Failed reset preserves the total and reports failure");
 writable=true;check(pedometer_reset(now) && pedometer_total()==0,"Confirmed reset clears the total");
 pedometer_init(now);check(pedometer_total()==0,"Reset is persisted immediately");
 walking=true;for(int i=0;i<2800;++i){now+=20;pedometer_tick(now);}walking=false;
 check(writes>=3,"Hundred-step checkpoint is saved without waiting five minutes");
 storage[136]^=1;pedometer_init(now);check(pedometer_total()==0,"Corrupt checkpoint is rejected");
 std::puts("Pedometer runtime and storage regressions passed");
}
