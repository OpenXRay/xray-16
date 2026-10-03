#pragma once

#include "LevelModels.h"

#include <array>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
struct DetailPrototype
{
    std::string shader, texture;
    float min_scale{}, max_scale{};
    std::vector<LevelVertex> vertices;
    std::vector<uint32_t> indices;
};

struct DetailCell
{
    float ground{}, height{};
    std::array<uint8_t, 4> ids{};
    std::array<std::array<uint8_t, 4>, 4> palette{};
};

struct DetailAssets
{
    int32_t offset_x{}, offset_z{};
    uint32_t width{}, height{};
    std::vector<DetailPrototype> prototypes;
    std::vector<DetailCell> cells;
};

// The three decompressed chunks of level.details. No engine pointers or
// renderer resources escape the decoder; failure leaves result unchanged.
bool decode_detail_assets(LevelBytes header, const std::vector<LevelBytes>& objects,
    LevelBytes slots, DetailAssets& result, std::string& error);
}
