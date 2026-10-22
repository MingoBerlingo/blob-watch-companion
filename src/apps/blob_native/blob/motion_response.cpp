#include "apps/blob_native/blob/motion_response.h"
#include <math.h>
namespace blob_native
{
    namespace
    {
        constexpr float kMovementOffsetGain = 100;
        constexpr float kLinearDeadband = 0.06f;
        constexpr float kRadians = 0.01745329252f;
        float deadzone(float value,float threshold) {
            return value>threshold ? value-threshold:(value<-threshold ? value+threshold:0);
        }
        MotionOutput output(const MotionResponse &state) {
            return {deadzone(state.smooth_tilt_x,0.012f),deadzone(state.smooth_tilt_y,0.012f),
                    kMovementOffsetGain*state.linear_y,-kMovementOffsetGain*state.linear_x};
        }
    }
    MotionOutput motion_response_update(MotionResponse &state, const MotionInput &in, uint32_t elapsed_ms)
    {
        MotionOutput out=output(state);
        if (!isfinite(in.acceleration_x_g) || !isfinite(in.acceleration_y_g) || !isfinite(in.acceleration_z_g) ||
            !isfinite(in.gyro_x_dps) || !isfinite(in.gyro_y_dps) || !isfinite(in.gyro_z_dps)) return out;
        const float ax=in.acceleration_x_g, ay=in.acceleration_y_g, az=in.acceleration_z_g;
        const float norm=sqrtf(ax*ax+ay*ay+az*az);
        if (!state.ready)
        {
            if (norm < 0.5f || norm > 1.5f) return out;
            state.gravity_x=ax/norm; state.gravity_y=ay/norm; state.gravity_z=az/norm;
            state.ready=true;
            state.smooth_tilt_x=state.gravity_x;state.smooth_tilt_y=state.gravity_y;
            return output(state);
        }
        const float dt=(elapsed_ms>64 ? 64:elapsed_ms)/1000.0f;
        if (dt<=0) return out;
        const float wx=in.gyro_x_dps*kRadians, wy=in.gyro_y_dps*kRadians, wz=in.gyro_z_dps*kRadians;
        const float rate=sqrtf(wx*wx+wy*wy+wz*wz);
        const float old_x=state.gravity_x, old_y=state.gravity_y, old_z=state.gravity_z;
        float gx=old_x, gy=old_y, gz=old_z;
        if (rate > 0.000001f)
        {
            const float nx=wx/rate, ny=wy/rate, nz=wz/rate;
            const float angle=rate*dt, c=cosf(angle), s=sinf(angle);
            const float dot=old_x*nx+old_y*ny+old_z*nz;
            // Gravity expressed in device coordinates rotates opposite the board.
            gx=old_x*c+(old_y*nz-old_z*ny)*s+nx*dot*(1-c);
            gy=old_y*c+(old_z*nx-old_x*nz)*s+ny*dot*(1-c);
            gz=old_z*c+(old_x*ny-old_y*nx)*s+nz*dot*(1-c);
        }
        // Gyro drives gravity direction only. Correct drift slowly while rotation
        // is quiet and acceleration magnitude is close to 1g; never kick from gyro.
        if (norm>0.92f && norm<1.08f && rate<0.2f)
        {
            const float alpha=dt/(2.5f+dt);
            gx+=(ax/norm-gx)*alpha;
            gy+=(ay/norm-gy)*alpha;
            gz+=(az/norm-gz)*alpha;
        }
        const float length=sqrtf(gx*gx+gy*gy+gz*gz);
        if (length<0.0001f) return out;
        state.gravity_x=gx/length; state.gravity_y=gy/length; state.gravity_z=gz/length;
        // Subtract gravity before mapping acceleration to a softened movement offset.
        // Deadzone rejects hand/sensor tremor; low-pass filters soften onset and
        // release without adding an unbounded integrated position drift.
        const float linear_alpha=1-expf(-dt/0.070f);
        const float tilt_alpha=1-expf(-dt/0.080f);
        const float linear_x=deadzone(fmaxf(-1,fminf(1,ax-state.gravity_x)),kLinearDeadband);
        const float linear_y=deadzone(fmaxf(-1,fminf(1,ay-state.gravity_y)),kLinearDeadband);
        state.linear_x+=(linear_x-state.linear_x)*linear_alpha;
        state.linear_y+=(linear_y-state.linear_y)*linear_alpha;
        state.smooth_tilt_x+=(state.gravity_x-state.smooth_tilt_x)*tilt_alpha;
        state.smooth_tilt_y+=(state.gravity_y-state.smooth_tilt_y)*tilt_alpha;
        return output(state);
    }
}
