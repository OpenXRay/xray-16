#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_matrix.h"
#include "xrCore/_vector2.h"

namespace xray::render::RENDER_NAMESPACE {
class CKinematics;
}

namespace xray::render::RENDER_NAMESPACE::decals {

struct MeshPickResult {
    float dist;
    Fvector worldPos;
    Fvector worldNormal;
    Fvector2 uv;
    u16 boneID;
    CKinematics* parent;
    Fvector triWorldEdge1;
    Fvector triWorldEdge2;
    Fvector2 triUV[3];
};

bool PickMeshDirect(
    CKinematics* obj,
    const Fmatrix& parentXForm,
    const Fvector& rayStart,
    const Fvector& rayDir,
    float maxDist,
    MeshPickResult& result);

} // namespace xray::render::RENDER_NAMESPACE::decals
