#pragma once

#include "Layers/xrRender/HWCaps.h"
#include "xrCore/ModuleLookup.hpp"
#include "SDL.h"
#include "SDL_syswm.h"

#if !defined(_MAYA_EXPORT)
#include "Layers/xrRender/stats_manager.h"
#endif

class CHW
{
public:
    CHW();
    ~CHW();

    void CreateD3D();
    void DestroyD3D();

    void CreateDevice(SDL_Window* sdlWnd);
    void DestroyDevice();

    void Reset();

    BOOL support(D3DFORMAT fmt, DWORD type, DWORD usage);

    std::pair<u32, u32> GetSurfaceSize() const;
    D3DFORMAT GetSurfaceFormat() const;
    void Present();
    DeviceState GetDeviceState();

    void ClearRenderTarget(ID3DRenderTargetView* view, Fcolor color) const;
    [[nodiscard]] int ClearRenderTargetRect(ID3DRenderTargetView* view, Fcolor color, u32 numRects, Irect* rects) const;

    void ClearDepth(ID3DDepthStencilView* view, float depth) const;
    [[nodiscard]] int ClearDepthRect(ID3DDepthStencilView* view, float depth, u32 numRects, Irect* rects) const;

    void ClearStencil(ID3DDepthStencilView* view, u8 stencil) const;

    void ClearDepthStencil(ID3DDepthStencilView* view, float depth, u8 stencil) const;

    void ClearRTAndZB(ID3DRenderTargetView* rt, Fcolor color,
        ID3DDepthStencilView* zb, float depth, u8 stencil) const;

private:
    u32 selectPresentInterval();
    u32 selectGPU();
    D3DFORMAT selectDepthStencil(D3DFORMAT);

public:
    CHWCaps Caps;

    ID3DDevice* pDevice = nullptr; // render device
    ID3DRenderTargetView* pBaseRT = nullptr; // base render target
    ID3DDepthStencilView* pBaseZB = nullptr; // base depth-stencil buffer

    D3D_DRIVER_TYPE m_DriverType;

#ifdef DEBUG
    IDirect3DStateBlock9* dwDebugSB = nullptr;
#endif

    IDirect3D9* pD3D = nullptr; // D3D

    UINT DevAdapter;

#if !defined(_MAYA_EXPORT)
    stats_manager stats_manager;
#endif

private:
    D3DPRESENT_PARAMETERS DevPP;
    XRay::Module hD3D = nullptr;
};

extern ECORE_API CHW HW;
