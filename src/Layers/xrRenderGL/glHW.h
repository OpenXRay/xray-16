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

    void Present();
    DeviceState GetDeviceState();

public:
    CHWCaps Caps;

    u32 BackBufferCount{};
    u32 CurrentBackBuffer{};

    CHW* pDevice;
    GLuint pPP;
    GLuint pFB;

    SDL_Window* m_window{};
    SDL_Window* m_helper_window{};

    SDL_GLContext m_context{};
    SDL_GLContext m_helper_context{};

    pcstr AdapterName;
    pcstr OpenGLVersionString;
    pcstr ShadingVersion;
    std::pair<GLint, GLint> OpenGLVersion;
    bool ShaderBinarySupported;
};

extern ECORE_API CHW HW;
