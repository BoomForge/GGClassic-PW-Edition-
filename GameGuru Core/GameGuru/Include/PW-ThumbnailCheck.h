#pragma once
// PW Edition: reject a solid-colour BMP produced by a failed camera capture.
// A valid image file is not proof of a useful model preview.
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace PWThumbnail {
inline unsigned U16(const unsigned char* p) {
    return unsigned(p[0]) | (unsigned(p[1]) << 8);
}
inline std::uint32_t U32(const unsigned char* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
        (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
inline bool HasContent(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return false;
    const std::streamoff length = file.tellg();
    if (length < 54 || length > 64LL * 1024 * 1024) return false;
    file.seekg(0);
    unsigned char head[54] = {};
    file.read(reinterpret_cast<char*>(head), sizeof(head));
    if (!file || head[0] != 'B' || head[1] != 'M' || U32(head + 14) < 40 ||
        U16(head + 26) != 1 || U32(head + 30) != 0) return false;
    const std::uint32_t width = U32(head + 18);
    const std::int32_t signedHeight = static_cast<std::int32_t>(U32(head + 22));
    if (signedHeight == INT32_MIN) return false;
    const std::uint32_t height = signedHeight < 0 ?
        static_cast<std::uint32_t>(-signedHeight) : static_cast<std::uint32_t>(signedHeight);
    const unsigned bits = U16(head + 28);
    if (width < 16 || height < 16 || width > 4096 || height > 4096 ||
        (bits != 24 && bits != 32)) return false;
    const std::uint64_t row = ((std::uint64_t(width) * bits + 31) / 32) * 4;
    const std::uint64_t start = U32(head + 10);
    if (start < 54 || start + row * height > static_cast<std::uint64_t>(length)) return false;
    // Sample the entire preview grid; a plain sky/clear colour is not a model thumbnail.
    unsigned char first[3] = {};
    bool haveFirst = false;
    unsigned variations = 0;
    for (unsigned iy = 0; iy < 12; ++iy) {
        const std::uint32_t y = (height - 1) * iy / 11;
        for (unsigned ix = 0; ix < 12; ++ix) {
            const std::uint32_t x = (width - 1) * ix / 11;
            const std::uint64_t pos = start + row * y + std::uint64_t(x) * (bits / 8);
            file.seekg(static_cast<std::streamoff>(pos));
            unsigned char sample[3] = {};
            file.read(reinterpret_cast<char*>(sample), 3);
            if (!file) return false;
            if (!haveFirst) {
                std::copy(sample, sample + 3, first);
                haveFirst = true;
            } else {
                const int diff = std::abs(int(sample[0])-int(first[0])) +
                                 std::abs(int(sample[1])-int(first[1])) +
                                 std::abs(int(sample[2])-int(first[2]));
                if (diff > 30) ++variations;
            }
        }
    }
    return variations >= 3;
}
} // namespace PWThumbnail
