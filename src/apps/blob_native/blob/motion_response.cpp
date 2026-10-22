#include "apps/blob_native/blob/motion_response.h"
#include <math.h>
namespace blob_native
{
    namespace
    {
        constexpr float kTiltVelocityGain = 650;
        constexpr float kShakeAccelerationGain = 6500;
        constexpr float kLinearDeadband = 0.04f;
        constexpr float kRadians = 0.01745329252f;
    }
    MotionOutput motion_response_update(MotionResponse &state, const MotionInput &in, uint32_t elapsed_ms)
    {
        MotionOutput out{state.gravity_x, state.gravity_y, 0, 0};
        if (!isfinite(in.acceleration_x_g) || !isfinite(in.acceleration_y_g) || !isfinite(in.acceleration_z_g) ||
            !isfinite(in.gyro_x_dps) || !isfinite(in.gyro_y_dps) || !isfinite(in.gyro_z_dps)) return out;
        const float ax=in.acceleration_x_g, ay=in.acceleration_y_g, az=in.acceleration_z_g;
        const float norm=sqrtf(ax*ax+ay*ay+az*az);
        if (!state.ready)
        {
            if (norm < 0.5f || norm > 1.5f) return out;
            state.gravity_x=ax/norm; state.gravity_y=ay/norm; state.gravity_z=az/norm;
            state.ready=true;
            return {state.gravity_x,state.gravity_y,0,0};
        }
        const float dt=(elapsed_ms>64 ? 64:elapsed_ms)/1000.0f;
        if (dt<=0) return out;
        const float wx=in.gyro_x_dps*kRadians, wy=in.gyro_y_dps*kRadians, wz=in.gyro_z_dps*kRadians;
        const float rate=sqrtf(wx*wx+wy*wy+wz*wz);
        const float old_x=state.gravity_x, old_y=state.gravity_y, old_z=state.gravity_z;
        float gx=old_x, gy=old_y, gz=old_z;
        if (rate > 0.02f)
        {
            const float nx=wx/rate, ny=wy/rate, nz=wz/rate;
            const float angle=rate*dt, c=cosf(angle), s=sinf(angle);
            const float dot=old_x*nx+old_y*ny+old_z*nz;
            // Gravity expressed in device coordinates rotates opposite the board.
            gx=old_x*c+(old_y*nz-old_z*ny)*s+nx*dot*(1-c);
            gy=old_y*c+(old_z*nx-old_x*nz)*s+ny*dot*(1-c);
            gz=old_z*c+(old_x*ny-old_y*nx)*s+nz*dot*(1-c);
        }
        const float tilt_delta_x=gx-old_x, tilt_delta_y=gy-old_y;
        // Slow correction limits gyro drift. Reject acceleration magnitudes that
        // clearly include a shake instead of interpreting them as a new tilt.
        if (norm>0.85f && norm<1.15f)
        {
            const float alpha=dt/(0.7f+dt);
            gx+=(ax/norm-gx)*alpha;
            gy+=(ay/norm-gy)*alpha;
            gz+=(az/norm-gz)*alpha;
        }
        const float length=sqrtf(gx*gx+gy*gy+gz*gz);
        if (length<0.0001f) return out;
        state.gravity_x=gx/length; state.gravity_y=gy/length; state.gravity_z=gz/length;
        float linear_x=ax-state.gravity_x, linear_y=ay-state.gravity_y;
        if (fabsf(linear_x)<kLinearDeadband) linear_x=0;
        if (fabsf(linear_y)<kLinearDeadband) linear_y=0;
        out.tilt_x=state.gravity_x; out.tilt_y=state.gravity_y;
        out.kick_x=kTiltVelocityGain*tilt_delta_y-kShakeAccelerationGain*linear_y*dt;
        out.kick_y=-kTiltVelocityGain*tilt_delta_x+kShakeAccelerationGain*linear_x*dt;
        return out;
    }
}
