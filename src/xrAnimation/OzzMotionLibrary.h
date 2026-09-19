#pragma once

#include "xrAnimation.h"
#include "xrCore/Animation/SkeletonMotions.hpp"
#include <memory>
#include <ozz/animation/runtime/animation.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>

namespace XRay::Animation
{
struct OzzSkeletonMirror;

struct OzzMotionLibrary
{
    MotionLibraryMetadata metadata;
    xr_vector<ozz::unique_ptr<ozz::animation::Animation>> animations;
    xr_vector<ozz::vector<ozz::math::SoaTransform>> firstFrame;
};

struct OzzModelAnimations
{
    std::shared_ptr<const OzzSkeletonMirror> skeleton;
    xr_vector<std::shared_ptr<const OzzMotionLibrary>> libraries;
    CPartition partition;
};

XRANIMATION_API std::shared_ptr<const OzzModelAnimations> LoadOzzModelAnimations(pcstr modelName);
XRANIMATION_API void PrepareOzzAnimationInventory();
XRANIMATION_API void ShutdownOzzAnimations();
XRANIMATION_API void DumpOzzAnimationStats();
}
