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

XR_EXPORT bool CheckRendererSupport()
{
    return xrRender_test_hw() ? true : false;
}
}

SCRIPT_EXPORT(CheckRendererSupport_R4, (),
{
    using namespace luabind;
    module(luaState)
    [
        def("xrRender_test_r4_hw", &CheckRendererSupport)
    ];
});
