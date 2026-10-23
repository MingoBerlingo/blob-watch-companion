#include "apps/blob_native/steps/pedometer.h"
#include "apps/blob_native/steps/step_counter.h"
#include "platform/waveshare_native_board.h"
#include <stddef.h>
namespace blob_native {
namespace {
StepCounter g_counter;
uint32_t g_last_sample=0,g_saved_total=0,g_last_save=0,g_last_attempt=0;
struct Record {uint32_t magic,version,total,checksum;};
static_assert(sizeof(Record)==16,"Pedometer record format changed");
uint32_t checksum(const Record &record){
 uint32_t hash=2166136261u;const auto *bytes=reinterpret_cast<const uint8_t*>(&record);
 for(size_t i=0;i<offsetof(Record,checksum);++i) hash=(hash^bytes[i])*16777619u;
 return hash;
}
bool save(uint32_t total){Record record{0x53544550,1,total,0};record.checksum=checksum(record);
 return waveshare_native_save_step_data(&record,sizeof(record));}
}
void pedometer_init(uint32_t now){
 g_counter={};Record record{};
 if(waveshare_native_load_step_data(&record,sizeof(record)) && record.magic==0x53544550 &&
    record.version==1 && record.checksum==checksum(record)) g_counter.total=record.total;
 g_saved_total=g_counter.total;g_last_sample=now-20;g_last_save=now;g_last_attempt=now-10000;
}
void pedometer_tick(uint32_t now){
 if(now-g_last_sample<20) return;
 g_last_sample=now;
 float ax,ay,az;
 if(waveshare_native_read_acceleration(&ax,&ay,&az)) step_counter_update(g_counter,now,ax,ay,az);
 else {g_counter.initialized=false;g_counter.streak=0;}
 if(g_counter.total!=g_saved_total && (g_counter.total-g_saved_total>=100 || now-g_last_save>=300000) &&
    now-g_last_attempt>=10000){
  g_last_attempt=now;
  if(save(g_counter.total)){g_saved_total=g_counter.total;g_last_save=now;}
 }
}
uint32_t pedometer_total(){return g_counter.total;}
bool pedometer_reset(uint32_t now){
 if(!save(0)) return false;
 g_counter={};g_saved_total=0;g_last_sample=now-20;g_last_save=g_last_attempt=now;
 return true;
}
}
