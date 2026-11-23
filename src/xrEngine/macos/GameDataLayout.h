#pragma once

#include <filesystem>
#include <string>

namespace xray::macos
{
struct GameDataLayoutPaths
{
    std::filesystem::path gameRoot;
    std::filesystem::path bundleResourcesRoot;
    std::filesystem::path runtimeRoot;
    std::filesystem::path appDataRoot;
};

// Link the installation into a separate runtime directory and supply missing OpenGL shaders.
// The source installation and bundle are read-only; error is populated if preparation fails.
bool PrepareGameDataLayout(const GameDataLayoutPaths& paths, std::string& error);
} // namespace xray::macos
