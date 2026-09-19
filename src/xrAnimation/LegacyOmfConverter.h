#pragma once

#include "xrAnimation.h"
#include "xrCore/Animation/SkeletonMotions.hpp"
#include <cstddef>
#include <ozz/animation/runtime/animation.h>
#include <ozz/base/memory/unique_ptr.h>

namespace XRay::Animation
{
struct OzzSkeletonMirror;

struct ConvertedOmfLibrary
{
    MotionLibraryMetadata metadata;
    xr_vector<ozz::unique_ptr<ozz::animation::Animation>> animations;
};

XRANIMATION_API ConvertedOmfLibrary ConvertLegacyOmf(const std::byte* data, size_t size,
    pcstr source, const OzzSkeletonMirror& skeleton);
}
