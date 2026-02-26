#include "stdafx.h"
#include "MeshPicker.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/SkeletonX.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/FSkinnedTypes.h"
#include "xrCDB/Intersect.hpp"

namespace xray::render::RENDER_NAMESPACE::decals {

struct TriPickResult {
    float dist;
    float baryU, baryV;
    Fvector skinnedPos[3];
    u32 vertIdx[3];
};

inline float decode_tc(float v) { return v; }
inline float decode_tc(s16 v) { return float(v) / (32767.f / 16.f); }

template <typename T> static Fvector2 GetHWUV(const vertHW_1W<T>& v) { return { decode_tc(v.TexCoord[0]), decode_tc(v.TexCoord[1]) }; }
template <typename T> static Fvector2 GetHWUV(const vertHW_2W<T>& v) { return { decode_tc(v.TexCoord_and_index[0]), decode_tc(v.TexCoord_and_index[1]) }; }
template <typename T> static Fvector2 GetHWUV(const vertHW_3W<T>& v) { return { decode_tc(v.TexCoord_and_index[0]), decode_tc(v.TexCoord_and_index[1]) }; }
template <typename T> static Fvector2 GetHWUV(const vertHW_4W<T>& v) { return { decode_tc(v.TexCoord[0]), decode_tc(v.TexCoord[1]) }; }

template <typename T> static u16 GetHWBone(const vertHW_1W<T>& v) { return v.get_bone(); }
template <typename T> static u16 GetHWBone(const vertHW_2W<T>& v) { return v.get_bone(0); }
template <typename T> static u16 GetHWBone(const vertHW_3W<T>& v) { return v.get_bone(0); }
template <typename T> static u16 GetHWBone(const vertHW_4W<T>& v) { return v.get_bone(0); }

template <typename T>
static TriVertexSkin ExtractSkin(const vertHW_1W<T>& v) {
    TriVertexSkin s;
    v.get_pos(s.restPos);
    s.boneIdx[0] = v.get_bone();
    s.boneIdx[1] = s.boneIdx[2] = s.boneIdx[3] = 0;
    s.weight[0] = 1.f;
    s.weight[1] = s.weight[2] = s.weight[3] = 0.f;
    return s;
}

template <typename T>
static TriVertexSkin ExtractSkin(const vertHW_2W<T>& v) {
    TriVertexSkin s;
    v.get_pos(s.restPos);
    s.boneIdx[0] = v.get_bone(0);
    s.boneIdx[1] = v.get_bone(1);
    s.boneIdx[2] = s.boneIdx[3] = 0;
    float w = v.get_weight();
    s.weight[0] = 1.f - w;
    s.weight[1] = w;
    s.weight[2] = s.weight[3] = 0.f;
    return s;
}

template <typename T>
static TriVertexSkin ExtractSkin(const vertHW_3W<T>& v) {
    TriVertexSkin s;
    v.get_pos(s.restPos);
    s.boneIdx[0] = v.get_bone(0);
    s.boneIdx[1] = v.get_bone(1);
    s.boneIdx[2] = v.get_bone(2);
    s.boneIdx[3] = 0;
    s.weight[0] = v.get_weight0();
    s.weight[1] = v.get_weight1();
    s.weight[2] = 1.f - s.weight[0] - s.weight[1];
    s.weight[3] = 0.f;
    return s;
}

template <typename T>
static TriVertexSkin ExtractSkin(const vertHW_4W<T>& v) {
    TriVertexSkin s;
    v.get_pos(s.restPos);
    for (u16 i = 0; i < 4; i++)
        s.boneIdx[i] = v.get_bone(i);
    s.weight[0] = v.get_weight0();
    s.weight[1] = v.get_weight1();
    s.weight[2] = v.get_weight2();
    s.weight[3] = 1.f - s.weight[0] - s.weight[1] - s.weight[2];
    return s;
}

struct UVBoneResult {
    Fvector2 uv;
    u16 boneID;
    Fvector2 triUV[3];
    TriVertexSkin triVerts[3];
};

template <typename T_vertex>
static bool PickAllTrisHW(T_vertex* vertices, CKinematics* parent, const Fvector& S, const Fvector& D,
    u16* indices, u32 numIndices, float& bestDist, TriPickResult& bestHit)
{
    bool found = false;
    for (u32 tri = 0; tri < numIndices / 3; tri++) {
        u32 idx = tri * 3;
        Fvector skinned[3];
        for (u32 k = 0; k < 3; k++)
            vertices[indices[idx + k]].get_pos_bones(skinned[k], parent);

        float u, v, dist = flt_max;
        if (CDB::TestRayTri(S, D, skinned, u, v, dist, false) && dist > 0.f && dist < bestDist) {
            bestDist = dist;
            bestHit.dist = dist;
            bestHit.baryU = u;
            bestHit.baryV = v;
            for (u32 k = 0; k < 3; k++) {
                bestHit.skinnedPos[k] = skinned[k];
                bestHit.vertIdx[k] = indices[idx + k];
            }
            found = true;
        }
    }
    return found;
}

template <typename T_vertex>
static UVBoneResult ExtractHW(T_vertex* vertices, const TriPickResult& hit)
{
    UVBoneResult r;
    for (u32 k = 0; k < 3; k++) {
        r.triUV[k] = GetHWUV(vertices[hit.vertIdx[k]]);
        r.triVerts[k] = ExtractSkin(vertices[hit.vertIdx[k]]);
    }
    float w0 = 1.f - hit.baryU - hit.baryV;
    r.uv = { r.triUV[0].x * w0 + r.triUV[1].x * hit.baryU + r.triUV[2].x * hit.baryV,
              r.triUV[0].y * w0 + r.triUV[1].y * hit.baryU + r.triUV[2].y * hit.baryV };
    r.boneID = GetHWBone(vertices[hit.vertIdx[0]]);
    return r;
}

template <typename T_vertex>
static bool TryPickChild(void* vbData, CKinematics* parent, const Fvector& S, const Fvector& D,
    u16* indices, u32 indexCount, float& bestDist, TriPickResult& bestHit, UVBoneResult& uvOut, bool& uvValid)
{
    T_vertex* verts = static_cast<T_vertex*>(vbData);
    TriPickResult childHit;
    float childBest = bestDist;
    if (PickAllTrisHW(verts, parent, S, D, indices, indexCount, childBest, childHit) && childBest < bestDist) {
        bestDist = childBest;
        bestHit = childHit;
        uvOut = ExtractHW(verts, childHit);
        uvValid = true;
        return true;
    }
    return false;
}

bool PickMeshDirect(
    CKinematics* obj,
    const Fmatrix& parentXForm,
    const Fvector& rayStart,
    const Fvector& rayDir,
    float maxDist,
    MeshPickResult& result)
{
    Fmatrix invParent;
    invParent.invert(parentXForm);
    Fvector S, D;
    invParent.transform_tiny(S, rayStart);
    invParent.transform_dir(D, rayDir);
    D.normalize();

    float bestDist = maxDist;
    TriPickResult bestHit;
    bestHit.dist = flt_max;
    UVBoneResult bestUV;
    bool hasHit = false;
    xr_string bestTextureName;

    for (u32 i = 0; i < obj->children.size(); i++) {
        auto* skelChild = dynamic_cast<CSkeletonX*>(obj->children[i]);
        if (!skelChild)
            continue;

        Fvisual* V = dynamic_cast<Fvisual*>(obj->children[i]);
        if (!V || !V->p_rm_Indices || !V->p_rm_Vertices)
            continue;

        u32 indexBase = V->iBase;
        u32 indexCount = V->iCount;

        auto* pm = dynamic_cast<CSkeletonX_PM*>(obj->children[i]);
        if (pm) {
            const FSlideWindowItem& swi = pm->GetSWI();
            if (swi.count > 0 && swi.sw) {
                indexBase += swi.sw[0].offset;
                indexCount = swi.sw[0].num_tris * 3;
            }
        }

        if (indexCount == 0)
            continue;

        u16* allIndices = static_cast<u16*>(V->p_rm_Indices->Map(0, V->dwPrimitives * 3, true));
        void* vbData = V->p_rm_Vertices->Map(V->vBase, V->vCount * V->vStride, true);
        if (!allIndices || !vbData) {
            if (allIndices) V->p_rm_Indices->Unmap();
            if (vbData) V->p_rm_Vertices->Unmap();
            continue;
        }

        u16* indices = allIndices + indexBase;
        UVBoneResult childUV;
        bool childUVValid = false;

        switch (skelChild->RenderMode) {
        case CSkeletonX::RM_SINGLE:
        case CSkeletonX::RM_SKINNING_1B:
            TryPickChild<vertHW_1W<s16>>(vbData, obj, S, D, indices, indexCount, bestDist, bestHit, childUV, childUVValid);
            break;
        case CSkeletonX::RM_SKINNING_2B:
            TryPickChild<vertHW_2W<s16>>(vbData, obj, S, D, indices, indexCount, bestDist, bestHit, childUV, childUVValid);
            break;
        case CSkeletonX::RM_SKINNING_3B:
            TryPickChild<vertHW_3W<s16>>(vbData, obj, S, D, indices, indexCount, bestDist, bestHit, childUV, childUVValid);
            break;
        case CSkeletonX::RM_SKINNING_4B:
            TryPickChild<vertHW_4W<s16>>(vbData, obj, S, D, indices, indexCount, bestDist, bestHit, childUV, childUVValid);
            break;
        case CSkeletonX::RM_SINGLE_HQ:
        case CSkeletonX::RM_SKINNING_1B_HQ:
            TryPickChild<vertHW_1W<float>>(vbData, obj, S, D, indices, indexCount, bestDist, bestHit, childUV, childUVValid);
            break;
        case CSkeletonX::RM_SKINNING_2B_HQ:
            TryPickChild<vertHW_2W<float>>(vbData, obj, S, D, indices, indexCount, bestDist, bestHit, childUV, childUVValid);
            break;
        case CSkeletonX::RM_SKINNING_3B_HQ:
            TryPickChild<vertHW_3W<float>>(vbData, obj, S, D, indices, indexCount, bestDist, bestHit, childUV, childUVValid);
            break;
        case CSkeletonX::RM_SKINNING_4B_HQ:
            TryPickChild<vertHW_4W<float>>(vbData, obj, S, D, indices, indexCount, bestDist, bestHit, childUV, childUVValid);
            break;
        }

        V->p_rm_Vertices->Unmap();
        V->p_rm_Indices->Unmap();

        if (childUVValid) {
            bestUV = childUV;
            hasHit = true;
            bestTextureName = V->textureName.c_str();
        }
    }

    if (!hasHit)
        return false;

    Fvector localHitPos;
    localHitPos.mad(S, D, bestHit.dist);

    Fvector localNormal;
    localNormal.mknormal(bestHit.skinnedPos[0], bestHit.skinnedPos[1], bestHit.skinnedPos[2]);

    parentXForm.transform_tiny(result.worldPos, localHitPos);
    parentXForm.transform_dir(result.worldNormal, localNormal);
    result.worldNormal.normalize();

    result.hitTextureName = bestTextureName;
    result.dist = bestDist;
    result.uv = bestUV.uv;
    result.boneID = bestUV.boneID;
    result.parent = obj;
    result.baryU = bestHit.baryU;
    result.baryV = bestHit.baryV;

    for (u32 k = 0; k < 3; k++) {
        result.triVerts[k] = bestUV.triVerts[k];
        result.triUV[k] = bestUV.triUV[k];
    }

    Fvector edge1, edge2;
    edge1.sub(bestHit.skinnedPos[1], bestHit.skinnedPos[0]);
    edge2.sub(bestHit.skinnedPos[2], bestHit.skinnedPos[0]);
    parentXForm.transform_dir(result.triWorldEdge1, edge1);
    parentXForm.transform_dir(result.triWorldEdge2, edge2);

    return true;
}

} // namespace xray::render::RENDER_NAMESPACE::decals
