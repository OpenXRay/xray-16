#include "stdafx.h"
#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"
#include "Layers/xrRender/D3DUtils.h"

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

XR_EXPORT bool CheckRendererSupport()
{
    // XXX: this check should be removed after implementing support for HLSL
    // https://github.com/OpenXRay/xray-16/issues/258
    // Check if shaders are available
    if (!FS.exist("$game_shaders$", RImplementation.getShaderPath()))
    {
        Log("~ No shaders found for OpenGL");
        return false;
    }

    // Check if minimal required OpenGL features are available
    // XXX: implement minimal feature availability check
    return true;
}
} // extern "C"
