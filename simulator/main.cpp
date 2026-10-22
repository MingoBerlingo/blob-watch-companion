#include "runtime.h"
#include <SDL.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include "apps/blob_native/lv_port/lv_port.h"
#include "apps/blob_native/pages/page_manager.h"

int main(int argc, char **argv)
{
    bool test = false;
    bool benchmark = false;
    bool demo = false;
    std::filesystem::path output = "screenshots";
    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--test") test = true;
        else if (argument == "--benchmark") benchmark = true;
        else if (argument == "--softbody-demo") demo = true;
        else if (argument == "--output" && i + 1 < argc) output = argv[++i];
        else if (argument == "--help")
        {
            std::cout << "blob_simulator [--test --output DIRECTORY] [--benchmark] [--softbody-demo --output DIRECTORY]\n"
                      << "Mouse: touch/drag/swipe. Left/Right: pages. W/X/A/D: tilt; R: reset.\n"
                      << "Space: toggle 10x clock. S: screenshot.bmp. Escape: quit.\n";
            return 0;
        }
        else { std::cerr << "Unknown or incomplete argument: " << argument << '\n'; return 2; }
    }
    try
    {
        simulator::init(test || benchmark || demo);
        blob_native::lv_port_init();
        blob_native::page_manager_init();
        simulator::step(50);
        if (demo)
        {
            simulator::soft_body_demo(output);
            simulator::shutdown();
            return 0;
        }
        if (benchmark)
        {
            simulator::benchmark_blob();
            simulator::shutdown();
            return 0;
        }
        if (test)
        {
            std::filesystem::create_directories(output);
            const bool passed = simulator::run_scenarios(output);
            simulator::shutdown();
            return passed ? 0 : 1;
        }
        std::cout << "Use --help for controls. The window shows the real firmware UI.\n";
        uint32_t last = SDL_GetTicks();
        while (simulator::handle_events())
        {
            const uint32_t now = SDL_GetTicks();
            const uint32_t elapsed = now - last;
            // Process every input sample before the next event poll clears its gesture.
            simulator::step(std::clamp<uint32_t>(elapsed, 5, 250) * (simulator::fast_clock() ? 10 : 1));
            last = now;
            simulator::present();
            SDL_Delay(5);
        }
        simulator::shutdown();
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        simulator::shutdown();
        return 1;
    }
}
