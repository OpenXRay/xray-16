#include "stdafx.h"

#include "r__pixel_calculator.h"
#include "Layers/xrRender/FBasicVisual.h"

#if defined(USE_DX11)// XXX: support pixel calculator on OpenGL
#   include <DirectXMath.h>
#endif

static constexpr u32 rt_dimensions = 1024;

void r_pixel_calculator::begin()
{
    rt.create("$user$pixel_calculator_rt", rt_dimensions, rt_dimensions, HW.Caps.fTarget);
    zb.create("$user$pixel_calculator_zb", rt_dimensions, rt_dimensions, HW.Caps.fDepth, 1, { CRT::CreateSurface });

    RCache.set_RT(rt->pRT);
#ifdef USE_DX11
    RCache.set_ZB(zb->pZRT[RCache.context_id]);
#elif defined(USE_OGL)
    RCache.set_ZB(zb->pRT);
#endif

    R_ASSERT(Device.RenderBegin());
}

void r_pixel_calculator::end()
{
    Device.RenderEnd();

    RCache.set_RT(RImplementation.Target->get_base_rt());
    RCache.set_ZB(RImplementation.Target->get_base_zb());

    zb = nullptr;
    rt = nullptr;
}

#if defined(USE_DX11)
// +X, -X, +Y, -Y, +Z, -Z
extern Fvector cmNorm[6];
extern Fvector cmDir [6];
#endif

r_aabb_ssa r_pixel_calculator::calculate(dxRender_Visual* V)
{
#if defined(USE_DX11)
    using namespace DirectX;

    r_aabb_ssa result = {0};
    float area = float(_sqr(rt_dimensions));

    auto& dsgraph = RImplementation.get_imm_context();

    //
    u32 id[6];
    for (u32 face = 0; face < 6; face++)
    {
        // setup matrices
        Fmatrix mProject, mView;
        Fvector vFrom;
        Fbox aabb;

        // camera - left-to-right
        mView.build_camera_dir(vFrom.invert(cmDir[face]).mul(100.f), cmDir[face], cmNorm[face]);
        aabb.xform(V->vis.box, mView);

        XMMATRIX project = XMMatrixOrthographicOffCenterLH(aabb.vMin.x, aabb.vMax.x, aabb.vMin.y, aabb.vMax.y, aabb.vMin.z, aabb.vMax.z);
        XMStoreFloat4x4(reinterpret_cast<XMFLOAT4X4*>(&mProject), project);

        RCache.set_xform_world(Fidentity);
        RCache.set_xform_view(mView);
        RCache.set_xform_project(mProject);

        // render-0
        Device.Clear(); // clear-ZB
        RCache.set_Shader(V->shader);
        V->Render(RCache, 1.f, dsgraph.o.phase == CRender::PHASE_SMAP);

        // render-1
        RImplementation.HWOCC.occq_begin(id[face]);
        V->Render(RCache, 1.f, dsgraph.o.phase == CRender::PHASE_SMAP);
        RImplementation.HWOCC.occq_end(id[face]);
    }

    //
    for (u32 it = 0; it < 6; it++)
    {
        float pixels = (float)RImplementation.HWOCC.occq_get(id[it]);
        float coeff = clampr(pixels / area, float(0), float(1));
        Msg(" - [%d] ssa_c: %1.3f,%f/%f", it, coeff, pixels, area);
        result.ssa[it] = (u8)clampr(iFloor(coeff * 255.f + 0.5f), int(0), int(255));
    }

    return result;
#elif defined(USE_OGL)
    VERIFY(!"Not implemented!");
    return {};
#else
#   error No graphics API selected or enabled!
#endif
}

void r_pixel_calculator::run()
{
    Log("----- ssa build start -----");
    begin();
    for (u32 it = 0; it < RImplementation.Visuals.size(); it++)
    {
        if (0 == dynamic_cast<IRender_Mesh*>(RImplementation.Visuals[it]))
            continue;
        Msg("*%d*", it);
        calculate((dxRender_Visual*)RImplementation.Visuals[it]);
    }
    end();
    Log("----- ssa build end -----");
}
