#include "stdafx.h"
#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"

extern "C"
{
XR_EXPORT void SetupEnv()
{
    GEnv.Render = &RImplementation;
    GEnv.RenderFactory = &RenderFactoryImpl;
    GEnv.DU = &DUImpl;
    GEnv.UIRender = &UIRenderImpl;
#ifdef DEBUG
    GEnv.DRender = &DebugRenderImpl;
#endif
    xrRender_initconsole();
}

XR_EXPORT pcstr GetModeName()
{
    return "renderer_r2.5";
}

XR_EXPORT bool CheckRendererSupport()
{
    D3DCAPS9 caps;
    CHW _HW;
    _HW.CreateD3D();
    _HW.pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    _HW.DestroyD3D();
    u16 ps_ver_major = u16(u32(u32(caps.PixelShaderVersion) & u32(0xf << 8ul)) >> 8);

    if (ps_ver_major < 3)
        return false;

    return true;
}
}
