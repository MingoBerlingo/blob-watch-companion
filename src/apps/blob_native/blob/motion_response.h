#ifndef BLOB_NATIVE_MOTION_RESPONSE_H
#define BLOB_NATIVE_MOTION_RESPONSE_H
#include <stdint.h>
namespace blob_native
{
    struct MotionInput
    {
        float acceleration_x_g, acceleration_y_g, acceleration_z_g;
        float gyro_x_dps, gyro_y_dps, gyro_z_dps;
    };
    struct MotionResponse
    {
        float gravity_x = 0, gravity_y = 0, gravity_z = 1;
        float smooth_tilt_x=0, smooth_tilt_y=0;
        float linear_x=0, linear_y=0;
        bool ready = false;
    };
    struct MotionOutput
    {
        float tilt_x, tilt_y; // filtered board tilt; screen mapping is (+Y, -X)
        float offset_x, offset_y; // filtered same-direction target offset in screen pixels
    };
    MotionOutput motion_response_update(MotionResponse &state, const MotionInput &input, uint32_t elapsed_ms);
}
#endif
