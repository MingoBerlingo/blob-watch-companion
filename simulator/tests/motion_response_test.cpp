#include "apps/blob_native/blob/motion_response.h"
#include "apps/blob_native/blob/soft_body.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace blob_native;
namespace {
void check(bool condition, const char *message) {
    if (!condition) { std::cerr << "FAIL " << message << '\n'; std::exit(1); }
}
}
int main() {
    MotionResponse state{};
    MotionInput sample{0, 0, 1, 0, 0, 0};
    auto result = motion_response_update(state, sample, 20);
    check(result.kick_x == 0 && result.kick_y == 0, "First sample never kicks");
    for (int i=0;i<1000;++i) {
        result = motion_response_update(state, sample, 20);
        check(std::fabs(result.kick_x)+std::fabs(result.kick_y)<0.001f, "Stationary board receives no boost");
    }
    // Roll 30 degrees in 100 ms. Accelerometer direction and gyro agree.
    float tilt_kick=0;
    for (int i=1;i<=5;++i) {
        const float angle=i*0.104719755f;
        sample={0,std::sin(angle),std::cos(angle),300,0,0};
        result=motion_response_update(state,sample,20);
        tilt_kick+=result.kick_x;
    }
    check(tilt_kick>250, "Quick tilt produces an immediate useful velocity change");
    sample.gyro_x_dps=0;
    float held_kick=0;
    for (int i=0;i<100;++i) {
        result=motion_response_update(state,sample,20);
        held_kick+=std::fabs(result.kick_x);
    }
    check(held_kick<10, "Holding the new tilt does not repeatedly boost velocity");
    check(std::fabs(result.tilt_y-0.5f)<0.02f, "Gravity direction follows the tilt without a gravity multiplier");

    state={};sample={0,0,1,0,0,0};motion_response_update(state,sample,20);
    sample.acceleration_y_g=0.8f;
    float shake_kick=0;
    for (int i=0;i<4;++i) shake_kick+=motion_response_update(state,sample,20).kick_x;
    check(shake_kick < -200, "Sideways acceleration creates opposite inertial motion promptly");
    state={};sample={0,0,1,0,0,0};motion_response_update(state,sample,20);
    for (int i=0;i<1000;++i) {
        sample.acceleration_x_g=(i%2 ? 0.01f:-0.01f);
        result=motion_response_update(state,sample,20);
        check(std::fabs(result.kick_y)<0.01f, "Small stationary sensor noise is rejected");
    }
    sample.acceleration_x_g=NAN;
    result=motion_response_update(state,sample,20);
    check(std::isfinite(result.kick_x)&&std::isfinite(result.tilt_x), "Invalid sensor input is ignored");

    MotionResponse one{}, split{};
    sample={0,0,1,0,0,0};motion_response_update(one,sample,20);motion_response_update(split,sample,20);
    sample={0,std::sin(0.06981317f),std::cos(0.06981317f),100,0,0};
    const auto whole=motion_response_update(one,sample,40);
    const auto endpoint=sample;
    sample.acceleration_y_g=std::sin(0.034906585f); sample.acceleration_z_g=std::cos(0.034906585f);
    const auto a=motion_response_update(split,sample,20),b=motion_response_update(split,endpoint,20);
    check(std::fabs(whole.kick_x-a.kick_x-b.kick_x)<0.1f, "Motion sensitivity remains similar across frame rates");

    SoftBody body{};soft_body_reset(body,120,120,50,116);
    soft_body_impulse(body,1000,-1000);
    check(std::hypot(body.velocity_x,body.velocity_y)<=400.01f, "Motion kick caps center speed");
    for (int i=0;i<1000;++i) {
        if (i%20==0) soft_body_impulse(body,(i%40?1:-1)*800,300);
        soft_body_update(body,16,0,650);
        float area=0;
        for (int n=0;n<SOFT_BODY_POINTS;++n) {
            const auto &p=body.points[n], &next=body.points[(n+1)%SOFT_BODY_POINTS];
            check(std::isfinite(p.x)&&std::hypot(p.x-120,p.y-120)<116.01f, "Repeated kicks remain finite and inside bezel");
            area+=p.x*next.y-p.y*next.x;
        }
        check(area>3000, "Repeated motion kicks do not collapse or invert the star");
    }
    std::cout << "Motion response regressions passed\n";
}
