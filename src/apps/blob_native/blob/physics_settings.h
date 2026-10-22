#ifndef BLOB_NATIVE_PHYSICS_SETTINGS_H
#define BLOB_NATIVE_PHYSICS_SETTINGS_H
#include <stdint.h>
namespace blob_native {
struct PhysicsSettings {
    float follow_stiffness=2500, follow_damping=85;
    float shape_stiffness=320, shape_damping=8;
    float tilt_sensitivity=1.5f, bounce=0.3f;
    float friction=0.04f, max_speed=500;
    float blob_radius=50;
};
constexpr int PHYSICS_SETTING_COUNT=9;
struct PhysicsControl {
    const char *label;
    int minimum, maximum, step;
    float scale;
    float PhysicsSettings::*field;
};
const PhysicsControl &physics_settings_control(int index);
const PhysicsSettings &physics_settings_get();
int physics_settings_value(int index);
void physics_settings_set(int index, int value);
void physics_settings_reset();
bool physics_settings_valid(const PhysicsSettings &settings);
bool physics_settings_load();
bool physics_settings_save();
}
#endif
