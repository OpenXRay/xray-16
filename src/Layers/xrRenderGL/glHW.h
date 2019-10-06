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

    std::pair<u32, u32> GetSurfaceSize() const;

    void UpdateViews();

    // TODO: OGL: Implement this into a compatibility layer?
    void ClearRenderTargetView(GLuint pRenderTargetView, const FLOAT ColorRGBA[4]);

    void ClearDepthStencilView(GLuint pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil);

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

    SDL_Window* m_hWnd;
    HDC m_hDC;
    SDL_GLContext m_hRC;
    pcstr AdapterName;
    pcstr OpenGLVersion;
    pcstr ShadingVersion;
    bool ShaderBinarySupported;
};

extern ECORE_API CHW HW;
