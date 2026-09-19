#pragma once

#include "xrCore/xrCore.h"

#include <ozz/animation/runtime/animation.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>

class IReader;

namespace XRay
{
namespace Animation
{
struct OzzSkeletonMirror;

struct OzzMotionLibrary
{
    shared_str key;
    u32 fingerprint{ 0u };
    xr_vector<ozz::unique_ptr<ozz::animation::Animation>> animations;
    xr_vector<ozz::vector<ozz::math::SoaTransform>> firstFrame;
    u32 refs{ 0u };
};

const ozz::vector<ozz::math::SoaTransform>& FirstFrame(OzzMotionLibrary& library, u16 idx);

bool PrebakeMotionLibrary(const shared_str& omf_key, IReader* omf, const OzzSkeletonMirror& mirror);

class OzzMotionLibraryContainer
{
    using LibraryMap = xr_map<shared_str, OzzMotionLibrary*>;
    LibraryMap container;

public:
    ~OzzMotionLibraryContainer();

    OzzMotionLibrary* dock(const shared_str& omf_key, IReader* omf, const OzzSkeletonMirror& mirror);
    void undock(OzzMotionLibrary* library);
    void clean(bool force);
};

extern OzzMotionLibraryContainer* g_pOzzMotionLibraries;
}
}
