#include "PW-ThumbnailCheck.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <vector>
static void u16(std::vector<unsigned char>& b, size_t i, unsigned n) {
    b[i] = static_cast<unsigned char>(n);
    b[i+1] = static_cast<unsigned char>(n >> 8);
}
static void u32(std::vector<unsigned char>& b, size_t i, unsigned n) {
    for (int k = 0; k < 4; ++k) b[i+k] = static_cast<unsigned char>(n >> (k*8));
}
static void writeBmp(const char* path, bool varied) {
    std::vector<unsigned char> b(54 + 32*32*4, 0);
    b[0] = 'B'; b[1] = 'M';
    u32(b, 2, unsigned(b.size()));
    u32(b, 10, 54); u32(b, 14, 40);
    u32(b, 18, 32); u32(b, 22, 32); u16(b, 26, 1); u16(b, 28, 32);
    for (unsigned y = 0; y < 32; ++y)
        for (unsigned x = 0; x < 32; ++x) {
            const size_t at = 54 + (y*32+x)*4;
            b[at] = varied && x > 8 && x < 24 ? 220 : 1;
            b[at+1] = varied && y > 8 && y < 24 ? 80 : 254;
            b[at+2] = varied && x > 8 ? 100 : 1;
            b[at+3] = 255;
        }
    std::ofstream f(path, std::ios::binary);
    assert(f.good());
    f.write(reinterpret_cast<const char*>(b.data()), b.size());
}
int main() {
    const char* path = "pw-thumb-test.bmp";
    writeBmp(path, false);
    assert(!PWThumbnail::HasContent(path)); // Serine's solid-green failure
    writeBmp(path, true);
    assert(PWThumbnail::HasContent(path));
    std::remove(path);
    puts("PW thumbnail content checks passed");
    return 0;
}
