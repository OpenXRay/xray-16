#include "stdafx.h"

#include "xrCore/FMesh.hpp"
#include "FSkinned.h"
#include "FSkinnedTypes.h"
#include "SkeletonX.h"
#include "Layers/xrRender/BufferUtils.h"
#include "xrEngine/EnnumerateVertices.h"
#include "xrCore/xrDebug_macros.h"

// XXX: test the parallel code in the load_hw()
//#define PARALLEL_BONE_VERTICES_PROCESSING

#ifdef PARALLEL_BONE_VERTICES_PROCESSING
#include <xrCore/Threading/ParallelFor.hpp>
#endif

#ifdef DEBUG
#include "xrCore/dump_string.h"
#endif

static shared_str sbones_array;

//////////////////////////////////////////////////////////////////////
// Body Part
//////////////////////////////////////////////////////////////////////
void CSkeletonX_PM::Copy(dxRender_Visual* V)
{
    inherited1::Copy(V);
    CSkeletonX_PM* X = (CSkeletonX_PM*)(V);
    _Copy((CSkeletonX*)X);
}
void CSkeletonX_ST::Copy(dxRender_Visual* P)
{
    inherited1::Copy(P);
    CSkeletonX_ST* X = (CSkeletonX_ST*)P;
    _Copy((CSkeletonX*)X);
}
//////////////////////////////////////////////////////////////////////
void CSkeletonX_PM::Render(float LOD)
{
    int lod_id = inherited1::last_lod;
    if (LOD >= 0.f)
    {
        clamp(LOD, 0.f, 1.f);
        lod_id = iFloor((1.f - LOD) * float(nSWI.count - 1) + 0.5f);
        inherited1::last_lod = lod_id;
    }
    VERIFY(lod_id >= 0 && lod_id < int(nSWI.count));
    FSlideWindow& SW = nSWI.sw[lod_id];
    _Render(rm_geom, SW.num_verts, SW.offset, SW.num_tris);
}
void CSkeletonX_ST::Render(float /*LOD*/) { _Render(rm_geom, vCount, 0, dwPrimitives); }
//////////////////////////////////////////////////////////////////////
void CSkeletonX_PM::Release() { inherited1::Release(); }
void CSkeletonX_ST::Release() { inherited1::Release(); }
//////////////////////////////////////////////////////////////////////
void CSkeletonX_PM::Load(const char* N, IReader* data, u32 dwFlags)
{
    _Load(N, data, vCount);
    void* _verts_ = data->pointer();
    inherited1::Load(N, data, dwFlags | VLOAD_NOVERTICES);
    GEnv.Render->shader_option_skinning(-1);
    vBase = 0;
    _Load_hw(*this, _verts_);
}
void CSkeletonX_ST::Load(const char* N, IReader* data, u32 dwFlags)
{
    _Load(N, data, vCount);
    void* _verts_ = data->pointer();
    inherited1::Load(N, data, dwFlags | VLOAD_NOVERTICES);
    GEnv.Render->shader_option_skinning(-1);
    vBase = 0;
    _Load_hw(*this, _verts_);
}

template <typename T>
ICF void set_vertice_hw(vertHW_1W<T>* dst, const vertBoned1W* src)
{
    Fvector2 uv;
    uv.set(src->u, src->v);
    dst->set(src->P, src->N, src->T, src->B, uv, src->matrix * 3);
}

template <typename T>
ICF void set_vertice_hw(vertHW_2W<T>* dst, const vertBoned2W* src)
{
    Fvector2 uv;
    uv.set(src->u, src->v);
    dst->set(src->P, src->N, src->T, src->B, uv, int(src->matrix0) * 3, int(src->matrix1) * 3, src->w);
}

template <typename T>
ICF void set_vertice_hw(vertHW_3W<T>* dst, const vertBoned3W* src)
{
    Fvector2 uv;
    uv.set(src->u, src->v);
    dst->set(src->P, src->N, src->T, src->B, uv, int(src->m[0]) * 3,
        int(src->m[1]) * 3, int(src->m[2]) * 3,
        src->w[0], src->w[1]);
}

template <typename T>
ICF void set_vertice_hw(vertHW_4W<T>* dst, const vertBoned4W* src)
{
    Fvector2 uv;
    uv.set(src->u, src->v);
    dst->set(src->P, src->N, src->T, src->B, uv, int(src->m[0]) * 3,
        int(src->m[1]) * 3, int(src->m[2]) * 3,
        int(src->m[3]) * 3, src->w[0], src->w[1], src->w[2]);
}

// If vertex count is bigger than this value,
// then it's reasonable to process vertices in parallel
// XXX: determine value dynamically, it should be different for different CPUs
constexpr auto PARALLEL_BONE_VERTICES_PROCESSING_MIN_VERTICES = 10000; // XXX: rough value

template <typename TDst, typename TSrc>
void load_hw(Fvisual& V, const TSrc* src)
{
    V.vStride = GetDeclVertexSize(get_decl<TDst>(), 0);
    VERIFY(nullptr == V.p_rm_Vertices);

    V.p_rm_Vertices = xr_new<VertexStagingBuffer>();
    V.p_rm_Vertices->Create(V.vCount * V.vStride, true); // VB may be read by wallmarks code

    TDst* dst = static_cast<TDst*>(V.p_rm_Vertices->Map());

    // XXX: install some Ultra HD models pack and test the parallel code
    // For the original game models parallel code is always slower...
    // But I don't want to enable untested code.
#ifdef PARALLEL_BONE_VERTICES_PROCESSING
    if (V.vCount > PARALLEL_BONE_VERTICES_PROCESSING_MIN_VERTICES)
    {
        using range_value_type = decltype(Fvisual::vCount);
        auto setVertices = [dst, src](TaskRange<range_value_type> range)
        {
            const TSrc* src2 = &src[range.begin()];
            TDst* dst2 = &dst[range.begin()];
            TDst* dst2End = &dst[range.end()];
            while (dst2 != dst2End)
            {
                set_vertice_hw(dst2, src2);
                dst2++;
                src2++;
            }
        };
        TaskRange<range_value_type> range(0, V.vCount);
        xr_parallel_for(range, setVertices);
    }
    else
#endif
    {
        for (u32 it = 0; it < V.vCount; it++)
        {
            set_vertice_hw(dst, src);
            dst++;
            src++;
        }
    }
    V.p_rm_Vertices->Unmap(true); // upload vertex data
    V.rm_geom.create(get_decl<TDst>(), *V.p_rm_Vertices, *V.p_rm_Indices);
}

void CSkeletonX_ext::_Load_hw(Fvisual& V, void* _verts_)
{
    switch (RenderMode)
    {
    case RM_SKINNING_SOFT:
        //Msg("skinning: software");
        V.rm_geom.create(vertRenderFVF, RCache.Vertex.Buffer(), *V.p_rm_Indices);
        break;

    case RM_SINGLE:
    case RM_SKINNING_1B:
        load_hw<vertHW_1W<s16>>(V, (vertBoned1W*)_verts_);
        break;

    case RM_SKINNING_2B:
        load_hw<vertHW_2W<s16>>(V, (vertBoned2W*)_verts_);
        break;

    case RM_SKINNING_3B:
        load_hw<vertHW_3W<s16>>(V, (vertBoned3W*)_verts_);
        break;

    case RM_SKINNING_4B:
        load_hw<vertHW_4W<s16>>(V, (vertBoned4W*)_verts_);
        break;

    case RM_SINGLE_HQ:
    case RM_SKINNING_1B_HQ:
        load_hw<vertHW_1W<float>>(V, (vertBoned1W*)_verts_);
        break;

    case RM_SKINNING_2B_HQ:
        load_hw<vertHW_2W<float>>(V, (vertBoned2W*)_verts_);
        break;

    case RM_SKINNING_3B_HQ:
        load_hw<vertHW_3W<float>>(V, (vertBoned3W*)_verts_);
        break;

    case RM_SKINNING_4B_HQ:
        load_hw<vertHW_4W<float>>(V, (vertBoned4W*)_verts_);
        break;
    }
}

//-----------------------------------------------------------------------------------------------------
// Wallmarks
//-----------------------------------------------------------------------------------------------------
#include "xrCDB/Intersect.hpp"

#ifdef DEBUG

template <typename vertex_type>
static void verify_vertex(const vertex_type& v, const Fvisual* V, const CKinematics* Parent, u32 iBase, u32 iCount,
    const u16* indices, u32 vertex_idx, u32 idx)
{
    VERIFY(Parent);
#ifndef _EDITOR
    for (u8 i = 0; i < vertex_type::bones_count; ++i)
        if (v.get_bone_id(i) >= Parent->LL_BoneCount())
        {
            Msg("v.get_bone_id(i): %d, Parent->LL_BoneCount() %d ", v.get_bone_id(i), Parent->LL_BoneCount());
            Msg("&v: %p, &V: %p, indices: %p", &v, V, indices);
            Msg(" iBase: %d, iCount: %d, V->iBase %d, V->iCount %d, "
                "V->vBase: %d, V->vCount %d, vertex_idx: %d, idx: %d",
                iBase, iCount, V->iBase, V->iCount, V->vBase, V->vCount, vertex_idx, idx);
            Msg(" v.P: %s , v.N: %s, v.T: %s, v.B: %s", get_string(v.P).c_str(), get_string(v.N).c_str(),
                get_string(v.T).c_str(), get_string(v.B).c_str());
            Msg("Parent->dbg_name: %s ", Parent->dbg_name.c_str());
            FlushLog();
            FATAL("v.get_bone_id(i) >= Parent->LL_BoneCount()");
        }
#endif
}
#endif

template <typename T>
ICF void append_bone_faces(CKinematics* Parent, vertHW_1W<T>& v, u16 ChildIDX, u32 idx)
{
    CBoneData& BD = Parent->LL_GetData(v.get_bone());
    BD.AppendFace(ChildIDX, (u16)(idx / 3));
}

template <typename T>
ICF void append_bone_faces(CKinematics* Parent, vertHW_2W<T>& v, u16 ChildIDX, u32 idx)
{
    CBoneData& BD0 = Parent->LL_GetData(v.get_bone(0));
    BD0.AppendFace(ChildIDX, (u16)(idx / 3));
    CBoneData& BD1 = Parent->LL_GetData(v.get_bone(1));
    BD1.AppendFace(ChildIDX, (u16)(idx / 3));
}

template <typename T>
ICF void append_bone_faces(CKinematics* Parent, vertHW_3W<T>& v, u16 ChildIDX, u32 idx)
{
    CBoneData& BD0 = Parent->LL_GetData(v.get_bone(0));
    BD0.AppendFace(ChildIDX, (u16)(idx / 3));
    CBoneData& BD1 = Parent->LL_GetData(v.get_bone(1));
    BD1.AppendFace(ChildIDX, (u16)(idx / 3));
    CBoneData& BD2 = Parent->LL_GetData(v.get_bone(2));
    BD2.AppendFace(ChildIDX, (u16)(idx / 3));
}

template <typename T>
ICF void append_bone_faces(CKinematics* Parent, vertHW_4W<T>& v, u16 ChildIDX, u32 idx)
{
    CBoneData& BD0 = Parent->LL_GetData(v.get_bone(0));
    BD0.AppendFace(ChildIDX, (u16)(idx / 3));
    CBoneData& BD1 = Parent->LL_GetData(v.get_bone(1));
    BD1.AppendFace(ChildIDX, (u16)(idx / 3));
    CBoneData& BD2 = Parent->LL_GetData(v.get_bone(2));
    BD2.AppendFace(ChildIDX, (u16)(idx / 3));
    CBoneData& BD3 = Parent->LL_GetData(v.get_bone(3));
    BD3.AppendFace(ChildIDX, (u16)(idx / 3));
}

template <typename T>
void CSkeletonX_ext::_CollectBoneFacesHW(u16* indices, Fvisual* V, u32 iCount)
{
    VERIFY(V->vStride == sizeof(T));
    T* vertices = static_cast<T*>(V->p_rm_Vertices->Map(V->vBase, V->vCount * V->vStride, true));

    for (u32 idx = 0; idx < iCount; idx++)
    {
        T& v = vertices[indices[idx]];
        append_bone_faces(Parent, v, ChildIDX, idx);
    }

    V->p_rm_Vertices->Unmap(); // ? keep ?
}

void CSkeletonX_ext::_CollectBoneFaces(Fvisual* V, u32 iBase, u32 iCount)
{
    u16* indices = static_cast<u16*>(V->p_rm_Indices->Map(0, V->dwPrimitives * 3, true));
    indices += iBase;

    switch (RenderMode)
    {
    case RM_SKINNING_SOFT:
    {
        if (*Vertices1W)
        {
            vertBoned1W* vertices = *Vertices1W;
            for (u32 idx = 0; idx < iCount; idx++)
            {
                vertBoned1W& v = vertices[V->vBase + indices[idx]];
#ifdef DEBUG
                verify_vertex(v, V, Parent, iBase, iCount, indices, V->vBase + indices[idx], idx);
#endif
                CBoneData& BD = Parent->LL_GetData((u16)v.matrix);
                BD.AppendFace(ChildIDX, (u16)(idx / 3));
            }
        }
        else if (*Vertices2W)
        {
            vertBoned2W* vertices = *Vertices2W;
            for (u32 idx = 0; idx < iCount; ++idx)
            {
                vertBoned2W& v = vertices[V->vBase + indices[idx]];
#ifdef DEBUG
                verify_vertex(v, V, Parent, iBase, iCount, indices, V->vBase + indices[idx], idx);
#endif
                CBoneData& BD0 = Parent->LL_GetData((u16)v.matrix0);
                BD0.AppendFace(ChildIDX, (u16)(idx / 3));
                CBoneData& BD1 = Parent->LL_GetData((u16)v.matrix1);
                BD1.AppendFace(ChildIDX, (u16)(idx / 3));
            }
        }
        else if (*Vertices3W)
        {
            vertBoned3W* vertices = *Vertices3W;
            for (u32 idx = 0; idx < iCount; ++idx)
            {
                vertBoned3W& v = vertices[V->vBase + indices[idx]];
#ifdef DEBUG
                verify_vertex(v, V, Parent, iBase, iCount, indices, V->vBase + indices[idx], idx);
#endif
                CBoneData& BD0 = Parent->LL_GetData((u16)v.m[0]);
                BD0.AppendFace(ChildIDX, (u16)(idx / 3));
                CBoneData& BD1 = Parent->LL_GetData((u16)v.m[1]);
                BD1.AppendFace(ChildIDX, (u16)(idx / 3));
                CBoneData& BD2 = Parent->LL_GetData((u16)v.m[2]);
                BD2.AppendFace(ChildIDX, (u16)(idx / 3));
            }
        }
        else if (*Vertices4W)
        {
            vertBoned4W* vertices = *Vertices4W;
            for (u32 idx = 0; idx < iCount; ++idx)
            {
                vertBoned4W& v = vertices[V->vBase + indices[idx]];
#ifdef DEBUG
                verify_vertex(v, V, Parent, iBase, iCount, indices, V->vBase + indices[idx], idx);
#endif
                CBoneData& BD0 = Parent->LL_GetData((u16)v.m[0]);
                BD0.AppendFace(ChildIDX, (u16)(idx / 3));
                CBoneData& BD1 = Parent->LL_GetData((u16)v.m[1]);
                BD1.AppendFace(ChildIDX, (u16)(idx / 3));
                CBoneData& BD2 = Parent->LL_GetData((u16)v.m[2]);
                BD2.AppendFace(ChildIDX, (u16)(idx / 3));
                CBoneData& BD3 = Parent->LL_GetData((u16)v.m[3]);
                BD3.AppendFace(ChildIDX, (u16)(idx / 3));
            }
        }
        else
            R_ASSERT2(0, "not implemented yet");
        break;
    }

    case RM_SINGLE:
    case RM_SKINNING_1B:
        _CollectBoneFacesHW<vertHW_1W<s16>>(indices, V, iCount);
        break;

    case RM_SKINNING_2B:
        _CollectBoneFacesHW<vertHW_2W<s16>>(indices, V, iCount);
        break;

    case RM_SKINNING_3B:
        _CollectBoneFacesHW<vertHW_3W<s16>>(indices, V, iCount);
        break;

    case RM_SKINNING_4B:
        _CollectBoneFacesHW<vertHW_4W<s16>>(indices, V, iCount);
        break;

    case RM_SINGLE_HQ:
    case RM_SKINNING_1B_HQ:
        _CollectBoneFacesHW<vertHW_1W<float>>(indices, V, iCount);
        break;

    case RM_SKINNING_2B_HQ:
        _CollectBoneFacesHW<vertHW_2W<float>>(indices, V, iCount);
        break;

    case RM_SKINNING_3B_HQ:
        _CollectBoneFacesHW<vertHW_3W<float>>(indices, V, iCount);
        break;

    case RM_SKINNING_4B_HQ:
        _CollectBoneFacesHW<vertHW_4W<float>>(indices, V, iCount);
        break;
    }
    V->p_rm_Indices->Unmap();
}

void CSkeletonX_ST::AfterLoad(CKinematics* parent, u16 child_idx)
{
    inherited2::AfterLoad(parent, child_idx);
    inherited2::_CollectBoneFaces(this, iBase, iCount);
}

void CSkeletonX_PM::AfterLoad(CKinematics* parent, u16 child_idx)
{
    inherited2::AfterLoad(parent, child_idx);
    FSlideWindow& SW = nSWI.sw[0]; // max LOD
    inherited2::_CollectBoneFaces(this, iBase + SW.offset, SW.num_tris * 3);
}

template <typename T>
void get_pos_bones(const T& v, Fvector& p, CKinematics* Parent)
{
    v.get_pos_bones(p, Parent);
}

template <typename T>
BOOL pick_bone(CKinematics* Parent, IKinematics::pick_result& r, float dist, const Fvector& S, const Fvector& D,
    Fvisual* V, u16* indices, CBoneData::FacesVec& faces)
{
    void* data = static_cast<u8*>(V->p_rm_Vertices->Map(V->vBase, V->vCount * V->vStride, true)); // read-back
    T* vertices = static_cast<T*>(data);
    const bool intersect = pick_bone<T, T*>(vertices, Parent, r, dist, S, D, indices, faces);
    V->p_rm_Vertices->Unmap();
    return intersect;
}

BOOL CSkeletonX_ext::_PickBone(IKinematics::pick_result& r, float dist, const Fvector& start, const Fvector& dir,
    Fvisual* V, u16 bone_id, u32 iBase, u32 /*iCount*/)
{
    VERIFY(Parent && ChildIDX != u16(-1));
    CBoneData& BD = Parent->LL_GetData(bone_id);
    CBoneData::FacesVec& faces = BD.child_faces[ChildIDX];
    BOOL result = FALSE;
    u16* indices = static_cast<u16*>(V->p_rm_Indices->Map(0, V->dwPrimitives * 3, true));
    switch (RenderMode)
    {
    case RM_SKINNING_SOFT:
    {
        if (*Vertices1W)
            result = _PickBoneSoft1W(r, dist, start, dir, indices + iBase, faces);
        else if (*Vertices2W)
            result = _PickBoneSoft2W(r, dist, start, dir, indices + iBase, faces);
        else if (*Vertices3W)
            result = _PickBoneSoft3W(r, dist, start, dir, indices + iBase, faces);
        else
        {
            VERIFY(!!*Vertices4W);
            result = _PickBoneSoft4W(r, dist, start, dir, indices + iBase, faces);
        }
        break;
    }

    case RM_SINGLE:
    case RM_SKINNING_1B:
        result = pick_bone<vertHW_1W<s16>>(Parent, r, dist, start, dir, V, indices, faces);
        break;

    case RM_SKINNING_2B:
        result = pick_bone<vertHW_2W<s16>>(Parent, r, dist, start, dir, V, indices, faces);
        break;
    case RM_SKINNING_3B:
        result = pick_bone<vertHW_3W<s16>>(Parent, r, dist, start, dir, V, indices, faces);
        break;

    case RM_SKINNING_4B:
        result = pick_bone<vertHW_4W<s16>>(Parent, r, dist, start, dir, V, indices, faces);
        break;

    case RM_SINGLE_HQ:
    case RM_SKINNING_1B_HQ:
        result = pick_bone<vertHW_1W<float>>(Parent, r, dist, start, dir, V, indices, faces);
        break;

    case RM_SKINNING_2B_HQ:
        result = pick_bone<vertHW_2W<float>>(Parent, r, dist, start, dir, V, indices, faces);
        break;
    case RM_SKINNING_3B_HQ:
        result = pick_bone<vertHW_3W<float>>(Parent, r, dist, start, dir, V, indices, faces);
        break;

    case RM_SKINNING_4B_HQ:
        result = pick_bone<vertHW_4W<float>>(Parent, r, dist, start, dir, V, indices, faces);
        break;

    default: NODEFAULT;
    }
    V->p_rm_Indices->Unmap();

    return result;
}

BOOL CSkeletonX_ST::PickBone(
    IKinematics::pick_result& r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id)
{
    return inherited2::_PickBone(r, dist, start, dir, this, bone_id, iBase, iCount);
}
BOOL CSkeletonX_PM::PickBone(
    IKinematics::pick_result& r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id)
{
    FSlideWindow& SW = nSWI.sw[0];
    return inherited2::_PickBone(r, dist, start, dir, this, bone_id, iBase + SW.offset, SW.num_tris * 3);
}

void CSkeletonX_ST::EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id)
{
    inherited2::_EnumBoneVertices(C, this, bone_id, iBase, iCount);
}

void CSkeletonX_PM::EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id)
{
    FSlideWindow& SW = nSWI.sw[0];
    inherited2::_EnumBoneVertices(C, this, bone_id, iBase + SW.offset, SW.num_tris * 3);
}

template <typename T>
ICF void fill_face(CKinematics* parent, Fvector p[3], CSkeletonWallmark::WMFace& F, u32 k, vertHW_1W<T>& vert)
{
    F.bone_id[k][0] = vert.get_bone();
    F.bone_id[k][1] = F.bone_id[k][0];
    F.bone_id[k][2] = F.bone_id[k][0];
    F.bone_id[k][3] = F.bone_id[k][0];
    F.weight[k][0] = 0.f;
    F.weight[k][1] = 0.f;
    F.weight[k][2] = 0.f;

    const Fmatrix& xform = parent->LL_GetBoneInstance(F.bone_id[k][0]).mRenderTransform;
    vert.get_pos(F.vert[k]);
    xform.transform_tiny(p[k], F.vert[k]);
}

template <typename T>
ICF void fill_face(CKinematics* parent, Fvector p[3], CSkeletonWallmark::WMFace& F, u32 k, vertHW_2W<T>& vert)
{
    F.bone_id[k][0] = vert.get_bone(0);
    F.bone_id[k][1] = vert.get_bone(1);
    F.bone_id[k][2] = F.bone_id[k][1];
    F.bone_id[k][3] = F.bone_id[k][1];
    F.weight[k][0] = vert.get_weight();
    F.weight[k][1] = 0.f;
    F.weight[k][2] = 0.f;

    Fmatrix& xform0 = parent->LL_GetBoneInstance(F.bone_id[k][0]).mRenderTransform;
    Fmatrix& xform1 = parent->LL_GetBoneInstance(F.bone_id[k][1]).mRenderTransform;
    vert.get_pos(F.vert[k]);

    Fvector P0, P1;
    xform0.transform_tiny(P0, F.vert[k]);
    xform1.transform_tiny(P1, F.vert[k]);
    p[k].lerp(P0, P1, F.weight[k][0]);
}

template <typename T>
ICF void fill_face(CKinematics* parent, Fvector p[3], CSkeletonWallmark::WMFace& F, u32 k, vertHW_3W<T>& vert)
{
    F.bone_id[k][0] = vert.get_bone(0);
    F.bone_id[k][1] = vert.get_bone(1);
    F.bone_id[k][2] = vert.get_bone(2);
    F.bone_id[k][3] = F.bone_id[k][2];
    F.weight[k][0] = vert.get_weight0();
    F.weight[k][1] = vert.get_weight1();
    F.weight[k][2] = 0.f;
    vert.get_pos(F.vert[k]);
    vert.get_pos_bones(p[k], parent);
}

template <typename T>
ICF void fill_face(CKinematics* parent, Fvector p[3], CSkeletonWallmark::WMFace& F, u32 k, vertHW_4W<T>& vert)
{
    F.bone_id[k][0] = vert.get_bone(0);
    F.bone_id[k][1] = vert.get_bone(1);
    F.bone_id[k][2] = vert.get_bone(2);
    F.bone_id[k][3] = vert.get_bone(3);
    F.weight[k][0] = vert.get_weight0();
    F.weight[k][1] = vert.get_weight1();
    F.weight[k][2] = vert.get_weight2();
    vert.get_pos(F.vert[k]);
    vert.get_pos_bones(p[k], parent);
}

template <typename T>
void fill_vertices_hw(CKinematics* parent, const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal,
    float size, Fvisual* V, u16* indices, CBoneData::FacesVec& faces)
{
    VERIFY(V->vStride == sizeof(T));
    T* vertices = static_cast<T*>(V->p_rm_Vertices->Map(V->vBase, V->vCount * V->vStride, true));

    for (auto it = faces.begin(); it != faces.end(); ++it)
    {
        Fvector p[3];
        u32 idx = *it * 3;
        CSkeletonWallmark::WMFace F;

        for (u32 k = 0; k < 3; k++)
        {
            T& vert = vertices[indices[idx + k]];
            fill_face(parent, p, F, k, vert);
        }

        Fvector test_normal;
        test_normal.mknormal(p[0], p[1], p[2]);
        const float cosa = test_normal.dotproduct(normal);
        if (cosa < EPS)
            continue;

        if (CDB::TestSphereTri(wm.ContactPoint(), size, p))
        {
            Fvector UV;
            for (u32 k = 0; k < 3; k++)
            {
                Fvector2& uv = F.uv[k];
                view.transform_tiny(UV, p[k]);
                uv.x = (1 + UV.x) * .5f;
                uv.y = (1 - UV.y) * .5f;
            }
            wm.m_Faces.push_back(F);
        }
    }
    V->p_rm_Vertices->Unmap();
}

void CSkeletonX_ext::_FillVertices(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal,
                                   float size, Fvisual* V, u16 bone_id, u32 iBase, u32 /*iCount*/)
{
    VERIFY(Parent && ChildIDX != u16(-1));
    CBoneData& BD = Parent->LL_GetData(bone_id);
    CBoneData::FacesVec* faces = &BD.child_faces[ChildIDX];
    u16* indices = static_cast<u16*>(V->p_rm_Indices->Map(0, V->dwPrimitives * 3, true));
    // fill vertices
    switch (RenderMode)
    {
    case RM_SKINNING_SOFT:
        if (*Vertices1W) _FillVerticesSoft1W(view, wm, normal, size, indices + iBase, *faces);
        else if (*Vertices2W) _FillVerticesSoft2W(view, wm, normal, size, indices + iBase, *faces);
        else if (*Vertices3W) _FillVerticesSoft3W(view, wm, normal, size, indices + iBase, *faces);
        else
        {
            VERIFY(!!*Vertices4W);
            _FillVerticesSoft4W(view, wm, normal, size, indices + iBase, *faces);
        }
        break;

    case RM_SINGLE:
    case RM_SKINNING_1B:
        fill_vertices_hw<vertHW_1W<s16>>(Parent, view, wm, normal, size, V, indices + iBase, *faces);
        break;

    case RM_SKINNING_2B:
        fill_vertices_hw<vertHW_2W<s16>>(Parent, view, wm, normal, size, V, indices + iBase, *faces);
        break;

    case RM_SKINNING_3B:
        fill_vertices_hw<vertHW_3W<s16>>(Parent, view, wm, normal, size, V, indices + iBase, *faces);
        break;

    case RM_SKINNING_4B:
        fill_vertices_hw<vertHW_4W<s16>>(Parent, view, wm, normal, size, V, indices + iBase, *faces);
        break;

    case RM_SINGLE_HQ:
    case RM_SKINNING_1B_HQ:
        fill_vertices_hw<vertHW_1W<float>>(Parent, view, wm, normal, size, V, indices + iBase, *faces);
        break;

    case RM_SKINNING_2B_HQ:
        fill_vertices_hw<vertHW_2W<float>>(Parent, view, wm, normal, size, V, indices + iBase, *faces);
        break;

    case RM_SKINNING_3B_HQ:
        fill_vertices_hw<vertHW_3W<float>>(Parent, view, wm, normal, size, V, indices + iBase, *faces);
        break;

    case RM_SKINNING_4B_HQ:
        fill_vertices_hw<vertHW_4W<float>>(Parent, view, wm, normal, size, V, indices + iBase, *faces);
        break;
    }
    V->p_rm_Indices->Unmap();
}

void CSkeletonX_ST::FillVertices(
    const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, u16 bone_id)
{
    inherited2::_FillVertices(view, wm, normal, size, this, bone_id, iBase, iCount);
}
void CSkeletonX_PM::FillVertices(
    const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, u16 bone_id)
{
    FSlideWindow& SW = nSWI.sw[0];
    inherited2::_FillVertices(view, wm, normal, size, this, bone_id, iBase + SW.offset, SW.num_tris * 3);
}

template <typename vertex_buffer_type>
void TEnumBoneVertices(
    vertex_buffer_type vertices, u16* indices, CBoneData::FacesVec& faces, SEnumVerticesCallback& C)
{
    for (auto it = faces.begin(); it != faces.end(); ++it)
    {
        u32 idx = (*it) * 3;
        for (u32 k = 0; k < 3; k++)
        {
            Fvector P;
            vertices[indices[idx + k]].get_pos(P);
            C(P);
        }
    }
}

void CSkeletonX_ext::_EnumBoneVertices(SEnumVerticesCallback& C, Fvisual* V, u16 bone_id, u32 iBase, u32 /*iCount*/) const
{
    VERIFY(Parent && ChildIDX != u16(-1));
    CBoneData& BD = Parent->LL_GetData(bone_id);
    CBoneData::FacesVec* faces = &BD.child_faces[ChildIDX];
    u16* indices = static_cast<u16*>(V->p_rm_Indices->Map(0, V->dwPrimitives * 3, true));

    // fill vertices
    switch (RenderMode)
    {
    case RM_SKINNING_SOFT:
    {
        if (*Vertices1W)
            TEnumBoneVertices(Vertices1W, indices + iBase, *faces, C);
        else if (*Vertices2W)
            TEnumBoneVertices(Vertices2W, indices + iBase, *faces, C);
        else if (*Vertices3W)
            TEnumBoneVertices(Vertices3W, indices + iBase, *faces, C);
        else
        {
            VERIFY(!!*Vertices4W);
            TEnumBoneVertices(Vertices4W, indices + iBase, *faces, C);
        }
    }
    break;
    case RM_SINGLE:
    case RM_SKINNING_1B:
    {
        VERIFY(V->vStride == sizeof(vertHW_1W<s16>));
        vertHW_1W<s16>* vertices = static_cast<vertHW_1W<s16>*>(V->p_rm_Vertices->Map(V->vBase, V->vCount * V->vStride, true));
        TEnumBoneVertices(vertices, indices + iBase, *faces, C);
        break;
    }
    case RM_SKINNING_2B:
    {
        VERIFY(V->vStride == sizeof(vertHW_2W<s16>));
        vertHW_2W<s16>* vertices = static_cast<vertHW_2W<s16>*>(V->p_rm_Vertices->Map(V->vBase, V->vCount * V->vStride, true));
        TEnumBoneVertices(vertices, indices + iBase, *faces, C);
        break;
    }
    case RM_SKINNING_3B:
    {
        VERIFY(V->vStride == sizeof(vertHW_3W<s16>));
        vertHW_3W<s16>* vertices = static_cast<vertHW_3W<s16>*>(V->p_rm_Vertices->Map(V->vBase, V->vCount * V->vStride, true));
        TEnumBoneVertices(vertices, indices + iBase, *faces, C);
        break;
    }
    case RM_SKINNING_4B:
    {
        VERIFY(V->vStride == sizeof(vertHW_4W<s16>));
        vertHW_4W<s16>* vertices = static_cast<vertHW_4W<s16>*>(V->p_rm_Vertices->Map(V->vBase, V->vCount * V->vStride, true));
        TEnumBoneVertices(vertices, indices + iBase, *faces, C);
        break;
    }
    case RM_SINGLE_HQ:
    case RM_SKINNING_1B_HQ:
    {
        VERIFY(V->vStride == sizeof(vertHW_1W<float>));
        vertHW_1W<float>* vertices = static_cast<vertHW_1W<float>*>(V->p_rm_Vertices->Map(V->vBase, V->vCount * V->vStride, true));
        TEnumBoneVertices(vertices, indices + iBase, *faces, C);
        break;
    }
    case RM_SKINNING_2B_HQ:
    {
        VERIFY(V->vStride == sizeof(vertHW_2W<float>));
        vertHW_2W<float>* vertices = static_cast<vertHW_2W<float>*>(V->p_rm_Vertices->Map(V->vBase, V->vCount * V->vStride, true));
        TEnumBoneVertices(vertices, indices + iBase, *faces, C);
        break;
    }
    case RM_SKINNING_3B_HQ:
    {
        VERIFY(V->vStride == sizeof(vertHW_3W<float>));
        vertHW_3W<float>* vertices = static_cast<vertHW_3W<float>*>(V->p_rm_Vertices->Map(V->vBase, V->vCount * V->vStride, true));
        TEnumBoneVertices(vertices, indices + iBase, *faces, C);
        break;
    }
    case RM_SKINNING_4B_HQ:
    {
        VERIFY(V->vStride == sizeof(vertHW_4W<float>));
        vertHW_4W<float>* vertices = static_cast<vertHW_4W<float>*>(V->p_rm_Vertices->Map(V->vBase, V->vCount * V->vStride, true));
        TEnumBoneVertices(vertices, indices + iBase, *faces, C);
        break;
    }
    default: NODEFAULT;
    }
    if (RenderMode != RM_SKINNING_SOFT)
        V->p_rm_Vertices->Unmap();
    V->p_rm_Indices->Unmap();
}
