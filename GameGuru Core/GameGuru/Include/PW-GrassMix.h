#pragma once
// PW Edition: deterministic, weighted 2-4 style grass distribution.
// Pure configuration and style-choice code; rendering is in M-Grass.cpp.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
namespace PWGrassMix {
constexpr int MaxStyles = 4;
struct Slot { std::string style; float weight = 0.f; bool enabled = false; };
extern Slot slots[MaxStyles];
extern unsigned revision;
inline unsigned Hash(unsigned x, unsigned y, unsigned seed = 0x9e3779b9u) {
    unsigned h = x * 0x85ebca6bu ^ y * 0xc2b2ae35u ^ seed;
    h ^= h >> 16; h *= 0x7feb352du; h ^= h >> 15;
    h *= 0x846ca68bu; h ^= h >> 16;
    return h;
}
inline float Unit(unsigned hash) {
    return (hash & 0x00ffffffu) * (1.f / 16777216.f);
}
inline int Count() {
    int count = 0;
    for (int i = 0; i < MaxStyles; ++i)
        if (slots[i].enabled && !slots[i].style.empty() && slots[i].weight > 0.f) ++count;
    return count;
}
inline bool Active() { return Count() >= 2; }
inline int Pick(float unit) {
    float total = 0.f;
    for (int i = 0; i < MaxStyles; ++i)
        if (slots[i].enabled && !slots[i].style.empty() && slots[i].weight > 0.f)
            total += slots[i].weight;
    if (total <= 0.f) return -1;
    const float selected = (std::max)(0.f, (std::min)(0.999999f, unit)) * total;
    float at = 0.f;
    for (int i = 0; i < MaxStyles; ++i) {
        if (!slots[i].enabled || slots[i].style.empty() || slots[i].weight <= 0.f) continue;
        at += slots[i].weight;
        if (selected < at) return i;
    }
    return -1;
}
inline void Dirty() { ++revision; }
void ResetInstances();
bool Ready();
void InvalidateMask();
void Refresh(float cameraX, float cameraZ, int terrainId, bool superflat, float flatHeight,
             int memblockId, float quantity);
void Save(const std::string& levelFolder);
void Load(const std::string& levelFolder);
}
