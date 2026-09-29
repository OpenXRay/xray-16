#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
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

struct LevelModelData
{
    std::vector<LevelMaterial> materials;
    std::vector<LevelModel> models;
};

// A missing/unsupported format fails the entire load. This ensures that a
// partially rendered level cannot accidentally be advertised as playable.
bool load_level_models(LevelBytes shaders, LevelBytes vertex_buffers,
    LevelBytes index_buffers, LevelBytes visuals, LevelModelData& result,
    std::string& error);
}
