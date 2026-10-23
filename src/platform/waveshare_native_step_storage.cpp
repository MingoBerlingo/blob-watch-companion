#include "platform/waveshare_native_board.h"
#include <LittleFS.h>
#include <string.h>
extern uint8_t _FS_start;
extern uint8_t _FS_end;
namespace {
bool g_mounted=false;
bool mount(bool initialize){
 if(g_mounted) return true;
 LittleFS.setConfig(LittleFSConfig(false));
 if(LittleFS.begin()){g_mounted=true;return true;}
 if(!initialize) return false;
 // Only initialize a blank partition. Mount errors must never erase a total.
 if(&_FS_end-&_FS_start<8192) return false;
 for(const uint8_t *p=&_FS_start;p<&_FS_end;++p) if(*p!=0xff) return false;
 if(!LittleFS.format() || !LittleFS.begin()) return false;
 g_mounted=true;return true;
}
}
bool waveshare_native_load_step_data(void *data,size_t size){
 if(!data || !size || size>128 || !mount(false)) return false;
 auto file=LittleFS.open("/steps.bin","r");
 if(!file || file.size()!=size) return false;
 const bool ok=file.read(static_cast<uint8_t*>(data),size)==static_cast<int>(size);
 file.close();return ok;
}
bool waveshare_native_save_step_data(const void *data,size_t size){
 if(!data || !size || size>128 || !mount(true)) return false;
 // Stage and verify a replacement. LittleFS rename commits atomically; a
 // checkpoint never erases the separate physics-settings EEPROM sector.
 auto file=LittleFS.open("/steps.tmp","w");
 if(!file) return false;
 const bool written=file.write(static_cast<const uint8_t*>(data),size)==size;
 file.flush();file.close();
 if(!written) return false;
 auto verify=LittleFS.open("/steps.tmp","r");
 uint8_t bytes[128];
 if(!verify || verify.size()!=size || verify.read(bytes,size)!=static_cast<int>(size) ||
    memcmp(bytes,data,size)!=0) return false;
 verify.close();
 return LittleFS.rename("/steps.tmp","/steps.bin");
}
