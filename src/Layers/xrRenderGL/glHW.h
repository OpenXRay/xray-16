#pragma once

#include "Layers/xrRender/HWCaps.h"
#include "xrCore/ModuleLookup.hpp"
#include "SDL.h"
#include "SDL_syswm.h"

class CHW
{
public:
    CHW();
    ~CHW();

    void CreateDevice(SDL_Window* sdlWnd);
    void DestroyDevice();

    void Reset();

    void SetPrimaryAttributes();

    int  MakeContextCurrent(IRender::RenderContext context) const;

    std::pair<u32, u32> GetSurfaceSize() const;

    void UpdateViews();

    void ClearRenderTarget(GLuint view, Fcolor color) const;
    [[nodiscard]] int ClearRenderTargetRect(GLuint view, Fcolor color, u32 numRects, Irect* rects) const;

    void ClearDepth(GLuint view, float depth) const;
    [[nodiscard]] int ClearDepthRect(GLuint view, float depth, u32 numRects, Irect* rects) const;

    void ClearStencil(GLuint view, u8 stencil) const;

    void ClearDepthStencil(GLuint view, float depth, u8 stencil) const;

    void ClearRTAndZB(GLuint rt, Fcolor color,
        GLuint zb, float depth, u8 stencil) const;

    void Present();
    DeviceState GetDeviceState();

public:
    CHWCaps Caps;

    CHW* pDevice;
    CHW* pContext;
    CHW* m_pSwapChain;
    GLuint pBaseRT;
    GLuint pBaseZB;
    GLuint pPP;
    GLuint pFB;

    SDL_Window* m_window{};
    SDL_Window* m_helper_window{};

    SDL_GLContext m_context{};
    SDL_GLContext m_helper_context{};

    pcstr AdapterName;
    pcstr OpenGLVersion;
    pcstr ShadingVersion;
    bool ShaderBinarySupported;
};

extern ECORE_API CHW HW;
