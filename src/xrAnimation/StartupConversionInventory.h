#pragma once

#include "OzzMotionLibrary.h"
#include "OzzSkeletonMirror.h"

namespace XRay::Animation::Startup
{
struct PreparedModel
{
    std::shared_ptr<const OzzSkeletonMirror> skeleton;
    xr_vector<xr_string> libraries;
    CPartition partition;
};

xr_string CanonicalPath(pcstr path);
bool LibraryExists(const xr_string& path);
std::shared_ptr<const OzzSkeletonMirror> PrepareSkeleton(const xr_vector<OzzBoneDesc>& bones);
xr_string PrepareLibrary(pcstr source, const void* data, size_t size);
CPartition LibraryPartition(const xr_string& path, const OzzSkeletonMirror& skeleton);
void RegisterModel(const xr_string& source, const xr_string& levelRoot, PreparedModel model);
void BuildInventory();
}
