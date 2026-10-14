#pragma once

#include "LevelModels.h"
#include "xrEngine/WaveForm.h"
#include <array>
#include <string>
#include <unordered_map>
#include <vector>

class IReader;

namespace xray::render::vulkan
{
struct TerrainMaterialTextures
{
    std::array<std::string, 4> detail;
    bool enabled{};
};
// A property retains its serialized type and value, including token tables.
// Scope distinguishes the two "Name"/"Transform" slots of terrain and
// environment blenders. A parsed definition is not necessarily renderable.
struct BlenderProperty
{
    std::string scope, name;
    uint32_t type{};
    std::vector<uint8_t> data;
};
struct MaterialPassState
{
    bool depth_test{true};
    bool depth_write{true};
    bool strict_sorting{};
    bool lighting{true};
    bool fog{true};
    bool clamp{true};
};
struct MaterialDefinition
{
    enum class Role { Standard, Water };
    uint32_t id{};
    std::string shader, class_name;
    uint16_t version{};
    SurfaceMode mode{SurfaceMode::Unresolved};
    bool pipeline_supported{};
    std::string unsupported_reason;
    int alpha_ref{-1}, blending{-1};
    uint32_t screen_flags{};
    MaterialPassState pass;
    Role role{Role::Standard};
    TerrainMaterialTextures terrain;
    std::string detail_texture;
    std::string environment_texture, environment_transform, environment_constant;
    std::array<WaveForm, 4> constant_waveforms{};
    bool has_environment_constant{};
    std::vector<BlenderProperty> properties;
};
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
        uint32_t* material_flags = nullptr, TerrainMaterialTextures* terrain = nullptr,
        MaterialDefinition* definition = nullptr) const;
    const MaterialDefinition* find(const std::string& shader) const;
    bool contains(const std::string& shader) const;
    size_t size() const { return entries_.size(); }
    void clear() { entries_.clear(); }

private:
    std::unordered_map<std::string, MaterialDefinition> entries_;
};
}
