#include "stdafx.h"

#include "OzzKinematicsAnimated.h"

namespace XRay
{
namespace Animation
{
void OzzKinematicsAnimated::Load(const char* N, IReader* data, u32 dwFlags)
{
    xray::render::fg::CKinematicsAnimated::Load(N, data, dwFlags);

    const u16 boneCount = LL_BoneCount();
    xr_vector<OzzBoneDesc> descs(boneCount);
    for (u16 i = 0; i < boneCount; ++i)
    {
        const CBoneData& bd = LL_GetData(i);
        descs[i].name = bd.name;
        descs[i].parent = bd.GetParentID();
        descs[i].bind_local = bd.bind_transform;
    }

    mirror = BuildOzzSkeletonMirror(ozz::span<const OzzBoneDesc>(descs.data(), descs.size()));
}

void OzzKinematicsAnimated::Copy(xray::render::fg::dxRender_Visual* P)
{
    xray::render::fg::CKinematicsAnimated::Copy(P);
    mirror = static_cast<OzzKinematicsAnimated*>(P)->mirror;
}
}
}
