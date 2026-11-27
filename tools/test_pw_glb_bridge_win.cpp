// PW Edition Model Importer 2: GLB gate and source-folder regression checks.
#include "PW-GLBBridge.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

static void writeLE(std::vector<unsigned char>& bytes, size_t at, unsigned value)
{
    for (int i = 0; i < 4; ++i) bytes[at + i] = static_cast<unsigned char>(value >> (i * 8));
}
static void writeFile(const std::string& path, const std::vector<unsigned char>& bytes)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    assert(stream.good());
    stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    assert(stream.good());
}
int main()
{
    const std::string file = "pw-glb-validation-test.glb";
    std::vector<unsigned char> bytes(24, 0);
    bytes[0] = 'g'; bytes[1] = 'l'; bytes[2] = 'T'; bytes[3] = 'F';
    writeLE(bytes, 4, 2);
    writeLE(bytes, 8, static_cast<unsigned>(bytes.size()));
    writeLE(bytes, 12, 4);
    writeLE(bytes, 16, 0x4E4F534A); // JSON
    bytes[20] = '{'; bytes[21] = '}'; bytes[22] = ' '; bytes[23] = ' ';
    std::string problem;
    writeFile(file, bytes);
    assert(PWGLB::IsGLB("C:\\Models\\avatar.GLB"));
    assert(PWGLB::Validate(file, problem));
    writeLE(bytes, 4, 1); writeFile(file, bytes);
    assert(!PWGLB::Validate(file, problem));
    writeLE(bytes, 4, 2); writeLE(bytes, 8, 99); writeFile(file, bytes);
    assert(!PWGLB::Validate(file, problem));
    writeLE(bytes, 8, 24); bytes[20] = '['; writeFile(file, bytes);
    assert(!PWGLB::Validate(file, problem));
    std::remove(file.c_str());
    assert(PWImport::DirectoryOf("C:\\GG\\Files\\importer\\temp\\pw_glb_42\\model.fbx")
        == "C:\\GG\\Files\\importer\\temp\\pw_glb_42\\");
    assert(PWImport::DirectoryOf("model.fbx").empty());
    puts("PW GLB validation and working-folder checks passed.");
    return 0;
}
