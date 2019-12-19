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
    SDL_Window* gl_test_window = SDL_CreateWindow("TestOpenGLWindow", 0, 0, 1, 1, SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);

    if (!gl_test_window)
    {
        Log("~ Cannot create helper window for OpenGL:", SDL_GetError());
        return false;
    }

    SDL_GLContext gl_test_context = SDL_GL_CreateContext(gl_test_window);
    if (!gl_test_context)
    {
        Log("~ Cannot create OpenGL context:", SDL_GetError());
        return false;
    }

    if (glewInit() != GLEW_OK)
    {
        Log("~ Could not initialize glew.");
        return false;
    }

    if (!glewIsSupported("GL_VERSION_4_1 GL_EXT_multi_draw_arrays"))
    {
        Log("~ GL_VERSION_4_1 or GL_EXT_multi_draw_arrays not supported");
        return false;
    }

    SDL_GL_DeleteContext(gl_test_context);
    SDL_DestroyWindow(gl_test_window);

    return true;
}
} // extern "C"
