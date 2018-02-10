// xrRender_GL.cpp : Defines the entry point for the DLL application.
//
#include "stdafx.h"
#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"

void SetupEnvRGL()
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

bool SupportsOpenGLRendering()
{
    // XXX: do a real check
    return true;
}

// This must not be optimized by compiler
static const volatile class GEnvHelper
{
public:
    GEnvHelper()
    {
        GEnv.CheckRGL = SupportsOpenGLRendering;
        GEnv.SetupRGL = SetupEnvRGL;
    }

    ~GEnvHelper()
    {
        GEnv.SetupRGL = nullptr;
        GEnv.SetupRGL = nullptr;
    }
} helper;
