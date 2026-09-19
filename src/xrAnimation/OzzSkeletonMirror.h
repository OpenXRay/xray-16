#pragma once

#include <memory>

#include "xrCore/xrCore.h"
#include "xrCore/Animation/Bone.hpp"
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/span.h>

namespace XRay
{
namespace Animation
{
struct OzzBoneDesc
{
    shared_str name;
    u16 parent{ BI_NONE };
    Fmatrix bind_local;
};

struct OzzSkeletonMirror
{
    ozz::animation::Skeleton skeleton;
    xr_vector<u16> boneToJoint;
    xr_vector<u16> jointToBone;
    u32 fingerprint{ 0u };
};

std::shared_ptr<const OzzSkeletonMirror> BuildOzzSkeletonMirror(ozz::span<const OzzBoneDesc> bones);
}
}
