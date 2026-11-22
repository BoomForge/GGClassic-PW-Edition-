#pragma once
// Load scale and yaw from a PW Edition skybank/vegbank style, preserving
// original Classic behaviour when no metadata is present.
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

namespace PWEnvironmentStyle {
struct Settings {
    float percent = 100.0f;
    float yaw = 0.0f;
};
inline Settings Read(const std::string& styleFolder) {
    Settings result;
    std::ifstream in(styleFolder + "\\pw_style_settings.txt");
    std::string line;
    while (in && std::getline(in, line)) {
        float number = 0.f;
        if (::sscanf_s(line.c_str(), "scale=%f", &number) == 1 &&
            number >= 1.f && number <= 400.f)
            result.percent = number;
        else if (::sscanf_s(line.c_str(), "rotation=%f", &number) == 1 &&
                 number >= -180.f && number <= 180.f)
            result.yaw = number;
    }
    return result;
}
} // namespace PWEnvironmentStyle
