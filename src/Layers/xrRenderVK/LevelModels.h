#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "SlidingWindows.h"

namespace xray::render::vulkan
{
struct VisualRecord;
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

enum class LevelGameProfile : uint8_t
{
    SoC,
    CS,
    CoP
};
const char *level_profile_name(LevelGameProfile profile);
bool validate_level_header(LevelGameProfile profile, LevelBytes header, std::string &error);

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
    float lightmap_uv[2]{};
    float baked[4]{1.f, 1.f, 1.f, 1.f};
};

struct LevelModel
{
    uint16_t material{};
    bool lightmap_uv{};
    std::vector<LevelVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<SlideWindow> windows;
    std::shared_ptr<LevelModel> fast;
};

// The index of a visual in the level OGF table is stable. Hierarchies keep
// their child references instead of duplicating every child as a scene draw.
struct LevelVisual
{
    uint8_t type{};
    int32_t mesh{-1};
    std::array<float, 10> bounds{}; // OGF box min/max, sphere center/radius
    std::vector<uint32_t> children;
    std::array<int32_t, 8> lod_facets{-1, -1, -1, -1, -1, -1, -1, -1};
    std::array<std::array<float, 3>, 8> lod_normals{};
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
bool load_level_models(LevelBytes shaders, LevelBytes vertex_buffers, LevelBytes index_buffers, LevelBytes visuals, LevelModelData &result, std::string &error,
                       LevelBytes fast_vertex_buffers = {}, LevelBytes fast_index_buffers = {}, LevelBytes tree_windows = {});
bool load_game_level_models(LevelGameProfile profile, LevelBytes header, LevelBytes shaders, LevelBytes vertex_buffers, LevelBytes index_buffers,
                            LevelBytes visuals, LevelModelData &result, std::string &error, LevelBytes fast_vertex_buffers = {},
                            LevelBytes fast_index_buffers = {}, LevelBytes tree_windows = {});
// Resolve a standalone visual's GCONTAINER against level.geom, using the
// same bounds and index validation as the level visual table.
bool load_container_model(LevelBytes vertex_buffers, LevelBytes index_buffers, const VisualRecord &visual, LevelModel &result, std::string &error,
                          LevelBytes tree_windows = {});
bool parse_level_visibility(LevelBytes portals, LevelBytes sectors,
    LevelModelData& result, std::string& error);
}
