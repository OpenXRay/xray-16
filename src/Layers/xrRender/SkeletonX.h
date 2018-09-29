// SkeletonX.h: interface for the CSkeletonX class.
//
//////////////////////////////////////////////////////////////////////

#ifndef SkeletonXH
#define SkeletonXH
#pragma once

#include "SkeletonCustom.h"
#include "SkeletonXVertRender.h"
#include "xrCDB/Intersect.hpp"

// refs
class CKinematics;
class Fvisual;

//.#pragma pack(push,4)

struct SEnumVerticesCallback;
class CSkeletonX
{
protected:
    enum
    {
        vertRenderFVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1
    };
    enum
    {
        RM_SKINNING_SOFT,
        RM_SINGLE,
        RM_SKINNING_1B,
        RM_SKINNING_2B,
        RM_SKINNING_3B,
        RM_SKINNING_4B
    };

    CKinematics* Parent; // setted up by parent
    ref_smem<vertBoned1W> Vertices1W; // shared
    ref_smem<vertBoned2W> Vertices2W; // shared
    ref_smem<vertBoned3W> Vertices3W; // shared
    ref_smem<vertBoned4W> Vertices4W; // shared
    ref_smem<u16> BonesUsed; // actual bones which have influence on vertices

    u16 RenderMode;
    u16 ChildIDX;

    // render-mode specifics
    union
    {
        struct
        { // soft-skinning only
            u32 cache_DiscardID;
            u32 cache_vCount;
            u32 cache_vOffset;
        };
        u32 RMS_boneid; // single-bone-rendering
        u32 RMS_bonecount; // skinning, maximal bone ID
    };

    void _Copy(CSkeletonX* V);
    void _Render_soft(ref_geom& hGeom, u32 vCount, u32 iOffset, u32 pCount);
    void _Render(ref_geom& hGeom, u32 vCount, u32 iOffset, u32 pCount);
    void _Load(const char* N, IReader* data, u32& dwVertCount);

    virtual void _Load_hw(Fvisual& V, void* data) = 0;
    virtual void _CollectBoneFaces(Fvisual* V, u32 iBase, u32 iCount) = 0;

    void _FillVerticesSoft1W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
                             u16* indices, CBoneData::FacesVec& faces);

    void _FillVerticesSoft2W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
                             u16* indices, CBoneData::FacesVec& faces);

    void _FillVerticesSoft3W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
                             u16* indices, CBoneData::FacesVec& faces);

    void _FillVerticesSoft4W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
                             u16* indices, CBoneData::FacesVec& faces);

    virtual void _FillVerticesHW1W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
        Fvisual* V, u16* indices, CBoneData::FacesVec& faces) = 0;
    virtual void _FillVerticesHW2W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
        Fvisual* V, u16* indices, CBoneData::FacesVec& faces) = 0;
    virtual void _FillVerticesHW3W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
        Fvisual* V, u16* indices, CBoneData::FacesVec& faces) = 0;
    virtual void _FillVerticesHW4W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
        Fvisual* V, u16* indices, CBoneData::FacesVec& faces) = 0;
    virtual void _FillVertices(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
        Fvisual* V, u16 bone_id, u32 iBase, u32 iCount) = 0;

    BOOL _PickBoneSoft1W(IKinematics::pick_result& r, float range, const Fvector& S, const Fvector& D, u16* indices,
        CBoneData::FacesVec& faces);
    BOOL _PickBoneSoft2W(IKinematics::pick_result& r, float range, const Fvector& S, const Fvector& D, u16* indices,
        CBoneData::FacesVec& faces);
    BOOL _PickBoneSoft3W(IKinematics::pick_result& r, float range, const Fvector& S, const Fvector& D, u16* indices,
        CBoneData::FacesVec& faces);
    BOOL _PickBoneSoft4W(IKinematics::pick_result& r, float range, const Fvector& S, const Fvector& D, u16* indices,
        CBoneData::FacesVec& faces);

    virtual BOOL _PickBoneHW1W(IKinematics::pick_result& r, float range, const Fvector& S, const Fvector& D, Fvisual* V,
        u16* indices, CBoneData::FacesVec& faces) = 0;
    virtual BOOL _PickBoneHW2W(IKinematics::pick_result& r, float range, const Fvector& S, const Fvector& D, Fvisual* V,
        u16* indices, CBoneData::FacesVec& faces) = 0;
    virtual BOOL _PickBoneHW3W(IKinematics::pick_result& r, float range, const Fvector& S, const Fvector& D, Fvisual* V,
        u16* indices, CBoneData::FacesVec& faces) = 0;
    virtual BOOL _PickBoneHW4W(IKinematics::pick_result& r, float range, const Fvector& S, const Fvector& D, Fvisual* V,
        u16* indices, CBoneData::FacesVec& faces) = 0;

    virtual BOOL _PickBone(IKinematics::pick_result& r, float range, const Fvector& S, const Fvector& D, Fvisual* V,
        u16 bone_id, u32 iBase, u32 iCount) = 0;

public:
    BOOL has_visible_bones();
    CSkeletonX()
    {
        Parent = nullptr;
        ChildIDX = u16(-1);
    }

    virtual void SetParent(CKinematics* K) { Parent = K; }
    virtual void AfterLoad(CKinematics* parent, u16 child_idx) = 0;
    virtual void EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id) = 0;
    virtual BOOL PickBone(
        IKinematics::pick_result& r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id) = 0;
    virtual void FillVertices(
        const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, u16 bone_id) = 0;

#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
protected:
    void _DuplicateIndices(const char* N, IReader* data);

    //	Index buffer replica since we can't read from index buffer in DX10
    ref_smem<u16> m_Indices;
#endif //	USE_DX10
};

template <typename T_vertex, typename T_buffer>
BOOL pick_bone(T_buffer vertices, CKinematics* Parent, IKinematics::pick_result& r, float dist, const Fvector& S,
    const Fvector& D, u16* indices, CBoneData::FacesVec& faces)
{
    for (auto it = faces.begin(); it != faces.end(); it++)
    {
        u32 idx = (*it) * 3;
        for (u32 k = 0; k < 3; k++)
        {
            T_vertex& vert = vertices[indices[idx + k]];
            get_pos_bones(vert, r.tri[k], Parent);
        }
        float u, v;
        r.dist = flt_max;
        if (CDB::TestRayTri(S, D, r.tri, u, v, r.dist, true) && (r.dist < dist))
        {
            r.normal.mknormal(r.tri[0], r.tri[1], r.tri[2]);
            return TRUE;
        };
    }
    return FALSE;
}

#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
template <typename T>
BOOL pick_bone(CKinematics* Parent, IKinematics::pick_result& r, float dist, const Fvector& S, const Fvector& D,
    Fvisual* V, u16* indices, CBoneData::FacesVec& faces)
{
    VERIFY(!"Not implemented");
    return FALSE;
}
#else //	USE_DX10
template <typename T>
BOOL pick_bone(CKinematics* Parent, IKinematics::pick_result& r, float dist, const Fvector& S, const Fvector& D,
    Fvisual* V, u16* indices, CBoneData::FacesVec& faces)
{
    T* vertices;
    CHK_DX(V->p_rm_Vertices->Lock(V->vBase, V->vCount, (void**)&vertices, D3DLOCK_READONLY));
    bool intersect = !!pick_bone<T, T*>(vertices, Parent, r, dist, S, D, indices, faces);
    CHK_DX(V->p_rm_Vertices->Unlock());
    return intersect;
}
#endif //	USE_DX10

#endif // SkeletonXH
