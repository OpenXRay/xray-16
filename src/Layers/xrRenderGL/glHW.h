#pragma once

#include "Layers/xrRender/HWCaps.h"
#include "xrCore/ModuleLookup.hpp"

#ifdef XR_PLATFORM_WEB
#include <emscripten/html5.h>
#endif

namespace xray::render::RENDER_NAMESPACE
{
class CHW
    : public pureAppActivate,
      public pureAppDeactivate
{
public:
    CHW();
    ~CHW();

    void CreateDevice(SDL_Window* sdlWnd);
    void DestroyDevice();

    void Reset();

    void SetPrimaryAttributes(u32& windowFlags);

    IRender::RenderContext GetCurrentContext() const;
    int  MakeContextCurrent(IRender::RenderContext context) const;

    static std::pair<u32, u32> GetSurfaceSize();
    DeviceState GetDeviceState() const;

public:
    void BeginScene();
    void EndScene();
    void Present();

public:
    void OnAppActivate() override;
    void OnAppDeactivate() override;

private:
    void UpdateViews();
    bool ThisInstanceIsGlobal() const;

public:
    void BeginPixEvent(pcstr name) const;
    void EndPixEvent() const;

public:
    static constexpr auto IMM_CTX_ID = 0;

    CHWCaps Caps;

    u32 BackBufferCount{};
    u32 CurrentBackBuffer{};

    GLuint pFB{};

    SDL_Window* m_window{};

    SDL_GLContext m_context{};

#ifdef XR_PLATFORM_WEB
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE m_webgl{};
    bool BaseVertexDrawSupported{};
#endif

    pcstr AdapterName;
    pcstr OpenGLVersionString;
    pcstr ShadingVersion;
    bool ComputeShadersSupported;
#ifdef XR_PLATFORM_WEB
    u32 TextureUploadUnit{};
#endif
};

struct texture_upload_unit
{
#ifdef XR_PLATFORM_WEB
    const GLenum target;
    explicit texture_upload_unit(GLenum target);
    ~texture_upload_unit();
#else
    explicit texture_upload_unit(GLenum) {}
#endif
};

extern ECORE_API CHW HW;
} // namespace xray::render::RENDER_NAMESPACE
