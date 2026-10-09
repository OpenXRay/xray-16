#pragma once

#include "LevelModels.h"
#include <string>
#include <unordered_map>

class IReader;

namespace xray::render::vulkan
{
// Reads the blender class and saved properties from the engine's shaders.xr.
// This is deliberately independent of the GLES resource manager.
class ShaderMaterialLibrary
{
public:
    bool load(IReader& file, std::string& error);
    bool resolve(const std::string& shader, SurfaceMode& mode, std::string& error,
        int* alpha_ref = nullptr, int* blend_mode = nullptr,
        bool particle_pipeline = false, bool screen_pipeline = false,
        bool level_pipeline = false, bool water_pipeline = false,
        uint32_t* material_flags = nullptr) const;
    bool contains(const std::string& shader) const;
    size_t size() const { return entries_.size(); }
    void clear() { entries_.clear(); }

private:
    struct Entry
    {
        uint32_t id{};
        SurfaceMode mode{};
        std::string class_name;
        bool supported{};
        uint16_t version{};
        int alpha_ref{-1};
        int blending{-1};
        uint32_t screen_flags{};
    };
    std::unordered_map<std::string, Entry> entries_;
};
}
