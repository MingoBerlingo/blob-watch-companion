#include "apps/blob_native/blob/physics_settings.h"
#include "platform/waveshare_native_board.h"
#include <stddef.h>
namespace blob_native {
namespace {
struct Record { uint32_t magic,version; PhysicsSettings settings; uint32_t checksum; };
struct LegacyRecord { uint32_t magic,version; float values[8]; uint32_t checksum; };
static_assert(sizeof(Record)==48,"Settings storage format changed: bump version");
static_assert(sizeof(LegacyRecord)==44,"Legacy settings format changed");
template<typename T> uint32_t checksum(const T &record) {
    uint32_t hash=2166136261u;
    const auto *bytes=reinterpret_cast<const uint8_t*>(&record);
    for(size_t i=0;i<offsetof(T,checksum);++i) hash=(hash^bytes[i])*16777619u;
    return hash;
}
}
bool physics_settings_load() {
    PhysicsSettings candidate{};
    Record record{};
    if(waveshare_native_load_settings(&record,sizeof(record)) && record.magic==0x424C4F42 &&
       record.version==2 && record.checksum==checksum(record)) {
        candidate=record.settings;
    } else {
        // Preserve the eight settings saved by firmware without a size slider.
        LegacyRecord legacy{};
        if(!waveshare_native_load_settings(&legacy,sizeof(legacy)) || legacy.magic!=0x424C4F42 ||
           legacy.version!=1 || legacy.checksum!=checksum(legacy)) return false;
        for(int i=0;i<8;++i) candidate.*(physics_settings_control(i).field)=legacy.values[i];
    }
    if(!physics_settings_valid(candidate)) return false;
    for(int i=0;i<PHYSICS_SETTING_COUNT;++i) {
        const auto &c=physics_settings_control(i);
        physics_settings_set(i,static_cast<int>(candidate.*(c.field)*c.scale+0.5f));
    }
    return true;
}
bool physics_settings_save() {
    Record record{0x424C4F42,2,physics_settings_get(),0};
    record.checksum=checksum(record);
    return waveshare_native_save_settings(&record,sizeof(record));
}
}
