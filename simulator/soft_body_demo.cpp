#include "runtime.h"
#include <iomanip>
#include <iostream>
#include <sstream>

namespace simulator {
void soft_body_demo(const std::filesystem::path &output) {
    std::filesystem::create_directories(output);
    for (int frame = 0; frame < 150; ++frame) {
        if (frame < 25) set_tilt(0, 0);
        else if (frame < 65) set_tilt(-0.6f, 0.8f);
        else if (frame < 105) set_tilt(0.7f, -0.7f);
        else set_tilt(0, 0);
        step(40);
        std::ostringstream name;
        name << "frame-" << std::setfill('0') << std::setw(3) << frame << ".bmp";
        screenshot(output / name.str());
        if (frame == 24) screenshot(output / "softbody-rest.bmp");
        if (frame == 64) screenshot(output / "softbody-rim.bmp");
        if (frame == 74) screenshot(output / "softbody-rebound.bmp");
        if (frame == 149) screenshot(output / "softbody-settled.bmp");
    }
    std::cout << "Saved 150 deterministic frames (6 simulated seconds) to " << output << '\n';
}
}
