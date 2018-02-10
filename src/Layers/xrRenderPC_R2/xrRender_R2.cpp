#include "stdafx.h"
#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"

void SetupEnvR2()
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

bool SupportsAdvancedRendering()
{
    D3DCAPS9 caps;
    CHW _HW;
    _HW.CreateD3D();
    _HW.pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    _HW.DestroyD3D();
    u16 ps_ver_major = u16(u32(u32(caps.PixelShaderVersion) & u32(0xf << 8ul)) >> 8);

    if (ps_ver_major < 3)
        return false;
    else
        return true;
}

// This must not be optimized by compiler
static const volatile class GEnvHelper
{
public:
    GEnvHelper()
    {
        GEnv.CheckR2 = SupportsAdvancedRendering;
        GEnv.SetupR2 = SetupEnvR2;
    }
    ~GEnvHelper()
    {
        GEnv.CheckR2 = nullptr;
        GEnv.SetupR2 = nullptr;
    }
} helper;
