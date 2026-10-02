#pragma once

#include "LevelModels.h"

#include <array>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
struct SkeletonBone
{
    std::string name;
    uint16_t parent = UINT16_MAX;
    std::array<uint8_t, 60> obb{};
    // One validated OGF_S_IKDATA record, including its version and material.
    std::vector<uint8_t> ik_data;
};

struct SkeletonBones
{
    std::vector<SkeletonBone> bones;
    uint16_t root = UINT16_MAX;
    std::vector<uint8_t> user_data;
};

// Parses raw, uncompressed OGF chunks without touching engine/GPU state.
// The caller has already decompressed the IReader's chunk payload.
bool parse_skeleton_bones(LevelBytes ogf, SkeletonBones& result, std::string& error);
}
