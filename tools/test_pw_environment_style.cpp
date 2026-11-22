#include "PW-EnvironmentStyleSettings.h"
#include <cassert>
#include <cstdio>
#include <fstream>
int main() {
    const char* folder = ".";
    const char* file = ".\\pw_style_settings.txt";
    {
        std::ofstream out(file);
        out << "scale=125\nrotation=-35\n";
    }
    const auto config = PWEnvironmentStyle::Read(folder);
    assert(config.percent == 125.f);
    assert(config.yaw == -35.f);
    std::remove(file);
    const auto fallback = PWEnvironmentStyle::Read(folder);
    assert(fallback.percent == 100.f);
    assert(fallback.yaw == 0.f);
    puts("PW environment scale/yaw metadata checks passed");
    return 0;
}
