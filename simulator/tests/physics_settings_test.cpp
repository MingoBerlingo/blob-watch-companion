#include "apps/blob_native/blob/physics_settings.h"
#include "platform/waveshare_native_board.h"
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace blob_native;
namespace {
uint8_t storage[256]{};size_t stored=0;bool writable=true;
void check(bool ok,const char *message) { if(!ok) {std::cerr<<"FAIL "<<message<<'\n';std::exit(1);} }
}
bool waveshare_native_load_settings(void *data,size_t size) {
    if(size!=stored) return false;memcpy(data,storage,size);return true;
}
bool waveshare_native_save_settings(const void *data,size_t size) {
    if(!writable || size>sizeof(storage)) return false;memcpy(storage,data,size);stored=size;return true;
}
int main() {
    physics_settings_reset();
    check(!physics_settings_load(),"Missing storage leaves defaults");
    check(physics_settings_get().follow_stiffness==2500 && physics_settings_get().shape_damping==8,
          "Defaults preserve the working cursor response");
    check(physics_settings_value(4)==15,"Default tilt influence is gentle at 1.5x");
    physics_settings_set(0,99999);physics_settings_set(4,-100);
    check(physics_settings_value(0)==3500 && physics_settings_value(4)==5,"Controls clamp out-of-range input");
    physics_settings_set(0,2670);check(physics_settings_value(0)==2700,"Controls use their advertised steps");
    physics_settings_set(8,160);
    check(physics_settings_save(),"Explicit Save persists settings");
    physics_settings_reset();
    check(physics_settings_load() && physics_settings_value(0)==2700 && physics_settings_value(4)==5 && physics_settings_value(8)==160,
          "A restart restores saved settings");
    storage[10]^=1;physics_settings_reset();
    check(!physics_settings_load() && physics_settings_value(0)==2500,"Corrupt storage cannot alter defaults");
    const uint8_t legacy[]={66,79,76,66,1,0,0,0,0,192,40,69,0,0,170,66,0,0,160,67,0,0,0,65,0,0,0,64,154,153,153,62,10,215,35,61,0,0,250,67,2,118,218,248};
    memcpy(storage,legacy,sizeof(legacy));stored=sizeof(legacy);
    check(physics_settings_load() && physics_settings_value(0)==2700 && physics_settings_value(4)==20 &&
          physics_settings_value(8)==100,"Legacy saved settings migrate with default Blob size");
    writable=false;check(!physics_settings_save(),"Flash failures propagate to the UI");
    auto invalid=physics_settings_get();invalid.bounce=NAN;
    check(!physics_settings_valid(invalid),"Non-finite persisted physics values are rejected");
    invalid=physics_settings_get();invalid.max_speed=10000;
    check(!physics_settings_valid(invalid),"Unsafe persisted physics values are rejected");
    std::cout<<"Physics settings regressions passed\n";
}
