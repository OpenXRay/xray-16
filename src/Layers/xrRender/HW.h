#pragma once

#include "HWCaps.h"
#include "xrCore/ModuleLookup.hpp"
#include "SDL.h"
#include "SDL_syswm.h"

#if !defined(_MAYA_EXPORT)
#include "stats_manager.h"
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

#if defined(DEBUG)
    void Validate()
    {
        VERIFY(pDevice);
        VERIFY(pD3D);
    };
#else
    void Validate() {}
#endif

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
    D3DPRESENT_PARAMETERS DevPP;

#if !defined(_MAYA_EXPORT)
    stats_manager stats_manager;
#endif

private:
    XRay::Module hD3D = nullptr;
};

extern ECORE_API CHW HW;
