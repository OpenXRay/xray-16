// SkeletonX.cpp: implementation of the CSkeletonX class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#if defined(WINDOWS)
#pragma warning(push)
#pragma warning(disable : 4995)
#include <d3dx9.h>
#pragma warning(pop)
#endif

#ifndef _EDITOR
#include "xrEngine/Render.h"
#else
#include "Include/xrAPI/xrAPI.h"
#endif
#include "SkeletonX.h"
#include "SkeletonCustom.h"
#include "xrCore/FMesh.hpp"
#include "xrCore/Math/MathUtil.hpp"

using namespace XRay::Math;

shared_str s_bones_array_const;

//////////////////////////////////////////////////////////////////////
// Body Part
//////////////////////////////////////////////////////////////////////
void CSkeletonX::AfterLoad(CKinematics* parent, u16 child_idx)
{
    SetParent(parent);
    ChildIDX = child_idx;
}
void CSkeletonX::_Copy(CSkeletonX* B)
{
    Parent = nullptr;
    ChildIDX = B->ChildIDX;
    Vertices1W = B->Vertices1W;
    Vertices2W = B->Vertices2W;
    Vertices3W = B->Vertices3W;
    Vertices4W = B->Vertices4W;
    BonesUsed = B->BonesUsed;

    // caution - overlapped (union)
    cache_DiscardID = B->cache_DiscardID;
    cache_vCount = B->cache_vCount;
    cache_vOffset = B->cache_vOffset;
    RenderMode = B->RenderMode;
    RMS_boneid = B->RMS_boneid;
    RMS_bonecount = B->RMS_bonecount;

#ifndef USE_DX9
    m_Indices = B->m_Indices;
#endif //	USE_DX10
}
//////////////////////////////////////////////////////////////////////
void CSkeletonX::_Render(ref_geom& hGeom, u32 vCount, u32 iOffset, u32 pCount)
{
    RCache.stat.r.s_dynamic.add(vCount);
    switch (RenderMode)
    {
    case RM_SKINNING_SOFT:
        _Render_soft(hGeom, vCount, iOffset, pCount);
        RCache.stat.r.s_dynamic_sw.add(vCount);
        break;
    case RM_SINGLE:
    {
        Fmatrix W;
        W.mul_43(RCache.xforms.m_w, Parent->LL_GetTransform_R(u16(RMS_boneid)));
        RCache.set_xform_world(W);
        RCache.set_Geometry(hGeom);
        RCache.Render(D3DPT_TRIANGLELIST, 0, 0, vCount, iOffset, pCount);
        RCache.stat.r.s_dynamic_inst.add(vCount);
    }
    break;
    case RM_SKINNING_1B:
    case RM_SKINNING_2B:
    case RM_SKINNING_3B:
    case RM_SKINNING_4B:
    {
        // transfer matrices
        ref_constant array = RCache.get_c(s_bones_array_const);
        u32 count = RMS_bonecount;
        for (u32 mid = 0; mid < count; mid++)
        {
            Fmatrix& M = Parent->LL_GetTransform_R(u16(mid));
            u32 id = mid * 3;
            RCache.set_ca(&*array, id + 0, M._11, M._21, M._31, M._41);
            RCache.set_ca(&*array, id + 1, M._12, M._22, M._32, M._42);
            RCache.set_ca(&*array, id + 2, M._13, M._23, M._33, M._43);
        }

        // render
        RCache.set_Geometry(hGeom);
        RCache.Render(D3DPT_TRIANGLELIST, 0, 0, vCount, iOffset, pCount);
        if (RM_SKINNING_1B == RenderMode)
            RCache.stat.r.s_dynamic_1B.add(vCount);
        else if (RM_SKINNING_2B == RenderMode)
            RCache.stat.r.s_dynamic_2B.add(vCount);
        else if (RM_SKINNING_3B == RenderMode)
            RCache.stat.r.s_dynamic_3B.add(vCount);
        else if (RM_SKINNING_4B == RenderMode)
            RCache.stat.r.s_dynamic_4B.add(vCount);
    }
    break;
    }
}
void CSkeletonX::_Render_soft(ref_geom& hGeom, u32 vCount, u32 iOffset, u32 pCount)
{
    u32 vOffset = cache_vOffset;

    _VertexStream& _VS = RCache.Vertex;
    if (cache_DiscardID != _VS.DiscardID() || vCount != cache_vCount)
    {
        vertRender* Dest = (vertRender*)_VS.Lock(vCount, hGeom->vb_stride, vOffset);
        cache_DiscardID = _VS.DiscardID();
        cache_vCount = vCount;
        cache_vOffset = vOffset;

        RImplementation.BasicStats.Skinning.Begin();
        if (*Vertices1W)
        {
            Skin1W(Dest, // dest
                *Vertices1W, // source
                vCount, // count
                Parent->bone_instances // bones
                );
        }
        else if (*Vertices2W)
        {
            Skin2W(Dest, // dest
                *Vertices2W, // source
                vCount, // count
                Parent->bone_instances // bones
                );
        }
        else if (*Vertices3W)
        {
            Skin3W(Dest, // dest
                *Vertices3W, // source
                vCount, // count
                Parent->bone_instances // bones
                );
        }
        else if (*Vertices4W)
        {
            Skin4W(Dest, // dest
                *Vertices4W, // source
                vCount, // count
                Parent->bone_instances // bones
                );
        }
        else
            R_ASSERT2(0, "unsupported soft rendering");

        RImplementation.BasicStats.Skinning.End();
        _VS.Unlock(vCount, hGeom->vb_stride);
    }

    RCache.set_Geometry(hGeom);
    RCache.Render(D3DPT_TRIANGLELIST, vOffset, 0, vCount, iOffset, pCount);
}

void CSkeletonX::_Load(const char* N, IReader* data, u32& dwVertCount)
{
    s_bones_array_const = "sbones_array";
    xr_vector<u16> bids;

    // Load vertices
    R_ASSERT(data->find_chunk(OGF_VERTICES));

    // u16			hw_bones_cnt		= u16((HW.Caps.geometry.dwRegisters-22)/3);
    //	Igor: some shaders in r1 need more free constant registers
    u16 hw_bones_cnt = u16((HW.Caps.geometry.dwRegisters - 22 - 3) / 3);

#if RENDER == R_R1
    if (ps_r1_SoftwareSkinning == 1)
        hw_bones_cnt = 0;
#endif // RENDER == R_R1

    u16 sw_bones_cnt = 0;
#ifdef _EDITOR
    hw_bones_cnt = 0;
#endif

    u32 dwVertType, size, it, crc;
    dwVertType = data->r_u32();
    dwVertCount = data->r_u32();

    RenderMode = RM_SKINNING_SOFT;
    GEnv.Render->shader_option_skinning(-1);

    switch (dwVertType)
    {
    case OGF_VERTEXFORMAT_FVF_1L: // 1-Link
    case 1:
    {
        size = dwVertCount * sizeof(vertBoned1W);
        vertBoned1W* pVO = (vertBoned1W*)data->pointer();

        for (it = 0; it < dwVertCount; ++it)
        {
            const vertBoned1W& VB = pVO[it];
            u16 mid = (u16)VB.matrix;

            if (bids.end() == std::find(bids.begin(), bids.end(), mid))
                bids.push_back(mid);

            sw_bones_cnt = _max(sw_bones_cnt, mid);
        }
#ifdef _EDITOR
        // software
        crc = crc32(data->pointer(), size);
        Vertices1W.create(crc, dwVertCount, (vertBoned1W*)data->pointer());
#else
        if (1 == bids.size())
        {
            // HW- single bone
            RenderMode = RM_SINGLE;
            RMS_boneid = *bids.begin();
            GEnv.Render->shader_option_skinning(0);
        }
        else if (sw_bones_cnt <= hw_bones_cnt)
        {
            // HW- one weight
            RenderMode = RM_SKINNING_1B;
            RMS_bonecount = sw_bones_cnt + 1;
            GEnv.Render->shader_option_skinning(1);
        }
        else
        {
            // software
            crc = crc32(data->pointer(), size);
            Vertices1W.create(crc, dwVertCount, (vertBoned1W*)data->pointer());
            GEnv.Render->shader_option_skinning(-1);
        }
#endif
    }
    break;
    case OGF_VERTEXFORMAT_FVF_2L: // 2-Link
    case 2:
    {
        size = dwVertCount * sizeof(vertBoned2W);
        vertBoned2W* pVO = (vertBoned2W*)data->pointer();

        for (it = 0; it < dwVertCount; ++it)
        {
            const vertBoned2W& VB = pVO[it];
            sw_bones_cnt = _max(sw_bones_cnt, VB.matrix0);
            sw_bones_cnt = _max(sw_bones_cnt, VB.matrix1);

            if (bids.end() == std::find(bids.begin(), bids.end(), VB.matrix0))
                bids.push_back(VB.matrix0);

            if (bids.end() == std::find(bids.begin(), bids.end(), VB.matrix1))
                bids.push_back(VB.matrix1);
        }
        //.			R_ASSERT(sw_bones_cnt<=hw_bones_cnt);
        if (sw_bones_cnt <= hw_bones_cnt)
        {
            // HW- two weights
            RenderMode = RM_SKINNING_2B;
            RMS_bonecount = sw_bones_cnt + 1;
            GEnv.Render->shader_option_skinning(2);
        }
        else
        {
            // software
            crc = crc32(data->pointer(), size);
            Vertices2W.create(crc, dwVertCount, (vertBoned2W*)data->pointer());
            GEnv.Render->shader_option_skinning(-1);
        }
    }
    break;
    case OGF_VERTEXFORMAT_FVF_3L: // 3-Link
    case 3:
    {
        size = dwVertCount * sizeof(vertBoned3W);
        vertBoned3W* pVO = (vertBoned3W*)data->pointer();

        for (it = 0; it < dwVertCount; ++it)
        {
            const vertBoned3W& VB = pVO[it];
            for (int i = 0; i < 3; ++i)
            {
                sw_bones_cnt = _max(sw_bones_cnt, VB.m[i]);

                if (bids.end() == std::find(bids.begin(), bids.end(), VB.m[i]))
                    bids.push_back(VB.m[i]);
            }
        }
        //.			R_ASSERT(sw_bones_cnt<=hw_bones_cnt);
        if ((sw_bones_cnt <= hw_bones_cnt))
        {
            RenderMode = RM_SKINNING_3B;
            RMS_bonecount = sw_bones_cnt + 1;
            GEnv.Render->shader_option_skinning(3);
        }
        else
        {
            crc = crc32(data->pointer(), size);
            Vertices3W.create(crc, dwVertCount, (vertBoned3W*)data->pointer());
            GEnv.Render->shader_option_skinning(-1);
        }
    }
    break;
    case OGF_VERTEXFORMAT_FVF_4L: // 4-Link
    case 4:
    {
        size = dwVertCount * sizeof(vertBoned4W);
        vertBoned4W* pVO = (vertBoned4W*)data->pointer();

        for (it = 0; it < dwVertCount; ++it)
        {
            const vertBoned4W& VB = pVO[it];

            for (int i = 0; i < 4; ++i)
            {
                sw_bones_cnt = _max(sw_bones_cnt, VB.m[i]);

                if (bids.end() == std::find(bids.begin(), bids.end(), VB.m[i]))
                    bids.push_back(VB.m[i]);
            }
        }
        //.			R_ASSERT(sw_bones_cnt<=hw_bones_cnt);
        if (sw_bones_cnt <= hw_bones_cnt)
        {
            RenderMode = RM_SKINNING_4B;
            RMS_bonecount = sw_bones_cnt + 1;
            GEnv.Render->shader_option_skinning(4);
        }
        else
        {
            crc = crc32(data->pointer(), size);
            Vertices4W.create(crc, dwVertCount, (vertBoned4W*)data->pointer());
            GEnv.Render->shader_option_skinning(-1);
        }
    }
    break;
    default: xrDebug::Fatal(DEBUG_INFO, "Invalid vertex type in skinned model '%s'", N); break;
    }
#ifdef _EDITOR
    if (bids.size() > 0)
#else
    if (bids.size() > 1)
#endif
    {
        crc = crc32(&*bids.begin(), bids.size() * sizeof(u16));
        BonesUsed.create(crc, bids.size(), &*bids.begin());
    }
}

BOOL CSkeletonX::has_visible_bones()
{
    if (RM_SINGLE == RenderMode)
    {
        return Parent->LL_GetBoneVisible((u16)RMS_boneid);
    }

    for (u32 it = 0; it < BonesUsed.size(); it++)
        if (Parent->LL_GetBoneVisible(BonesUsed[it]))
        {
            return TRUE;
        }
    return FALSE;
}

void get_pos_bones(const vertBoned1W& v, Fvector& p, CKinematics* Parent)
{
    const Fmatrix& xform = Parent->LL_GetBoneInstance((u16)v.matrix).mRenderTransform;
    xform.transform_tiny(p, v.P);
}

void get_pos_bones(const vertBoned2W& vert, Fvector& p, CKinematics* Parent)
{
    Fvector P0, P1;

    Fmatrix& xform0 = Parent->LL_GetBoneInstance(vert.matrix0).mRenderTransform;
    Fmatrix& xform1 = Parent->LL_GetBoneInstance(vert.matrix1).mRenderTransform;
    xform0.transform_tiny(P0, vert.P);
    xform1.transform_tiny(P1, vert.P);
    p.lerp(P0, P1, vert.w);
}

void get_pos_bones(const vertBoned3W& vert, Fvector& p, CKinematics* Parent)
{
    Fmatrix& M0 = Parent->LL_GetBoneInstance(vert.m[0]).mRenderTransform;
    Fmatrix& M1 = Parent->LL_GetBoneInstance(vert.m[1]).mRenderTransform;
    Fmatrix& M2 = Parent->LL_GetBoneInstance(vert.m[2]).mRenderTransform;

    Fvector P0, P1, P2;
    M0.transform_tiny(P0, vert.P);
    P0.mul(vert.w[0]);
    M1.transform_tiny(P1, vert.P);
    P1.mul(vert.w[1]);
    M2.transform_tiny(P2, vert.P);
    P2.mul(1.0f - vert.w[0] - vert.w[1]);

    p = P0;
    p.add(P1);
    p.add(P2);
}
void get_pos_bones(const vertBoned4W& vert, Fvector& p, CKinematics* Parent)
{
    Fmatrix& M0 = Parent->LL_GetBoneInstance(vert.m[0]).mRenderTransform;
    Fmatrix& M1 = Parent->LL_GetBoneInstance(vert.m[1]).mRenderTransform;
    Fmatrix& M2 = Parent->LL_GetBoneInstance(vert.m[2]).mRenderTransform;
    Fmatrix& M3 = Parent->LL_GetBoneInstance(vert.m[3]).mRenderTransform;

    Fvector P0, P1, P2, P3;
    M0.transform_tiny(P0, vert.P);
    P0.mul(vert.w[0]);
    M1.transform_tiny(P1, vert.P);
    P1.mul(vert.w[1]);
    M2.transform_tiny(P2, vert.P);
    P2.mul(vert.w[2]);
    M3.transform_tiny(P3, vert.P);
    P3.mul(1.0f - vert.w[0] - vert.w[1] - vert.w[2]);

    p = P0;
    p.add(P1);
    p.add(P2);
    p.add(P3);
}

//-----------------------------------------------------------------------------------------------------
// Wallmarks
//-----------------------------------------------------------------------------------------------------
#include "xrCDB/Intersect.hpp"
BOOL CSkeletonX::_PickBoneSoft1W(IKinematics::pick_result& r, float dist, const Fvector& S, const Fvector& D,
    u16* indices, CBoneData::FacesVec& faces)
{
    return pick_bone<vertBoned1W>(Vertices1W, Parent, r, dist, S, D, indices, faces);
}

BOOL CSkeletonX::_PickBoneSoft2W(IKinematics::pick_result& r, float dist, const Fvector& S, const Fvector& D,
    u16* indices, CBoneData::FacesVec& faces)
{
    return pick_bone<vertBoned2W>(Vertices2W, Parent, r, dist, S, D, indices, faces);
}

BOOL CSkeletonX::_PickBoneSoft3W(IKinematics::pick_result& r, float dist, const Fvector& S, const Fvector& D,
    u16* indices, CBoneData::FacesVec& faces)
{
    return pick_bone<vertBoned3W>(Vertices3W, Parent, r, dist, S, D, indices, faces);
}

BOOL CSkeletonX::_PickBoneSoft4W(IKinematics::pick_result& r, float dist, const Fvector& S, const Fvector& D,
    u16* indices, CBoneData::FacesVec& faces)
{
    return pick_bone<vertBoned4W>(Vertices4W, Parent, r, dist, S, D, indices, faces);
}
/*
BOOL	CSkeletonX::_PickBoneSoft1W	(Fvector& normal, float& dist, const Fvector& S, const Fvector& D, u16* indices,
CBoneData::FacesVec& faces)
{
    VERIFY				(*Vertices1W);
    bool intersect		= FALSE;
    for (CBoneData::auto it=faces.begin(); it!=faces.end(); it++){
        Fvector			p[3];
        u32 idx			= (*it)*3;
        for (u32 k=0; k<3; k++){
            vertBoned1W& vert		= Vertices1W[indices[idx+k]];
            get_pos_bones(vert, p[k], Parent);
        }
        float u,v,range	= flt_max;
        if (CDB::TestRayTri(S,D,p,u,v,range,true)&&(range<dist)){
            normal.mknormal(p[0],p[1],p[2]);
            dist		= range;
            intersect	= TRUE;
        }
    }
    return intersect;
}

BOOL CSkeletonX::_PickBoneSoft2W	(Fvector& normal, float& dist, const Fvector& S, const Fvector& D, u16* indices,
CBoneData::FacesVec& faces)
{
    VERIFY				(*Vertices2W);
    bool intersect		= FALSE;
    for (CBoneData::auto it=faces.begin(); it!=faces.end(); it++){
        Fvector			p[3];
        u32 idx			= (*it)*3;
        for (u32 k=0; k<3; k++){
            vertBoned2W& vert		= Vertices2W[indices[idx+k]];
            get_pos_bones(vert, p[k], Parent);
        }
        float u,v,range	= flt_max;
        if (CDB::TestRayTri(S,D,p,u,v,range,true)&&(range<dist)){
            normal.mknormal(p[0],p[1],p[2]);
            dist		= range;
            intersect	= TRUE;
        }
    }
    return intersect;
}
*/
// Fill Vertices
void CSkeletonX::_FillVerticesSoft1W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
                                     u16* indices, CBoneData::FacesVec& faces)
{
    VERIFY(*Vertices1W);
    for (auto it = faces.begin(); it != faces.end(); it++)
    {
        Fvector p[3];
        u32 idx = (*it) * 3;
        CSkeletonWallmark::WMFace F;
        for (u32 k = 0; k < 3; k++)
        {
            vertBoned1W& vert = Vertices1W[indices[idx + k]];
            F.bone_id[k][0] = (u16)vert.matrix;
            F.bone_id[k][1] = F.bone_id[k][0];
            F.bone_id[k][2] = F.bone_id[k][0];
            F.bone_id[k][3] = F.bone_id[k][0];
            F.weight[k][0] = 0.f;
            F.weight[k][1] = 0.f;
            F.weight[k][2] = 0.f;

            const Fmatrix& xform = Parent->LL_GetBoneInstance(F.bone_id[k][0]).mRenderTransform;
            F.vert[k].set(vert.P);
            xform.transform_tiny(p[k], F.vert[k]);
        }
        Fvector test_normal;
        test_normal.mknormal(p[0], p[1], p[2]);
        float cosa = test_normal.dotproduct(normal);
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
}

void CSkeletonX::_FillVerticesSoft2W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
                                     u16* indices, CBoneData::FacesVec& faces)
{
    VERIFY(*Vertices2W);
    for (auto it = faces.begin(); it != faces.end(); it++)
    {
        Fvector p[3];
        u32 idx = (*it) * 3;
        CSkeletonWallmark::WMFace F;
        for (u32 k = 0; k < 3; k++)
        {
            Fvector P0, P1;

            vertBoned2W& vert = Vertices2W[indices[idx + k]];
            F.bone_id[k][0] = vert.matrix0;
            F.bone_id[k][1] = vert.matrix1;
            F.bone_id[k][2] = F.bone_id[k][1];
            F.bone_id[k][3] = F.bone_id[k][1];
            F.weight[k][0] = vert.w;
            F.weight[k][1] = 0.f;
            F.weight[k][2] = 0.f;

            Fmatrix& xform0 = Parent->LL_GetBoneInstance(F.bone_id[k][0]).mRenderTransform;
            Fmatrix& xform1 = Parent->LL_GetBoneInstance(F.bone_id[k][1]).mRenderTransform;
            F.vert[k].set(vert.P);
            xform0.transform_tiny(P0, F.vert[k]);
            xform1.transform_tiny(P1, F.vert[k]);
            p[k].lerp(P0, P1, F.weight[k][0]);
        }
        Fvector test_normal;
        test_normal.mknormal(p[0], p[1], p[2]);
        float cosa = test_normal.dotproduct(normal);
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
}

void CSkeletonX::_FillVerticesSoft3W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
                                     u16* indices, CBoneData::FacesVec& faces)
{
    VERIFY(*Vertices3W);
    for (auto it = faces.begin(); it != faces.end(); ++it)
    {
        Fvector p[3];
        u32 idx = *it * 3;
        CSkeletonWallmark::WMFace F;

        for (u32 k = 0; k < 3; k++)
        {
            const vertBoned3W& vert = Vertices3W[indices[idx + k]];
            F.bone_id[k][0] = vert.m[0];
            F.bone_id[k][1] = vert.m[1];
            F.bone_id[k][2] = vert.m[2];
            F.bone_id[k][3] = F.bone_id[k][2];
            F.weight[k][0] = vert.w[0];
            F.weight[k][1] = vert.w[1];
            F.weight[k][2] = 0.f;
            vert.get_pos(F.vert[k]);
            get_pos_bones(vert, p[k], Parent);
        }
        Fvector test_normal;
        test_normal.mknormal(p[0], p[1], p[2]);
        float cosa = test_normal.dotproduct(normal);
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
}

void CSkeletonX::_FillVerticesSoft4W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size,
                                     u16* indices, CBoneData::FacesVec& faces)
{
    VERIFY(*Vertices4W);
    for (auto it = faces.begin(); it != faces.end(); ++it)
    {
        Fvector p[3];
        u32 idx = *it * 3;
        CSkeletonWallmark::WMFace F;

        for (u32 k = 0; k < 3; k++)
        {
            const vertBoned4W& vert = Vertices4W[indices[idx + k]];
            F.bone_id[k][0] = vert.m[0];
            F.bone_id[k][1] = vert.m[1];
            F.bone_id[k][2] = vert.m[2];
            F.bone_id[k][3] = vert.m[3];
            F.weight[k][0] = vert.w[0];
            F.weight[k][1] = vert.w[1];
            F.weight[k][2] = vert.w[2];
            vert.get_pos(F.vert[k]);
            get_pos_bones(vert, p[k], Parent);
        }
        Fvector test_normal;
        test_normal.mknormal(p[0], p[1], p[2]);
        float cosa = test_normal.dotproduct(normal);
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
}

#ifndef USE_DX9
void CSkeletonX::_DuplicateIndices(const char* /*N*/, IReader* data)
{
    //	We will have trouble with container since don't know were to take readable indices
    VERIFY(!data->find_chunk(OGF_ICONTAINER));
    //	Index buffer replica since we can't read from index buffer in DX10
    // ref_smem<u16>			Indices;
    R_ASSERT(data->find_chunk(OGF_INDICES));
    u32 iCount = data->r_u32();

    u32 size = iCount * 2;
    u32 crc = crc32(data->pointer(), size);
    m_Indices.create(crc, iCount, (u16*)data->pointer());
}
#endif // !USE_DX9
