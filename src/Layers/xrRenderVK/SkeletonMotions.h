#pragma once

#include "LevelModels.h"

#include <array>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace xray::render::vulkan
{
struct MotionMark
{
    std::string name;
    std::vector<std::array<float, 2>> intervals;
};

struct MotionDefinition
{
    std::string name;
    uint32_t flags{};
    uint16_t bone_or_part{};
    uint16_t motion{};
    std::array<float, 4> parameters{}; // speed, power, accrue, falloff
    std::vector<MotionMark> marks;
};

struct BoneMotion
{
    uint8_t flags{};
    // Quantized quaternion keys and translation keys, in file order.
    std::vector<std::array<int16_t, 4>> rotations;
    std::vector<std::array<int16_t, 3>> translations16;
    std::vector<std::array<int8_t, 3>> translations8;
    std::array<float, 3> translation_size{};
    std::array<float, 3> translation_init{};
};

struct MotionClip
{
    std::string name;
    uint32_t frames{};
    // Indexed by the skeleton's bone ID, not the OMF partition order.
    std::vector<BoneMotion> bones;
};

struct MotionSlot
{
    std::string source;
    std::vector<uint8_t> raw; // retained for legacy IKinematicsAnimated motion access
    std::vector<std::string> partition_names;
    std::vector<std::vector<uint16_t>> partitions;
    std::vector<MotionDefinition> definitions;
    std::vector<MotionClip> clips;
};

using MotionFile = std::pair<std::string, std::vector<uint8_t>>;
using MotionResolver = std::function<bool(const std::string&, std::vector<MotionFile>&, std::string&)>;

// Reads embedded OGF motions or referenced OMF slots. The resolver receives
// a filename or wildcard including .omf and supplies matching engine IReader bytes.
// Result remains unchanged if any slot is missing or malformed.
bool parse_skeleton_motions(LevelBytes ogf, const std::vector<std::string>& bone_names, const std::string& model_name, const MotionResolver& resolve,
    std::vector<MotionSlot>& result, std::string& error);
} // namespace xray::render::vulkan
