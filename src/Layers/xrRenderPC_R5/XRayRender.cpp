// xrRender_R2.cpp : Defines the entry point for the DLL application.
//
#include "pch.h"
#pragma comment(lib,"xrEngine.lib")
//const bchar* GGraphicsAPI[] = { TEXT("bear_render_dx11"),TEXT("bear_render_dx12"),TEXT("bear_render_dx12_1"),TEXT("bear_render_vulkan1_0"),TEXT("bear_render_vulkan1_1") };
XRayRenderFactory BRenderFactory;
XRayDUInterface  BDUInterface;
#ifdef DEBUG
XRayDebugRender BDebugRender;
#endif
extern "C" {
XR_EXPORT void SetupEnv()
{
    GEnv.Render = &GRenderInterface;
    GEnv.RenderFactory = &BRenderFactory;
    GEnv.DU = &BDUInterface;
    GEnv.UIRender = &GUIRender;
#ifdef DEBUG
    GEnv.DRender = &BDebugRender;
#endif
}

XR_EXPORT bool CheckRendererSupport() { return true; }
}
