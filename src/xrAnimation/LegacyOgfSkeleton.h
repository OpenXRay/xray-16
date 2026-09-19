#pragma once

#include "xrCore/xrCore.h"
#include "xrCommon/xr_vector.h"

#include "OzzSkeletonMirror.h"

class IReader;

namespace XRay
{
namespace Animation
{
bool ReadOgfSkeleton(
    IReader* ogf, xr_vector<OzzBoneDesc>& bones, xr_vector<shared_str>& motion_refs, bool& has_embedded_motions);
}
}
