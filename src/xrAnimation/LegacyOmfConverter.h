#pragma once

#include "xrCommon/xr_string.h"
#include "xrCommon/xr_vector.h"

#include <cstddef>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/memory/unique_ptr.h>

namespace XRay
{
namespace Animation
{
struct ConvertedOmfAnimation
{
    xr_string name;
    u32 frame_count = 0;
    ozz::unique_ptr<ozz::animation::Animation> animation;
};

bool ConvertLegacyOmf(const std::byte* data, size_t size, const xr_vector<xr_string>& skeleton_bone_names,
    const ozz::animation::Skeleton& skeleton, xr_vector<ConvertedOmfAnimation>& out_animations);
}
}
