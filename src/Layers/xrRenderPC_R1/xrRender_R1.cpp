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
    return "renderer_r1";
}

XR_EXPORT bool CheckRendererSupport()
{
    CHW _HW;
    _HW.CreateD3D();
    if (_HW.pD3D)
        return true;
    return false;
}
}
