#pragma once

#include "xrCommon/xr_string.h"
#include "xrCommon/xr_vector.h"

#include "xrCore/_matrix.h"

#include "ExtendedBoneMetadata.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/memory/unique_ptr.h>
#include <ozz/base/span.h>

namespace XRay
{
namespace Animation
{
struct LegacyVisualInput
{
    std::optional<std::filesystem::path> source_path;
    ozz::span<const std::byte> buffer;
};

struct LegacyVisualConversionOptions
{
    bool build_skeleton = true;
    bool build_mesh = true;
    bool deduplicate_vertices = true;
};

struct LegacyOgfBone
{
    xr_string name;
    xr_string parent_name;
    Fmatrix local_transform{};
    Fmatrix global_transform{};
};

struct LegacyVisualConversionResult
{
    ozz::unique_ptr<ozz::animation::Skeleton> skeleton;
    xr_vector<xr_string> bone_names;
    xr_vector<LegacyOgfBone> bones;
    xr_vector<xr_string> motion_refs;
    std::vector<std::uint8_t> skeleton_binary;
    std::vector<std::uint8_t> mesh_binary;
    size_t mesh_surface_count = 0;
    ExtendedBoneMetadataCollection bone_metadata;
    std::vector<std::uint8_t> user_data;
    std::vector<std::uint8_t> embedded_animation_binary;
    std::uint8_t model_type = 0;  // MT enum value - set by converter based on motion refs
};

bool ConvertLegacyVisualToOzzBundle(const LegacyVisualInput& input,
                                    LegacyVisualConversionResult& out_result,
                                    const LegacyVisualConversionOptions& options = {},
                                    xr_string* out_error = nullptr);

bool ConvertLegacyVisualToOzzBundle(const std::filesystem::path& ogf_path,
                                    LegacyVisualConversionResult& out_result,
                                    const LegacyVisualConversionOptions& options = {},
                                    xr_string* out_error = nullptr);
} // namespace Animation
} // namespace XRay
