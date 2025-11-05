#pragma once

#include "xrCommon/xr_string.h"
#include "xrCommon/xr_vector.h"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>

#include "xrCore/_matrix.h"
#include "xrCore/_quaternion.h"
#include "xrCore/_vector3d.h"
#include "xrCore/Animation/SkeletonMotionDefs.hpp"
#include "xrCore/Animation/SkeletonMotions.hpp"

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/memory/unique_ptr.h>

namespace XRay
{
namespace Animation
{
struct LegacyMotionInterval
{
    float first = 0.f;
    float second = 0.f;
};

struct LegacyMotionMark
{
    xr_string name;
    xr_vector<LegacyMotionInterval> intervals;
};

struct LegacyMotionMetadata
{
    xr_string name;
    u32 flags = 0;
    u16 bone_or_part = 0;
    u16 motion_id = 0;
    float speed = 0.f;
    float power = 0.f;
    float accrue = 0.f;
    float falloff = 0.f;
    xr_vector<LegacyMotionMark> marks;
};

struct LegacyBoneTrack
{
    xr_vector<Fquaternion> rotations;
    xr_vector<Fvector> translations;
    xr_vector<CKeyQR> rotation_keys;
    xr_vector<CKeyQT8> translation_keys8;
    xr_vector<CKeyQT16> translation_keys16;
    Fvector translation_init{};
    Fvector translation_size{};
    u8 flags = 0;
    u32 rotation_crc = 0;
    u32 translation_crc = 0;
};

struct LegacyOmfMotion
{
    xr_string name;
    u32 frame_count = 0;
    xr_vector<LegacyBoneTrack> bone_tracks;
    LegacyMotionMetadata metadata;
};

struct ConvertedBoneMotion
{
    u16 bone_id = BI_NONE;
    u8 flags = 0;
    u32 rotation_crc = 0;
    u32 translation_crc = 0;
    xr_vector<CKeyQR> rotation_keys;
    xr_vector<CKeyQT8> translation_keys8;
    xr_vector<CKeyQT16> translation_keys16;
    Fvector translation_init{};
    Fvector translation_size{};
};

struct LegacyOmfData
{
    xr_vector<u16> bone_remap;
    xr_vector<xr_string> remap_bone_names;
    xr_vector<LegacyMotionMetadata> metadata;
    xr_vector<LegacyOmfMotion> motions;
};

struct ConvertedOmfAnimation
{
    xr_string name;
    LegacyMotionMetadata metadata;
    ozz::unique_ptr<ozz::animation::Animation> animation;
    u32 frame_count = 0;
    xr_vector<ConvertedBoneMotion> bone_motions;
};

bool ConvertLegacyOmf(const std::filesystem::path& omf_path,
                      const xr_vector<xr_string>& skeleton_bone_names,
                      const ozz::animation::Skeleton& skeleton,
                      xr_vector<ConvertedOmfAnimation>& out_animations,
                      std::optional<xr_string> motion_filter = std::nullopt,
                      bool optimize = false);

bool ConvertLegacyOmf(const std::byte* data,
                      size_t size,
                      const xr_vector<xr_string>& skeleton_bone_names,
                      const ozz::animation::Skeleton& skeleton,
                      xr_vector<ConvertedOmfAnimation>& out_animations,
                      std::optional<xr_string> motion_filter = std::nullopt,
                      bool optimize = false);

void SerializeBoneMotions(ozz::io::OArchive& archive, const ConvertedOmfAnimation& animation);
}
} // namespace XRay::Animation
