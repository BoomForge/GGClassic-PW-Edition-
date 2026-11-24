#include "PW-GrassMix.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <limits>
namespace PWGrassMix { Slot slots[MaxStyles]; unsigned revision = 0; }
int main() {
    using namespace PWGrassMix;
    assert(!Active());
    slots[0] = {"Short Grass", 25.f, true};
    slots[1] = {"Tall Grass", 75.f, true};
    assert(Active());
    assert(Pick(0.0f) == 0);
    assert(Pick(0.249f) == 0);
    assert(Pick(0.251f) == 1);
    assert(Pick(0.999f) == 1);
    assert(Hash(100, 200) == Hash(100, 200));
    assert(Hash(100, 200) != Hash(101, 200));
    // Normalised world dimensions must respect native VegWidth/VegHeight,
    // custom-style size settings, and MUST apply scale only once to instances.
    InstanceSizing size;
    assert(ComputeInstanceSizing(25.f, 10.f, 50.f, 50.f, 100.f, size));
    assert(std::fabs(size.xz-200.f)<0.001f);
    assert(std::fabs(size.y-500.f)<0.001f);
    assert(ComputeInstanceSizing(25.f, 10.f, 50.f, 50.f, 200.f, size));
    assert(std::fabs(size.xz-400.f)<0.001f);
    assert(std::fabs(size.y-1000.f)<0.001f);
    assert(!ComputeInstanceSizing(0.f, 10.f, 50.f, 50.f, 100.f, size));
    assert(!ComputeInstanceSizing(25.f, 0.f, 50.f, 50.f, 100.f, size));
    assert(!ComputeInstanceSizing(25.f, 10.f, 0.f, 50.f, 100.f, size));
    assert(!ComputeInstanceSizing(25.f, 10.f, 50.f, 50.f,
        std::numeric_limits<float>::infinity(), size));
    slots[1].enabled = false;
    assert(!Active());
    assert(Pick(0.5f) == 0);
    slots[0].weight = 0.f;
    assert(Pick(0.5f) == -1);
    puts("PW weighted multi-grass selection tests passed");
    return 0;
}
