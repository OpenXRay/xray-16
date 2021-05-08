#include "stdafx.h"
#pragma hdrstop

#include "xrCore/FMesh.hpp"
#include "FVisual.h"
#include "Layers/xrRender/BufferUtils.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Fvisual::Fvisual() : dxRender_Visual() { m_fast = nullptr; }
Fvisual::~Fvisual()
{
    xr_delete(m_fast);
}

void Fvisual::Release() { dxRender_Visual::Release(); }
void Fvisual::Load(const char* N, IReader* data, u32 dwFlags)
{
    dxRender_Visual::Load(N, data, dwFlags);

    u32 fvf = 0;
    VertexElement* vFormat = nullptr;
    dwPrimitives = 0;
    BOOL loaded_v = false;

    if (data->find_chunk(OGF_GCONTAINER))
    {
#ifndef _EDITOR
        // verts
        u32 ID = data->r_u32();
        vBase = data->r_u32();
        vCount = data->r_u32();

        VERIFY(nullptr == p_rm_Vertices);
        p_rm_Vertices = RImplementation.getVB(ID);
        p_rm_Vertices->AddRef();

        vFormat = RImplementation.getVB_Format(ID);
        loaded_v = true;

        // indices
        ID = data->r_u32();
        iBase = data->r_u32();
        iCount = data->r_u32();
        dwPrimitives = iCount / 3;

        VERIFY(nullptr == p_rm_Indices);
        p_rm_Indices = RImplementation.getIB(ID);
        p_rm_Indices->AddRef();
#endif
        // check for fast-vertices
#if RENDER == R_R1
        if (data->find_chunk(OGF_FASTPATH) && ps_r1_force_geomx)
#else
        if (data->find_chunk(OGF_FASTPATH))
#endif
        {
            destructor<IReader> geomdef(data->open_chunk(OGF_FASTPATH));
            destructor<IReader> def(geomdef().open_chunk(OGF_GCONTAINER));

            // we have fast-mesh
            m_fast = xr_new<IRender_Mesh>();

            // verts
            VertexElement* fmt = nullptr;
            ID = def().r_u32();
            m_fast->vBase = def().r_u32();
            m_fast->vCount = def().r_u32();

            VERIFY(nullptr == m_fast->p_rm_Vertices);
            m_fast->p_rm_Vertices = RImplementation.getVB(ID, true);
            m_fast->p_rm_Vertices->AddRef();

            fmt = RImplementation.getVB_Format(ID, true);

            // indices
            ID = def().r_u32();
            m_fast->iBase = def().r_u32();
            m_fast->iCount = def().r_u32();
            m_fast->dwPrimitives = iCount / 3;

            VERIFY(nullptr == m_fast->p_rm_Indices);
            m_fast->p_rm_Indices = RImplementation.getIB(ID, true);
            m_fast->p_rm_Indices->AddRef();

            // geom
            m_fast->rm_geom.create(fmt, *m_fast->p_rm_Vertices, *m_fast->p_rm_Indices);
        }
    }

    // read vertices
    if (!loaded_v && (dwFlags & VLOAD_NOVERTICES) == 0)
    {
        if (data->find_chunk(OGF_VCONTAINER))
        {
            R_ASSERT2(0, "pls notify andy about this.");
#ifndef _EDITOR
            u32 ID = data->r_u32();
            vBase = data->r_u32();
            vCount = data->r_u32();

            VERIFY(nullptr == p_rm_Vertices);
            p_rm_Vertices = RImplementation.getVB(ID);
            p_rm_Vertices->AddRef();

            vFormat = RImplementation.getVB_Format(ID);
#endif // !_EDITOR
        }
        else
        {
            R_ASSERT(data->find_chunk(OGF_VERTICES));
            vBase = 0;
            fvf = data->r_u32();
            vCount = data->r_u32();

            VERIFY(nullptr == p_rm_Vertices);
            vStride = GetFVFVertexSize(fvf);
            p_rm_Vertices = xr_new<VertexStagingBuffer>();
            p_rm_Vertices->Create(vCount * vStride);
            u8* bytes = static_cast<u8*>(p_rm_Vertices->Map());
            CopyMemory(bytes, data->pointer(), vCount * vStride);
            p_rm_Vertices->Unmap(true); // upload vertex data
        }
    }

    // indices
    if (!loaded_v)
    {
        dwPrimitives = 0;
        if (data->find_chunk(OGF_ICONTAINER))
        {
            R_ASSERT2(0, "pls notify andy about this.");
#ifndef _EDITOR
            u32 ID = data->r_u32();
            iBase = data->r_u32();
            iCount = data->r_u32();
            dwPrimitives = iCount / 3;
            VERIFY(nullptr == p_rm_Indices);
            p_rm_Indices = RImplementation.getIB(ID);
            p_rm_Indices->AddRef();
#endif // !_EDITOR
        }
        else
        {
            R_ASSERT(data->find_chunk(OGF_INDICES));
            iBase = 0;
            iCount = data->r_u32();
            dwPrimitives = iCount / 3;

            VERIFY(nullptr == p_rm_Indices);
            p_rm_Indices = xr_new<IndexStagingBuffer>();
            p_rm_Indices->Create(iCount * 2, true); // indices are read in model-wallmarks code
            u8* bytes = static_cast<u8*>(p_rm_Indices->Map());
            CopyMemory(bytes, data->pointer(), iCount * 2);
            p_rm_Indices->Unmap(true); // upload index data
        }
    }

    if (dwFlags & VLOAD_NOVERTICES)
        return;
    else if (fvf)
        rm_geom.create(fvf, *p_rm_Vertices, *p_rm_Indices);
    else
        rm_geom.create(vFormat, *p_rm_Vertices, *p_rm_Indices);
}

void Fvisual::Render(float)
{
#if RENDER == R_R1
    if (m_fast && ps_r1_force_geomx)
#else
    if (m_fast && (ps_r1_force_geomx || RImplementation.phase == CRender::PHASE_SMAP && !RCache.is_TessEnabled()))
#endif
    {
        RCache.set_Geometry(m_fast->rm_geom);
        RCache.Render(D3DPT_TRIANGLELIST, m_fast->vBase, 0, m_fast->vCount, m_fast->iBase, m_fast->dwPrimitives);
        RCache.stat.r.s_static.add(m_fast->vCount);
    }
    else
    {
        RCache.set_Geometry(rm_geom);
        RCache.Render(D3DPT_TRIANGLELIST, vBase, 0, vCount, iBase, dwPrimitives);
        RCache.stat.r.s_static.add(vCount);
    }
}

#define PCOPY(a) a = pFrom->a
void Fvisual::Copy(dxRender_Visual* pSrc)
{
    dxRender_Visual::Copy(pSrc);

    Fvisual* pFrom = dynamic_cast<Fvisual*>(pSrc);

    PCOPY(rm_geom);
    PCOPY(p_rm_Vertices);
    if (p_rm_Vertices)
        p_rm_Vertices->AddRef();
    PCOPY(vBase);
    PCOPY(vCount);
    PCOPY(vStride);
    PCOPY(p_rm_Indices);
    if (p_rm_Indices)
        p_rm_Indices->AddRef();
    PCOPY(iBase);
    PCOPY(iCount);
    PCOPY(dwPrimitives);

    PCOPY(m_fast);
}
