#include "stdafx.h"
#pragma hdrstop

#ifndef USE_DX9
extern IC u32 GetIndexCount(D3DPRIMITIVETYPE T, u32 iPrimitiveCount);
#endif

void CBackend::InitializeDebugDraw()
{
#ifndef USE_DX9
    vs_L.create(FVF::F_L, RCache.Vertex.Buffer(), RCache.Index.Buffer());
    vs_TL.create(FVF::F_TL, RCache.Vertex.Buffer(), RCache.Index.Buffer());
#endif
}

void CBackend::DestroyDebugDraw()
{
#ifndef USE_DX9
    vs_L.destroy();
    vs_TL.destroy();
#endif
}

void CBackend::dbg_DP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 vBase, u32 pc)
{
    set_Geometry(geom);
    Render(pt, vBase, pc);
}

void CBackend::dbg_DIP(D3DPRIMITIVETYPE pt, ref_geom geom, u32 baseV, u32 startV, u32 countV, u32 startI, u32 PC)
{
    set_Geometry(geom);
    Render(pt, baseV, startV, countV, startI, PC);
}

#ifdef DEBUG

void CBackend::dbg_Draw(D3DPRIMITIVETYPE T, FVF::L* pVerts, int vcnt, u16* pIdx, int pcnt)
{
#ifndef USE_DX9
    u32 vBase;
    {
        FVF::L* pv = (FVF::L*)Vertex.Lock(vcnt, vs_L->vb_stride, vBase);
        for (size_t i = 0; i < vcnt; i++)
        {
            pv[i] = pVerts[i];
        }
        Vertex.Unlock(vcnt, vs_L->vb_stride);
    }

    u32 iBase;
    {
        const size_t count = GetIndexCount(T, pcnt);
        u16* indices = Index.Lock(count, iBase);
        for (size_t i = 0; i < count; i++)
            indices[i] = pIdx[i];
        Index.Unlock(count);
    }
    set_Geometry(vs_L);
    set_RT(RImplementation.Target->get_base_rt());
    RImplementation.rmNormal();
    set_Stencil(FALSE);
    Render(T, vBase, 0, vcnt, iBase, pcnt);
#else // USE_DX9
    OnFrameEnd();
    CHK_DX(HW.pDevice->SetFVF(FVF::F_L));
    CHK_DX(HW.pDevice->DrawIndexedPrimitiveUP(T, 0, vcnt, pcnt, pIdx, D3DFMT_INDEX16, pVerts, sizeof(FVF::L)));
#endif // USE_OGL
}
void CBackend::dbg_Draw(D3DPRIMITIVETYPE T, FVF::L* pVerts, int pcnt)
{
#ifndef USE_DX9
    u32 vBase;
    {
        const size_t count = GetIndexCount(T, pcnt);
        FVF::L* pv = (FVF::L*)Vertex.Lock(count, vs_L->vb_stride, vBase);
        for (size_t i = 0; i < count; i++)
        {
            pv[i] = pVerts[i];
        }
        Vertex.Unlock(count, vs_L->vb_stride);
    }
    set_Geometry(vs_L);
    set_RT(RImplementation.Target->get_base_rt());
    RImplementation.rmFar();
    set_Stencil(FALSE);
    Render(T, vBase, pcnt);
#else // USE_DX9
    OnFrameEnd();
    CHK_DX(HW.pDevice->SetFVF(FVF::F_L));
    CHK_DX(HW.pDevice->DrawPrimitiveUP(T, pcnt, pVerts, sizeof(FVF::L)));
#endif // USE_OGL
}

#define RGBA_GETALPHA(rgb) ((rgb) >> 24)
void CBackend::dbg_DrawOBB(Fmatrix& T, Fvector& half_dim, u32 C)
{
    Fmatrix mL2W_Transform, mScaleTransform;

    mScaleTransform.scale(half_dim);
    mL2W_Transform.mul_43(T, mScaleTransform);

    FVF::L aabb[8];
    aabb[0].set(-1, -1, -1, C); // 0
    aabb[1].set(-1, +1, -1, C); // 1
    aabb[2].set(+1, +1, -1, C); // 2
    aabb[3].set(+1, -1, -1, C); // 3
    aabb[4].set(-1, -1, +1, C); // 4
    aabb[5].set(-1, +1, +1, C); // 5
    aabb[6].set(+1, +1, +1, C); // 6
    aabb[7].set(+1, -1, +1, C); // 7

    u16 aabb_id[12 * 2] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 1, 5, 2, 6, 3, 7, 0, 4};
    set_xform_world(mL2W_Transform);
#ifndef USE_DX9
    RCache.set_c("tfactor", float(color_get_R(C)) / 255.f, float(color_get_G(C)) / 255.f, \
        float(color_get_B(C)) / 255.f, float(color_get_A(C)) / 255.f);
#endif // !USE_DX9
    dbg_Draw(D3DPT_LINELIST, aabb, 8, aabb_id, 12);
}
void CBackend::dbg_DrawTRI(Fmatrix& T, Fvector& p1, Fvector& p2, Fvector& p3, u32 C)
{
    FVF::L tri[3];
    tri[0].p = p1;
    tri[0].color = C;
    tri[1].p = p2;
    tri[1].color = C;
    tri[2].p = p3;
    tri[2].color = C;

    set_xform_world(T);
#ifndef USE_DX9
    RCache.set_c("tfactor", float(color_get_R(C)) / 255.f, float(color_get_G(C)) / 255.f, \
        float(color_get_B(C)) / 255.f, float(color_get_A(C)) / 255.f);
#endif // !USE_DX9
    dbg_Draw(D3DPT_TRIANGLESTRIP, tri, 1);
}
void CBackend::dbg_DrawLINE(Fmatrix& T, Fvector& p1, Fvector& p2, u32 C)
{
    FVF::L line[2];
    line[0].p = p1;
    line[0].color = C;
    line[1].p = p2;
    line[1].color = C;

    set_xform_world(T);
#ifndef USE_DX9
    RCache.set_c("tfactor", float(color_get_R(C)) / 255.f, float(color_get_G(C)) / 255.f, \
        float(color_get_B(C)) / 255.f, float(color_get_A(C)) / 255.f);
#endif // !USE_DX9
    dbg_Draw(D3DPT_LINELIST, line, 1);
}
void CBackend::dbg_DrawEllipse(Fmatrix& T, u32 C)
{
    float gVertices[] = {0.0000f, 0.0000f, 1.0000f, 0.0000f, 0.3827f, 0.9239f, -0.1464f, 0.3536f, 0.9239f, -0.2706f,
        0.2706f, 0.9239f, -0.3536f, 0.1464f, 0.9239f, -0.3827f, 0.0000f, 0.9239f, -0.3536f, -0.1464f, 0.9239f, -0.2706f,
        -0.2706f, 0.9239f, -0.1464f, -0.3536f, 0.9239f, 0.0000f, -0.3827f, 0.9239f, 0.1464f, -0.3536f, 0.9239f, 0.2706f,
        -0.2706f, 0.9239f, 0.3536f, -0.1464f, 0.9239f, 0.3827f, 0.0000f, 0.9239f, 0.3536f, 0.1464f, 0.9239f, 0.2706f,
        0.2706f, 0.9239f, 0.1464f, 0.3536f, 0.9239f, 0.0000f, 0.7071f, 0.7071f, -0.2706f, 0.6533f, 0.7071f, -0.5000f,
        0.5000f, 0.7071f, -0.6533f, 0.2706f, 0.7071f, -0.7071f, 0.0000f, 0.7071f, -0.6533f, -0.2706f, 0.7071f, -0.5000f,
        -0.5000f, 0.7071f, -0.2706f, -0.6533f, 0.7071f, 0.0000f, -0.7071f, 0.7071f, 0.2706f, -0.6533f, 0.7071f, 0.5000f,
        -0.5000f, 0.7071f, 0.6533f, -0.2706f, 0.7071f, 0.7071f, 0.0000f, 0.7071f, 0.6533f, 0.2706f, 0.7071f, 0.5000f,
        0.5000f, 0.7071f, 0.2706f, 0.6533f, 0.7071f, 0.0000f, 0.9239f, 0.3827f, -0.3536f, 0.8536f, 0.3827f, -0.6533f,
        0.6533f, 0.3827f, -0.8536f, 0.3536f, 0.3827f, -0.9239f, 0.0000f, 0.3827f, -0.8536f, -0.3536f, 0.3827f, -0.6533f,
        -0.6533f, 0.3827f, -0.3536f, -0.8536f, 0.3827f, 0.0000f, -0.9239f, 0.3827f, 0.3536f, -0.8536f, 0.3827f, 0.6533f,
        -0.6533f, 0.3827f, 0.8536f, -0.3536f, 0.3827f, 0.9239f, 0.0000f, 0.3827f, 0.8536f, 0.3536f, 0.3827f, 0.6533f,
        0.6533f, 0.3827f, 0.3536f, 0.8536f, 0.3827f, 0.0000f, 1.0000f, 0.0000f, -0.3827f, 0.9239f, 0.0000f, -0.7071f,
        0.7071f, 0.0000f, -0.9239f, 0.3827f, 0.0000f, -1.0000f, 0.0000f, 0.0000f, -0.9239f, -0.3827f, 0.0000f, -0.7071f,
        -0.7071f, 0.0000f, -0.3827f, -0.9239f, 0.0000f, 0.0000f, -1.0000f, 0.0000f, 0.3827f, -0.9239f, 0.0000f, 0.7071f,
        -0.7071f, 0.0000f, 0.9239f, -0.3827f, 0.0000f, 1.0000f, 0.0000f, 0.0000f, 0.9239f, 0.3827f, 0.0000f, 0.7071f,
        0.7071f, 0.0000f, 0.3827f, 0.9239f, 0.0000f, 0.0000f, 0.9239f, -0.3827f, -0.3536f, 0.8536f, -0.3827f, -0.6533f,
        0.6533f, -0.3827f, -0.8536f, 0.3536f, -0.3827f, -0.9239f, 0.0000f, -0.3827f, -0.8536f, -0.3536f, -0.3827f,
        -0.6533f, -0.6533f, -0.3827f, -0.3536f, -0.8536f, -0.3827f, 0.0000f, -0.9239f, -0.3827f, 0.3536f, -0.8536f,
        -0.3827f, 0.6533f, -0.6533f, -0.3827f, 0.8536f, -0.3536f, -0.3827f, 0.9239f, 0.0000f, -0.3827f, 0.8536f,
        0.3536f, -0.3827f, 0.6533f, 0.6533f, -0.3827f, 0.3536f, 0.8536f, -0.3827f, 0.0000f, 0.7071f, -0.7071f, -0.2706f,
        0.6533f, -0.7071f, -0.5000f, 0.5000f, -0.7071f, -0.6533f, 0.2706f, -0.7071f, -0.7071f, 0.0000f, -0.7071f,
        -0.6533f, -0.2706f, -0.7071f, -0.5000f, -0.5000f, -0.7071f, -0.2706f, -0.6533f, -0.7071f, 0.0000f, -0.7071f,
        -0.7071f, 0.2706f, -0.6533f, -0.7071f, 0.5000f, -0.5000f, -0.7071f, 0.6533f, -0.2706f, -0.7071f, 0.7071f,
        0.0000f, -0.7071f, 0.6533f, 0.2706f, -0.7071f, 0.5000f, 0.5000f, -0.7071f, 0.2706f, 0.6533f, -0.7071f, 0.0000f,
        0.3827f, -0.9239f, -0.1464f, 0.3536f, -0.9239f, -0.2706f, 0.2706f, -0.9239f, -0.3536f, 0.1464f, -0.9239f,
        -0.3827f, 0.0000f, -0.9239f, -0.3536f, -0.1464f, -0.9239f, -0.2706f, -0.2706f, -0.9239f, -0.1464f, -0.3536f,
        -0.9239f, 0.0000f, -0.3827f, -0.9239f, 0.1464f, -0.3536f, -0.9239f, 0.2706f, -0.2706f, -0.9239f, 0.3536f,
        -0.1464f, -0.9239f, 0.3827f, 0.0000f, -0.9239f, 0.3536f, 0.1464f, -0.9239f, 0.2706f, 0.2706f, -0.9239f, 0.1464f,
        0.3536f, -0.9239f, 0.0000f, 0.0000f, -1.0000f};

    u16 gFaces[224 * 3] = {0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 5, 0, 5, 6, 0, 6, 7, 0, 7, 8, 0, 8, 9, 0, 9, 10, 0, 10, 11,
        0, 11, 12, 0, 12, 13, 0, 13, 14, 0, 14, 15, 0, 15, 16, 0, 16, 1, 1, 17, 18, 1, 18, 2, 2, 18, 19, 2, 19, 3, 3,
        19, 20, 3, 20, 4, 4, 20, 21, 4, 21, 5, 5, 21, 22, 5, 22, 6, 6, 22, 23, 6, 23, 7, 7, 23, 24, 7, 24, 8, 8, 24, 25,
        8, 25, 9, 9, 25, 26, 9, 26, 10, 10, 26, 27, 10, 27, 11, 11, 27, 28, 11, 28, 12, 12, 28, 29, 12, 29, 13, 13, 29,
        30, 13, 30, 14, 14, 30, 31, 14, 31, 15, 15, 31, 32, 15, 32, 16, 16, 32, 17, 16, 17, 1, 17, 33, 34, 17, 34, 18,
        18, 34, 35, 18, 35, 19, 19, 35, 36, 19, 36, 20, 20, 36, 37, 20, 37, 21, 21, 37, 38, 21, 38, 22, 22, 38, 39, 22,
        39, 23, 23, 39, 40, 23, 40, 24, 24, 40, 41, 24, 41, 25, 25, 41, 42, 25, 42, 26, 26, 42, 43, 26, 43, 27, 27, 43,
        44, 27, 44, 28, 28, 44, 45, 28, 45, 29, 29, 45, 46, 29, 46, 30, 30, 46, 47, 30, 47, 31, 31, 47, 48, 31, 48, 32,
        32, 48, 33, 32, 33, 17, 33, 49, 50, 33, 50, 34, 34, 50, 51, 34, 51, 35, 35, 51, 52, 35, 52, 36, 36, 52, 53, 36,
        53, 37, 37, 53, 54, 37, 54, 38, 38, 54, 55, 38, 55, 39, 39, 55, 56, 39, 56, 40, 40, 56, 57, 40, 57, 41, 41, 57,
        58, 41, 58, 42, 42, 58, 59, 42, 59, 43, 43, 59, 60, 43, 60, 44, 44, 60, 61, 44, 61, 45, 45, 61, 62, 45, 62, 46,
        46, 62, 63, 46, 63, 47, 47, 63, 64, 47, 64, 48, 48, 64, 49, 48, 49, 33, 49, 65, 66, 49, 66, 50, 50, 66, 67, 50,
        67, 51, 51, 67, 68, 51, 68, 52, 52, 68, 69, 52, 69, 53, 53, 69, 70, 53, 70, 54, 54, 70, 71, 54, 71, 55, 55, 71,
        72, 55, 72, 56, 56, 72, 73, 56, 73, 57, 57, 73, 74, 57, 74, 58, 58, 74, 75, 58, 75, 59, 59, 75, 76, 59, 76, 60,
        60, 76, 77, 60, 77, 61, 61, 77, 78, 61, 78, 62, 62, 78, 79, 62, 79, 63, 63, 79, 80, 63, 80, 64, 64, 80, 65, 64,
        65, 49, 65, 81, 82, 65, 82, 66, 66, 82, 83, 66, 83, 67, 67, 83, 84, 67, 84, 68, 68, 84, 85, 68, 85, 69, 69, 85,
        86, 69, 86, 70, 70, 86, 87, 70, 87, 71, 71, 87, 88, 71, 88, 72, 72, 88, 89, 72, 89, 73, 73, 89, 90, 73, 90, 74,
        74, 90, 91, 74, 91, 75, 75, 91, 92, 75, 92, 76, 76, 92, 93, 76, 93, 77, 77, 93, 94, 77, 94, 78, 78, 94, 95, 78,
        95, 79, 79, 95, 96, 79, 96, 80, 80, 96, 81, 80, 81, 65, 81, 97, 98, 81, 98, 82, 82, 98, 99, 82, 99, 83, 83, 99,
        100, 83, 100, 84, 84, 100, 101, 84, 101, 85, 85, 101, 102, 85, 102, 86, 86, 102, 103, 86, 103, 87, 87, 103, 104,
        87, 104, 88, 88, 104, 105, 88, 105, 89, 89, 105, 106, 89, 106, 90, 90, 106, 107, 90, 107, 91, 91, 107, 108, 91,
        108, 92, 92, 108, 109, 92, 109, 93, 93, 109, 110, 93, 110, 94, 94, 110, 111, 94, 111, 95, 95, 111, 112, 95, 112,
        96, 96, 112, 97, 96, 97, 81, 113, 98, 97, 113, 99, 98, 113, 100, 99, 113, 101, 100, 113, 102, 101, 113, 103,
        102, 113, 104, 103, 113, 105, 104, 113, 106, 105, 113, 107, 106, 113, 108, 107, 113, 109, 108, 113, 110, 109,
        113, 111, 110, 113, 112, 111, 113, 97, 112};

    const int vcnt = sizeof(gVertices) / (sizeof(float) * 3);
    FVF::L verts[vcnt];
    for (int i = 0; i < vcnt; i++)
    {
        int k = i * 3;
        verts[i].set(gVertices[k], gVertices[k + 1], gVertices[k + 2], C);
    }

    set_xform_world(T);
#ifndef USE_DX9
    RCache.set_c("tfactor", float(color_get_R(C)) / 255.f, float(color_get_G(C)) / 255.f, \
        float(color_get_B(C)) / 255.f, float(color_get_A(C)) / 255.f);
#endif // !USE_DX9

    RCache.set_FillMode(D3DFILL_WIREFRAME);
    dbg_Draw(D3DPT_TRIANGLELIST, verts, vcnt, gFaces, 224);
    RCache.set_FillMode(D3DFILL_SOLID);
}
#endif

void CBackend::dbg_OverdrawBegin()
{
    // Turn stenciling
    u32 zfail = D3DSTENCILOP_INCRSAT; // ZB access
    if (1 == HW.Caps.SceneMode)
        zfail = D3DSTENCILOP_KEEP; // Overdraw

    // Increment the stencil buffer for each pixel drawn
    set_Stencil(TRUE, D3DCMP_ALWAYS, 0, 0x00000000, 0xffffffff,
        D3DSTENCILOP_KEEP, D3DSTENCILOP_INCRSAT, zfail);
}

void CBackend::dbg_OverdrawEnd()
{
    set_Stencil(TRUE, D3DCMP_EQUAL, 0, 0xff, 0xffffffff,
        D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP);

    // Set the background to black
    RCache.ClearRT(get_RT(), color_xrgb(255, 0, 0)); // XXX: it's red, not black. Check why.

    OnFrameEnd();

    // Draw a rectangle wherever the count equal I
#ifdef USE_DX9
    CHK_DX(HW.pDevice->SetFVF(FVF::F_TL));
#else
    set_Geometry(vs_TL);
#endif

    // Render gradients
    for (int I = 0; I < 12; I++)
    {
        u32 _c = I * 256 / 13;
        u32 c = color_xrgb(_c, _c, _c);
#ifdef USE_DX9
        FVF::TL pv[4];
        pv[0].set(float(0), float(Device.dwHeight), c, 0, 0);
        pv[1].set(float(0), float(0), c, 0, 0);
        pv[2].set(float(Device.dwWidth), float(Device.dwHeight), c, 0, 0);
        pv[3].set(float(Device.dwWidth), float(0), c, 0, 0);
        CHK_DX(HW.pDevice->SetRenderState(D3DRS_STENCILREF, I));
        CHK_DX(HW.pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, pv, sizeof(FVF::TL)));
#else
        u32 vBase;
        FVF::TL* pv = (FVF::TL*)Vertex.Lock(4, vs_L->vb_stride, vBase);
        pv[0].set(float(0), float(Device.dwHeight), c, 0, 0);
        pv[1].set(float(0), float(0), c, 0, 0);
        pv[2].set(float(Device.dwWidth), float(Device.dwHeight), c, 0, 0);
        pv[3].set(float(Device.dwWidth), float(0), c, 0, 0);
        Vertex.Unlock(4, vs_L->vb_stride);
        // Set up the stencil states
        set_Stencil(TRUE, D3DCMP_EQUAL, I, 0xff, 0xffffffff,
            D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP, D3DSTENCILOP_KEEP);
        Render(D3DPT_TRIANGLESTRIP, vBase, 4);
#endif
    }
    set_Stencil(FALSE);
}

void CBackend::dbg_SetRS(D3DRENDERSTATETYPE p1, u32 p2)
{
#ifdef USE_DX9
    CHK_DX(HW.pDevice->SetRenderState(p1, p2));
#else
    VERIFY(!"Not implemented");
#endif
}

void CBackend::dbg_SetSS(u32 sampler, D3DSAMPLERSTATETYPE type, u32 value)
{
#ifdef USE_DX9
    CHK_DX(HW.pDevice->SetSamplerState(sampler, type, value));
#else
    VERIFY(!"Not implemented");
#endif
}
