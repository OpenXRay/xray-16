#pragma once

#include "OzzMotionLibrary.h"
#include "OzzSkeletonMirror.h"

namespace XRay::Animation::Startup
{
struct PreparedModel
{
    xr_string skeleton;
    xr_vector<xr_string> libraries;
    CPartition partition;
};

xr_string CanonicalPath(pcstr path);
xr_string ContentKey(const void* data, size_t size);
xr_string PrepareSkeleton(const xr_vector<OzzBoneDesc>& bones, std::shared_ptr<const OzzSkeletonMirror>& mirror);
xr_string PrepareLibrary(pcstr source, const void* data, size_t size,
    const xr_string& skeletonKey, const OzzSkeletonMirror& mirror, CPartition& partition);
void RegisterModel(const xr_string& source, const xr_string& levelRoot, PreparedModel model);
void BuildInventory();
}
