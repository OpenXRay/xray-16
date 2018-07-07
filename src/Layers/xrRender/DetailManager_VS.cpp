#include "stdafx.h"
#pragma hdrstop
#include "DetailManager.h"
#ifdef _EDITOR
#include "IGame_Persistent.h"
#include "Environment.h"
#else
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"
#endif
#include "Layers/xrRenderDX10/dx10BufferUtils.h"
#include "Layers/xrRenderGL/glBufferUtils.h"

const int quant = 16384;
const int c_hdr = 10;
const int c_size = 4;

static D3DVERTEXELEMENT9 dwDecl[] = {{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, // pos
    {0, 12, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0}, // uv
    D3DDECL_END()};

#pragma pack(push, 1)
struct vertHW
{
    float x, y, z;
    short u, v, t, mid;
};
#pragma pack(pop)

short QC(float v)
{
    int t = iFloor(v * float(quant));
    clamp(t, -32768, 32767);
    return short(t & 0xffff);
}

void CDetailManager::hw_Load()
{
    hw_Load_Geom();
    hw_Load_Shaders();
}

void CDetailManager::hw_Load_Geom()
{
    // Analyze batch-size
    hw_BatchSize = (u32(HW.Caps.geometry.dwRegisters) - c_hdr) / c_size;
    clamp(hw_BatchSize, (u32)0, (u32)64);
    Msg("* [DETAILS] VertexConsts(%d), Batch(%d)", u32(HW.Caps.geometry.dwRegisters), hw_BatchSize);

    // Pre-process objects
    u32 dwVerts = 0;
    u32 dwIndices = 0;
    for (u32 o = 0; o < objects.size(); o++)
    {
        const CDetail& D = *objects[o];
        dwVerts += D.number_vertices * hw_BatchSize;
        dwIndices += D.number_indices * hw_BatchSize;
    }
    u32 vSize = sizeof(vertHW);
    Msg("* [DETAILS] %d v(%d), %d p", dwVerts, vSize, dwIndices / 3);

#if !defined(USE_DX10) && !defined(USE_DX11) && !defined(USE_OGL)
    // Determine POOL & USAGE
    u32 dwUsage = D3DUSAGE_WRITEONLY;

    // Create VB/IB
    R_CHK(HW.pDevice->CreateVertexBuffer(dwVerts * vSize, dwUsage, 0, D3DPOOL_MANAGED, &hw_VB, nullptr));
    HW.stats_manager.increment_stats_vb(hw_VB);
    R_CHK(HW.pDevice->CreateIndexBuffer(dwIndices * 2, dwUsage, D3DFMT_INDEX16, D3DPOOL_MANAGED, &hw_IB, nullptr));
    HW.stats_manager.increment_stats_ib(hw_IB);

#endif //	USE_DX10
    Msg("* [DETAILS] Batch(%d), VB(%dK), IB(%dK)", hw_BatchSize, (dwVerts * vSize) / 1024, (dwIndices * 2) / 1024);

    // Fill VB
    {
        vertHW* pV;
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
        vertHW* pVOriginal;
        pVOriginal = xr_alloc<vertHW>(dwVerts);
        pV = pVOriginal;
#else //	USE_DX10
        R_CHK(hw_VB->Lock(0, 0, (void**)&pV, 0));
#endif //	USE_DX10
        for (u32 o = 0; o < objects.size(); o++)
        {
            const CDetail& D = *objects[o];
            for (u32 batch = 0; batch < hw_BatchSize; batch++)
            {
                u32 mid = batch * c_size;
                for (u32 v = 0; v < D.number_vertices; v++)
                {
                    const Fvector& vP = D.vertices[v].P;
                    pV->x = vP.x;
                    pV->y = vP.y;
                    pV->z = vP.z;
                    pV->u = QC(D.vertices[v].u);
                    pV->v = QC(D.vertices[v].v);
                    pV->t = QC(vP.y / (D.bv_bb.vMax.y - D.bv_bb.vMin.y));
                    pV->mid = short(mid);
                    pV++;
                }
            }
        }
#if defined(USE_OGL)
        glBufferUtils::CreateVertexBuffer(&hw_VB, pVOriginal, dwVerts * vSize);
        xr_free(pVOriginal);
#elif defined(USE_DX10) || defined(USE_DX11)
        R_CHK(dx10BufferUtils::CreateVertexBuffer(&hw_VB, pVOriginal, dwVerts * vSize));
        HW.stats_manager.increment_stats_vb(hw_VB);
        xr_free(pVOriginal);
#else //	USE_DX10
        R_CHK(hw_VB->Unlock());
#endif //	USE_DX10
    }

    // Fill IB
    {
        u16* pI;
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
        u16* pIOriginal;
        pIOriginal = xr_alloc<u16>(dwIndices);
        pI = pIOriginal;
#else //	USE_DX10
        R_CHK(hw_IB->Lock(0, 0, (void**)(&pI), 0));
#endif //	USE_DX10
        for (u32 o = 0; o < objects.size(); o++)
        {
            const CDetail& D = *objects[o];
            u16 offset = 0;
            for (u32 batch = 0; batch < hw_BatchSize; batch++)
            {
                for (u32 i = 0; i < u32(D.number_indices); i++)
                    *pI++ = u16(u16(D.indices[i]) + u16(offset));
                offset = u16(offset + u16(D.number_vertices));
            }
        }
#if defined(USE_OGL)
        glBufferUtils::CreateIndexBuffer(&hw_IB, pIOriginal, dwIndices * 2);
        xr_free(pIOriginal);
#elif defined(USE_DX10) || defined(USE_DX11)
        R_CHK(dx10BufferUtils::CreateIndexBuffer(&hw_IB, pIOriginal, dwIndices * 2));
        HW.stats_manager.increment_stats_ib(hw_IB);
        xr_free(pIOriginal);
#else //	USE_DX10
        R_CHK(hw_IB->Unlock());
#endif //	USE_DX10
    }

    // Declare geometry
    hw_Geom.create(dwDecl, hw_VB, hw_IB);
}

void CDetailManager::hw_Unload()
{
    // Destroy VS/VB/IB
    hw_Geom.destroy();
#ifdef USE_OGL
    GLuint buffers[] = { hw_IB, hw_VB };
    glDeleteBuffers(2, buffers);
#else
    HW.stats_manager.decrement_stats_vb(hw_VB);
    HW.stats_manager.decrement_stats_ib(hw_IB);
    _RELEASE(hw_IB);
    _RELEASE(hw_VB);
#endif // USE_OGL
}

#if !defined(USE_DX10) && !defined(USE_DX11) && !defined(USE_OGL)
void CDetailManager::hw_Load_Shaders()
{
    // Create shader to access constant storage
    ref_shader S;
    S.create("details\\set");
    R_constant_table& T0 = *(S->E[0]->passes[0]->constants);
    R_constant_table& T1 = *(S->E[1]->passes[0]->constants);
    hwc_consts = T0.get("consts");
    hwc_wave = T0.get("wave");
    hwc_wind = T0.get("dir2D");
    hwc_array = T0.get("array");
    hwc_s_consts = T1.get("consts");
    hwc_s_xform = T1.get("xform");
    hwc_s_array = T1.get("array");
}

void CDetailManager::hw_Render()
{
    // Render-prepare
    //	Update timer
    //	Can't use RDEVICE.fTimeDelta since it is smoothed! Don't know why, but smoothed value looks more choppy!
    float fDelta = RDEVICE.fTimeGlobal - m_global_time_old;
    if ((fDelta < 0) || (fDelta > 1))
        fDelta = 0.03;
    m_global_time_old = RDEVICE.fTimeGlobal;

    m_time_rot_1 += (PI_MUL_2 * fDelta / swing_current.rot1);
    m_time_rot_2 += (PI_MUL_2 * fDelta / swing_current.rot2);
    m_time_pos += fDelta * swing_current.speed;

    // float		tm_rot1		= (PI_MUL_2*RDEVICE.fTimeGlobal/swing_current.rot1);
    // float		tm_rot2		= (PI_MUL_2*RDEVICE.fTimeGlobal/swing_current.rot2);
    float tm_rot1 = m_time_rot_1;
    float tm_rot2 = m_time_rot_2;

    Fvector4 dir1, dir2;
    dir1.set(_sin(tm_rot1), 0, _cos(tm_rot1), 0).normalize().mul(swing_current.amp1);
    dir2.set(_sin(tm_rot2), 0, _cos(tm_rot2), 0).normalize().mul(swing_current.amp2);

    // Setup geometry and DMA
    RCache.set_Geometry(hw_Geom);

    // Wave0
    float scale = 1.f / float(quant);
    Fvector4 wave;
    // wave.set				(1.f/5.f,		1.f/7.f,	1.f/3.f,	RDEVICE.fTimeGlobal*swing_current.speed);
    wave.set(1.f / 5.f, 1.f / 7.f, 1.f / 3.f, m_time_pos);
    RCache.set_c(&*hwc_consts, scale, scale, ps_r__Detail_l_aniso, ps_r__Detail_l_ambient); // consts
    RCache.set_c(&*hwc_wave, wave.div(PI_MUL_2)); // wave
    RCache.set_c(&*hwc_wind, dir1); // wind-dir
    hw_Render_dump(&*hwc_array, 1, 0, c_hdr);

    // Wave1
    // wave.set				(1.f/3.f,		1.f/7.f,	1.f/5.f,	RDEVICE.fTimeGlobal*swing_current.speed);
    wave.set(1.f / 3.f, 1.f / 7.f, 1.f / 5.f, m_time_pos);
    RCache.set_c(&*hwc_wave, wave.div(PI_MUL_2)); // wave
    RCache.set_c(&*hwc_wind, dir2); // wind-dir
    hw_Render_dump(&*hwc_array, 2, 0, c_hdr);

    // Still
    RCache.set_c(&*hwc_s_consts, scale, scale, scale, 1.f);
    RCache.set_c(&*hwc_s_xform, RDEVICE.mFullTransform);
    hw_Render_dump(&*hwc_s_array, 0, 1, c_hdr);
}

void CDetailManager::hw_Render_dump(ref_constant x_array, u32 var_id, u32 lod_id, u32 /*c_offset*/)
{
    RImplementation.BasicStats.DetailCount = 0;

    // Matrices and offsets
    u32 vOffset = 0;
    u32 iOffset = 0;

    vis_list& list = m_visibles[var_id];

    Fvector c_sun, c_ambient, c_hemi;
#ifndef _EDITOR
    CEnvDescriptor& desc = *g_pGamePersistent->Environment().CurrentEnv;
    c_sun.set(desc.sun_color.x, desc.sun_color.y, desc.sun_color.z);
    c_sun.mul(.5f);
    c_ambient.set(desc.ambient.x, desc.ambient.y, desc.ambient.z);
    c_hemi.set(desc.hemi_color.x, desc.hemi_color.y, desc.hemi_color.z);
#else
    c_sun.set(1, 1, 1);
    c_sun.mul(.5f);
    c_ambient.set(1, 1, 1);
    c_hemi.set(1, 1, 1);
#endif

    VERIFY(objects.size() <= list.size());

    // Iterate
    for (u32 O = 0; O < objects.size(); O++)
    {
        CDetail& Object = *objects[O];
        xr_vector<SlotItemVec*>& vis = list[O];
        if (!vis.empty())
        {
            // Setup matrices + colors (and flush it as nesessary)
            RCache.set_Element(Object.shader->E[lod_id]);
            RImplementation.apply_lmaterial();
            u32 c_base = x_array->vs.index;
            Fvector4* c_storage = RCache.get_ConstantCache_Vertex().get_array_f().access(c_base);

            u32 dwBatch = 0;

            xr_vector<SlotItemVec*>::iterator _vI = vis.begin();
            xr_vector<SlotItemVec*>::iterator _vE = vis.end();
            for (; _vI != _vE; _vI++)
            {
                SlotItemVec* items = *_vI;
                auto _iI = items->begin();
                auto _iE = items->end();
                for (; _iI != _iE; _iI++)
                {
                    SlotItem& Instance = **_iI;
                    u32 base = dwBatch * 4;

                    // Build matrix ( 3x4 matrix, last row - color )
                    float scale = Instance.scale_calculated;
                    Fmatrix& M = Instance.mRotY;
                    c_storage[base + 0].set(M._11 * scale, M._21 * scale, M._31 * scale, M._41);
                    c_storage[base + 1].set(M._12 * scale, M._22 * scale, M._32 * scale, M._42);
                    c_storage[base + 2].set(M._13 * scale, M._23 * scale, M._33 * scale, M._43);

// Build color
#if RENDER == R_R1
                    Fvector C;
                    C.set(c_ambient);
                    //					C.mad					(c_lmap,Instance.c_rgb);
                    C.mad(c_hemi, Instance.c_hemi);
                    C.mad(c_sun, Instance.c_sun);
                    c_storage[base + 3].set(C.x, C.y, C.z, 1.f);
#else
                    // R2 only needs hemisphere
                    float h = Instance.c_hemi;
                    float s = Instance.c_sun;
                    c_storage[base + 3].set(s, s, s, h);
#endif
                    dwBatch++;
                    if (dwBatch == hw_BatchSize)
                    {
                        // flush
                        RImplementation.BasicStats.DetailCount += dwBatch;
                        u32 dwCNT_verts = dwBatch * Object.number_vertices;
                        u32 dwCNT_prims = (dwBatch * Object.number_indices) / 3;
                        RCache.get_ConstantCache_Vertex().b_dirty = TRUE;
                        RCache.get_ConstantCache_Vertex().get_array_f().dirty(c_base, c_base + dwBatch * 4);
                        RCache.Render(D3DPT_TRIANGLELIST, vOffset, 0, dwCNT_verts, iOffset, dwCNT_prims);
                        RCache.stat.r.s_details.add(dwCNT_verts);

                        // restart
                        dwBatch = 0;
                    }
                }
            }
            // flush if nessecary
            if (dwBatch)
            {
                RImplementation.BasicStats.DetailCount += dwBatch;
                u32 dwCNT_verts = dwBatch * Object.number_vertices;
                u32 dwCNT_prims = (dwBatch * Object.number_indices) / 3;
                RCache.get_ConstantCache_Vertex().b_dirty = TRUE;
                RCache.get_ConstantCache_Vertex().get_array_f().dirty(c_base, c_base + dwBatch * 4);
                RCache.Render(D3DPT_TRIANGLELIST, vOffset, 0, dwCNT_verts, iOffset, dwCNT_prims);
                RCache.stat.r.s_details.add(dwCNT_verts);
            }
        }
        vOffset += hw_BatchSize * Object.number_vertices;
        iOffset += hw_BatchSize * Object.number_indices;
    }
}

#endif //	USE_DX10
