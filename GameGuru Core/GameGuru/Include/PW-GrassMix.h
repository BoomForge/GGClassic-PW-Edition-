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
// Scales a native grass mesh to the SAME world-size controls used by the
// Classic vegetation grid. A hidden source object remains at 100%; only
// visible instances use these percentages (otherwise scale is doubled).
struct InstanceSizing { float xz = 0.f; float y = 0.f; };
inline bool ComputeInstanceSizing(float rawWidth, float rawHeight,
                                  float vegWidth, float vegHeight,
                                  float stylePercent, InstanceSizing& out) {
    out = InstanceSizing();
    if (!std::isfinite(rawWidth) || !std::isfinite(rawHeight) ||
        !std::isfinite(vegWidth) || !std::isfinite(vegHeight) ||
        !std::isfinite(stylePercent) || rawWidth < 0.01f ||
        rawHeight < 0.01f || vegWidth <= 0.f || vegHeight <= 0.f)
        return false;
    const float ratio = (std::max)(0.05f, (std::min)(10.f, stylePercent/100.f));
    const float xz = 100.f * vegWidth * ratio / rawWidth;
    const float y = 100.f * vegHeight * ratio / rawHeight;
    if (!std::isfinite(xz) || !std::isfinite(y) || xz <= 0.f || y <= 0.f ||
        xz > 100000.f || y > 100000.f)
        return false;
    out.xz = xz;
    out.y = y;
    return true;
}
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
// Selecting multiple styles only CONFIGURES a mix. The experimental
// standalone-mesh renderer must be explicitly enabled to avoid blank maps.
bool PreviewEnabled();
bool RenderEnabled();
void SetPreviewEnabled(bool enabled);
std::string Diagnostics();
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
// True only when at least one real mixed instance was populated near the view.
// Merely loading style resources must NEVER hide Classic's visible grass grid.
bool OwnsGrid();
// Match Classic's temporary shadow-pass hide/restore without showing stale instances.
void SetInstancesVisible(bool visible);
void InvalidateMask();
void Refresh(float cameraX, float cameraZ, int terrainId, bool superflat, float flatHeight,
             int memblockId, float quantity);
void Save(const std::string& levelFolder);
void Load(const std::string& levelFolder);
}
