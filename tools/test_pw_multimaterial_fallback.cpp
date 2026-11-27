#include "PW-MultiMaterialFallback.h"
#include <cassert>
#include <set>
#include <string>
#include <iostream>
int main() {
    const std::set<std::string> files = {
        "entitybank\\user\\SerineAlbidoIdle.png",
        "entitybank\\user\\another.dds",
        "entitybank\\user\\another.png"
    };
    const auto exists = [&files](const std::string& path) {
        return files.count(path)>0;
    };
    using PWMultiMaterialFallback::ResolveMissingDDS;
    assert(ResolveMissingDDS("entitybank\\user",
        "SerineAlbidoIdle.dds",exists) == "entitybank\\user\\SerineAlbidoIdle.png");
    assert(ResolveMissingDDS("entitybank\\user\\",
        "C:\\old\\SerineAlbidoIdle.DDS",exists) == "entitybank\\user\\SerineAlbidoIdle.png");
    assert(ResolveMissingDDS("entitybank\\user\\",
        "another.dds",exists).empty()); // Original DDS exists: never override.
    assert(ResolveMissingDDS("entitybank\\user\\",
        "SerineAlbidoIdle.png",exists).empty()); // Explicit PNG left alone.
    assert(ResolveMissingDDS("entitybank\\user\\",
        "missing.dds",exists).empty());
    std::cout << "PW multi-material DBO -> PNG fallback: PASS\n";
}
