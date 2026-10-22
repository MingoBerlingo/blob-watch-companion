#include "apps/blob_native/blob/physics_settings.h"
#include <math.h>
namespace blob_native {
namespace {
PhysicsSettings g_settings;
const PhysicsControl kControls[PHYSICS_SETTING_COUNT]={
    {"Follow speed",1500,3500,100,1,&PhysicsSettings::follow_stiffness},
    {"Follow damping",60,110,5,1,&PhysicsSettings::follow_damping},
    {"Stiffness",240,480,20,1,&PhysicsSettings::shape_stiffness},
    {"Shape damping",4,20,1,1,&PhysicsSettings::shape_damping},
    {"Tilt sensitivity",5,50,1,10,&PhysicsSettings::tilt_sensitivity},
    {"Bounce",10,40,1,100,&PhysicsSettings::bounce},
    {"Wall friction",0,8,1,100,&PhysicsSettings::friction},
    {"Max speed",350,500,10,1,&PhysicsSettings::max_speed},
    {"Blob size",60,160,5,2,&PhysicsSettings::blob_radius},
};
}
const PhysicsControl &physics_settings_control(int index) { return kControls[index>=0 && index<PHYSICS_SETTING_COUNT ? index:0]; }
const PhysicsSettings &physics_settings_get() { return g_settings; }
int physics_settings_value(int index) {
    const auto &c=physics_settings_control(index);
    return static_cast<int>(lroundf(g_settings.*(c.field)*c.scale));
}
void physics_settings_set(int index,int value) {
    if(index<0 || index>=PHYSICS_SETTING_COUNT) return;
    const auto &c=kControls[index];
    value=value<c.minimum ? c.minimum:(value>c.maximum ? c.maximum:value);
    value=c.minimum+((value-c.minimum+c.step/2)/c.step)*c.step;
    g_settings.*(c.field)=value/c.scale;
}
void physics_settings_reset() { g_settings=PhysicsSettings{}; }
bool physics_settings_valid(const PhysicsSettings &settings) {
    for(const auto &c:kControls) {
        const float value=settings.*(c.field)*c.scale;
        if(!isfinite(value) || value<c.minimum-0.001f || value>c.maximum+0.001f) return false;
    }
    return true;
}
}
