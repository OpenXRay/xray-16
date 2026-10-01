#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
enum class SurfaceMode : uint8_t
{
    Opaque,
    AlphaTest,
    Transparent
};

SurfaceMode classify_surface_material(const std::string& shader, const std::string& texture);

// Decoded static subset of level.geom and the level's OGF visual table. The
// caller supplies decompressed chunk payloads from the engine's IReader.
struct LevelBytes
{
    const uint8_t* data{};
    size_t size{};
};

struct LevelMaterial
{
    std::string shader;
    std::string textures;
    SurfaceMode mode{SurfaceMode::Opaque};
};

struct LevelVertex
{
    float position[3]{};
    float normal[3]{};
    float uv[2]{};
};

struct LevelModel
{
    uint16_t material{};
    std::vector<LevelVertex> vertices;
    std::vector<uint32_t> indices;
};

// The index of a visual in the level OGF table is stable. Hierarchies keep
// their child references instead of duplicating every child as a scene draw.
struct LevelVisual
{
    uint8_t type{};
    int32_t mesh{-1};
    std::array<float, 10> bounds{}; // OGF box min/max, sphere center/radius
    std::vector<uint32_t> children;
};

struct LevelPortal
{
    uint16_t sector_front{};
    uint16_t sector_back{};
    std::vector<std::array<float, 3>> vertices;
    std::array<float, 3> center{};
    float radius{};
};

struct LevelSector
{
    uint32_t root{};
    std::vector<uint16_t> portals;
};

struct LevelModelData
{
    std::vector<LevelMaterial> materials;
    std::vector<LevelModel> models;
    std::vector<LevelVisual> visuals;
    std::vector<uint32_t> roots;
    std::vector<LevelSector> sectors;
    std::vector<LevelPortal> portals;
};

// A missing/unsupported format fails the entire load. This ensures that a
// partially rendered level cannot accidentally be advertised as playable.
bool load_level_models(LevelBytes shaders, LevelBytes vertex_buffers,
    LevelBytes index_buffers, LevelBytes visuals, LevelModelData& result,
    std::string& error);
bool parse_level_visibility(LevelBytes portals, LevelBytes sectors,
    LevelModelData& result, std::string& error);
}
