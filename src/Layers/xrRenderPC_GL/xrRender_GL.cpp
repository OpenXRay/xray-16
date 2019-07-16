// xrRender_GL.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"

extern "C"
{
XR_EXPORT void SetupEnvRGL()
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
    // XXX: do a real check
    return true;
}
}
