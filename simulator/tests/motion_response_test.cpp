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
    MotionInput sample{0,0,1,0,0,0};
    auto result=motion_response_update(state,sample,16);
    for(int i=0;i<500;++i) {
        sample.acceleration_x_g=(i%2 ? 0.03f:-0.03f);
        sample.acceleration_y_g=(i%2 ? -0.03f:0.03f);
        result=motion_response_update(state,sample,16);
        check(std::fabs(result.offset_x)<0.1f && std::fabs(result.offset_y)<0.1f,
              "Small hand tremor does not move the movement target");
    }
    state={};sample={0,0,1,0,0,0};motion_response_update(state,sample,16);
    sample.acceleration_y_g=0.6f;
    result=motion_response_update(state,sample,16);
    check(result.offset_x>0 && result.offset_x<20,"Movement follows the hand with a softened onset");
    for(int i=0;i<5;++i) result=motion_response_update(state,sample,16);
    check(result.offset_x > 20,"Deliberate shakes still produce visible softened motion");
    sample={0,0,1,0,0,0};
    for(int i=0;i<150;++i) result=motion_response_update(state,sample,16);
    check(std::fabs(result.offset_x)<0.1f,"Movement offset settles after movement stops");
    state={};motion_response_update(state,sample,20);
    for(int i=1;i<=5;++i) {
        const float angle=i*0.104719755f;
        sample={0,std::sin(angle),std::cos(angle),300,0,0};
        result=motion_response_update(state,sample,20);
        check(std::fabs(result.offset_x)<0.1f,"Pure gyro-tracked tilt does not masquerade as a shake");
    }
    sample.gyro_x_dps=0;
    for(int i=0;i<100;++i) result=motion_response_update(state,sample,20);
    check(result.tilt_y>0.47f && result.tilt_y<0.51f,"Held tilt retains the working downhill axis mapping");
    sample.acceleration_x_g=NAN;
    result=motion_response_update(state,sample,20);
    check(std::isfinite(result.offset_x)&&std::isfinite(result.tilt_x),"Invalid sensor data is ignored");

    // A small 8 Hz angular tremor has matching acceleration and gyro input.
    state={};sample={0,0,1,0,0,0};motion_response_update(state,sample,8);
    for(int i=1;i<1000;++i) {
        const float time=i*0.008f;
        const float angle=0.01f*std::sin(50.265482f*time);
        const float rate=28.8f*std::cos(50.265482f*(time-0.004f));
        sample={0,std::sin(angle),std::cos(angle),rate,0,0};
        result=motion_response_update(state,sample,8);
        check(std::fabs(result.tilt_y)<0.001f && std::fabs(result.offset_x)<0.1f,
              "Small angular tremor is softened without creating fake shakes");
    }
    state={};sample={0,0,1,0,0,0};motion_response_update(state,sample,20);
    for(int i=1;i<=500;++i) {
        const float angle=i*0.00034906585f;
        sample={0,std::sin(angle),std::cos(angle),1,0,0};
        result=motion_response_update(state,sample,20);
    }
    check(result.tilt_y>0.155f,"Slow deliberate tilts remain responsive despite tremor filtering");
    MotionResponse whole{},split{};sample={0,0,1,0,0,0};
    motion_response_update(whole,sample,20);motion_response_update(split,sample,20);
    sample.acceleration_y_g=0.6f;
    const auto one=motion_response_update(whole,sample,40);
    motion_response_update(split,sample,20);
    const auto two=motion_response_update(split,sample,20);
    check(std::fabs(one.offset_x-two.offset_x)<0.001f,"Smoothing is consistent across sample intervals");

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
