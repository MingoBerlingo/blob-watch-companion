#include "apps/blob_native/blob/soft_body.h"
#include "apps/blob_native/shared_state.h"
#include "apps/blob_native/blob/face.h"
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <iostream>

using namespace blob_native;
namespace {
void check(bool passed, const char *message) {
    if (!passed) { std::cerr << "FAIL " << message << '\n'; std::exit(1); }
}
float area(const SoftBody &body) {
    float sum = 0;
    for (int i = 0; i < SOFT_BODY_POINTS; ++i) {
        const auto &a = body.points[i], &b = body.points[(i + 1) % SOFT_BODY_POINTS];
        sum += a.x * b.y - b.x * a.y;
    }
    return sum * 0.5f;
}
float deformation(const SoftBody &body) {
    float sum = 0;
    for (int i = 0; i < SOFT_BODY_POINTS; ++i) {
        const float dx = body.points[i].x - body.center_x - body.rest_x[i];
        const float dy = body.points[i].y - body.center_y - body.rest_y[i];
        sum += dx * dx + dy * dy;
    }
    return sum;
}
float turn(const SoftBodyPoint &a, const SoftBodyPoint &b, const SoftBodyPoint &c) {
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}
void bounded(const SoftBody &body) {
    for (const auto &p : body.points) {
        check(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.vx) && std::isfinite(p.vy), "Finite particle state");
        check(std::hypot(p.x - body.arena_x, p.y - body.arena_y) <= body.arena_radius + 0.01f, "Particles stay inside the round screen");
    }
    for (int i=0; i<SOFT_BODY_POINTS; ++i) {
        const int ni=(i+1)%SOFT_BODY_POINTS;
        for (int j=i+1; j<SOFT_BODY_POINTS; ++j) {
            const int nj=(j+1)%SOFT_BODY_POINTS;
            if (j==ni || nj==i) continue;
            const auto &a=body.points[i], &b=body.points[ni], &c=body.points[j], &d=body.points[nj];
            check(!(turn(a,b,c)*turn(a,b,d)<-0.001f && turn(c,d,a)*turn(c,d,b)<-0.001f), "Contour edges do not cross under stress");
        }
    }
}
}
int main() {
    SoftBody body{};
    soft_body_reset(body, 120, 120, 50, 116);
    check(SOFT_BODY_POINTS < 24, "Physics still uses fewer points than the first prototype");
    int tips=0;
    for (int i=0;i<SOFT_BODY_POINTS;++i) {
        const int before=(i+SOFT_BODY_POINTS-1)%SOFT_BODY_POINTS, after=(i+1)%SOFT_BODY_POINTS;
        const float r=std::hypot(body.rest_x[i],body.rest_y[i]);
        if (r>std::hypot(body.rest_x[before],body.rest_y[before])+1 &&
            r>std::hypot(body.rest_x[after],body.rest_y[after])+1) ++tips;
    }
    check(tips==8, "The rest star has eight distinct spikes");
    float shortest = 1000, longest = 0;
    for (int i=0; i<SOFT_BODY_POINTS; ++i) {
        const float r=std::hypot(body.rest_x[i], body.rest_y[i]);
        shortest=std::fmin(shortest,r); longest=std::fmax(longest,r);
    }
    check(shortest > longest*0.75f && shortest < longest*0.85f, "Star has a larger inner radius while retaining visible valleys");
    auto response = body;
    soft_body_follow(response,16,152,120);
    check(response.center_x>120.5f, "Position controller responds immediately toward accelerometer target");
    for (int i=0;i<200;++i) soft_body_follow(response,8,152,120);
    check(std::fabs(response.center_x-152)<0.5f, "Held tilt settles at a position instead of falling indefinitely");
    for (int i=0;i<200;++i) soft_body_follow(response,8,120,120);
    check(std::fabs(response.center_x-120)<0.5f, "Returning flat recenters the soft body");
    auto grouped=body, subdivided=body;
    for (int i=0;i<100;++i) {
        soft_body_follow(grouped,32,184,120);
        for (int j=0;j<4;++j) soft_body_follow(subdivided,8,184,120);
    }
    for (int i=0;i<SOFT_BODY_POINTS;++i)
        check(grouped.points[i].x==subdivided.points[i].x && grouped.points[i].y==subdivided.points[i].y,
              "Position control is independent of render frame partition");
    const float rest_area = area(body);
    auto initial = body;
    for (int i = 0; i < 2000; ++i) soft_body_update(body, 8, 0, 0);
    check(std::fabs(area(body) / rest_area - 1) < 0.001f, "Rest state preserves area");
    check(deformation(body) < 0.01f, "Rest shape stays at equilibrium");

    // Shape matching must not pull a translated or rotated body to its old pose.
    body = initial;
    for (auto &p : body.points) { p.x += 12; p.y -= 9; }
    for (int i = 0; i < 200; ++i) soft_body_update(body, 8, 0, 0);
    check(std::fabs(body.center_x - 132) < 0.01f && std::fabs(body.center_y - 111) < 0.01f, "Translation invariance");
    body = initial;
    for (auto &p : body.points) {
        const float dx = p.x - 120, dy = p.y - 120;
        p.x = 120 - dy; p.y = 120 + dx;
    }
    const auto rotated = body;
    float rotation_cos, rotation_sin;
    soft_body_rotation(body, &rotation_cos, &rotation_sin);
    check(std::fabs(rotation_cos)<0.001f && std::fabs(rotation_sin-1)<0.001f, "Expose the body's actual quarter-turn rotation");
    int16_t lx,ly,rx,ry;
    compute_eye_positions(120,120,rotation_sin,-rotation_cos,0,0,&lx,&ly,&rx,&ry);
    check(std::abs(lx-rx)<=1 && std::abs(ly-ry)>8 && lx>120, "Eyes turn with the body instead of staying upright");
    int16_t mx0,my0,mxm,mym,mx1,my1;
    compute_mouth_points(120,120,rotation_sin,-rotation_cos,0,0,&mx0,&my0,&mxm,&mym,&mx1,&my1);
    check(std::abs(mx0-mx1)<=1 && std::abs(my0-my1)>10 && mxm<120, "Mouth turns with the body");

    for (int i = 0; i < 100; ++i) soft_body_update(body, 8, 0, 0);
    float rotation_error = 0;
    for (int i = 0; i < SOFT_BODY_POINTS; ++i)
        rotation_error += std::hypot(body.points[i].x - rotated.points[i].x, body.points[i].y - rotated.points[i].y);
    check(rotation_error < 0.1f, "Rotation invariance");

    body = initial;
    body.points[0].x = 120 + (body.points[0].x-120)*0.6f;
    body.points[0].y = 120 + (body.points[0].y-120)*0.6f;
    const float displaced = deformation(body);
    for (int i = 0; i < 1000; ++i) soft_body_update(body, 8, 0, 0);
    check(deformation(body) < displaced * 0.02f, "Local dent relaxes instead of oscillating forever");
    check(area(body) > rest_area * 0.98f, "Dent recovery preserves the body volume");

    body = initial;
    auto partitioned = initial;
    for (int i = 0; i < 200; ++i) {
        soft_body_update(body, 32, 500, 250);
        for (int j = 0; j < 4; ++j) soft_body_update(partitioned, 8, 500, 250);
    }
    for (int i = 0; i < SOFT_BODY_POINTS; ++i)
        check(body.points[i].x == partitioned.points[i].x && body.points[i].y == partitioned.points[i].y, "Frame partition does not change physics");
    check(body.center_x > 150 && body.center_y > 135, "Tilt acceleration moves the body to the rim");
    bounded(body);
    check(area(body) > rest_area * 0.65f, "Rim compression does not collapse the body");
    check(deformation(body) > 5, "Rim contact produces real deformation");

    body = initial;
    partitioned = initial;
    soft_body_update(body, 0xffffffffu, 500, 0);
    soft_body_update(partitioned, 64, 500, 0);
    for (int i = 0; i < SOFT_BODY_POINTS; ++i)
        check(body.points[i].x == partitioned.points[i].x, "Long stalls have bounded catch-up work");

    body = initial;
    for (int i = 0; i < 30000; ++i) {
        const float phase = i * 0.008f;
        soft_body_update(body, 8, std::sin(phase * 1.3f) * 900, std::cos(phase * 0.7f) * 900);
        if (i % 20 == 0) {
            bounded(body);
            if (area(body) <= rest_area * 0.45f) std::cerr << "Stress step " << i << " area ratio " << area(body)/rest_area << " center " << body.center_x << "," << body.center_y << "\n";
            check(area(body) > rest_area * 0.45f, "Long stress run avoids inversion or collapse");
        }
    }
    int16_t px[48], py[48];
    soft_body_outline(body, px, py, 48);
    for (int i = 0; i < 48; ++i)
        check(std::hypot(px[i] - 120, py[i] - 120) < 118, "Smoothed outline remains within the visible bezel");
    soft_body_update(body, 16, NAN, INFINITY);
    bounded(body);

    body = initial;
    for (int i=0;i<20000;++i) {
        const float phase=i*0.008f;
        soft_body_follow(body,8,120+80*std::sin(phase*1.3f),120+80*std::cos(phase*0.7f));
        if (i%20==0) {
            bounded(body);
            check(area(body)>rest_area*0.45f, "Position following preserves the soft body under rim impacts");
        }
    }
    body = initial;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 100000; ++i) soft_body_update(body, 8, 400, 200);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << "Soft-body regressions passed; storage " << sizeof(SoftBody)
              << " bytes; 100000 fixed steps " << ms << " desktop ms (not board FPS).\n";
}
