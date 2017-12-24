#include "stdafx.h"
#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"

#pragma comment(lib, "xrEngine.lib")

extern "C" void XR_EXPORT SetupEnv()
{
    GEnv.Render = &RImplementation;
    GEnv.RenderFactory = &RenderFactoryImpl;
    GEnv.DU = &DUImpl;
    GEnv.UIRender = &UIRenderImpl;
#ifdef DEBUG
    GEnv.DRender = &DebugRenderImpl;
#endif
    xrRender_initconsole(); // XXX: Xottab_DUTY: move somewhere
}

extern "C" bool XR_EXPORT SupportsDX11Rendering()
{
    return xrRender_test_hw() ? true : false;
    /*
    D3DCAPS9					caps;
    CHW							_HW;
    _HW.CreateD3D				();
    _HW.pD3D->GetDeviceCaps		(D3DADAPTER_DEFAULT,D3DDEVTYPE_HAL,&caps);
    _HW.DestroyD3D				();
    u16		ps_ver_major		= u16 ( u32(u32(caps.PixelShaderVersion)&u32(0xf << 8ul))>>8 );

    if (ps_ver_major<3)
        return false;
    else
        return true;
    */
}
