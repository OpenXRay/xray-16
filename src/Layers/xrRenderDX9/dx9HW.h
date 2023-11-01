#pragma once

#include "xrCore/ModuleLookup.hpp"

#include "Layers/xrRender/HWCaps.h"
#include "Layers/xrRender/stats_manager.h"

#include <SDL.h>

class CHW
    : public pureAppActivate,
      public pureAppDeactivate
{
public:
    CHW();
    ~CHW();

    void CreateD3D();
    void DestroyD3D();

    void CreateDevice(SDL_Window* sdlWnd);
    void DestroyDevice();

    void Reset();

    void SetPrimaryAttributes(u32& windowFlags);

    BOOL support(D3DFORMAT fmt, u32 type, u32 usage) const;
    static bool GivenGPUIsIntelGMA(u32 id_vendor, u32 id_device);

    std::pair<u32, u32> GetSurfaceSize() const;
    DeviceState GetDeviceState() const;

public:
    void BeginScene();
    void EndScene();
    void Present();

public:
    void OnAppActivate() override;
    void OnAppDeactivate() override;

public:
    void BeginPixEvent(LPCWSTR wszName) const;
    void EndPixEvent() const;

private:
    u32 selectPresentInterval() const;
    u32 selectGPU() const;
    D3DFORMAT selectDepthStencil(D3DFORMAT) const;
    bool ThisInstanceIsGlobal() const;

public:
    static constexpr auto IMM_CTX_ID = 0;

    CHWCaps Caps;

    u32 BackBufferCount{};
    u32 CurrentBackBuffer{};

    ID3DDevice* pDevice = nullptr; // render device

    D3DDEVTYPE m_DriverType;

#ifdef DEBUG
    IDirect3DStateBlock9* dwDebugSB = nullptr;
#endif

    IDirect3D9* pD3D = nullptr; // D3D

    u32 DevAdapter;

    decltype(&D3DPERF_BeginEvent) d3dperf_BeginEvent = nullptr;
    decltype(&D3DPERF_EndEvent) d3dperf_EndEvent = nullptr;

#if !defined(_MAYA_EXPORT)
    stats_manager stats_manager;
#endif

private:
    D3DPRESENT_PARAMETERS DevPP;
    XRay::Module hD3D = nullptr;
};

extern ECORE_API CHW HW;
