#pragma once

#include "HWCaps.h"
#include "xrCore/ModuleLookup.hpp"
#include "SDL.h"
#include "SDL_syswm.h"

#if !defined(_MAYA_EXPORT) && !defined(USE_OGL)
#include "stats_manager.h"
#endif

class CHW
#if defined(USE_DX10) || defined(USE_DX11)
    : public pureAppActivate,
      public pureAppDeactivate
#endif //	USE_DX10
{
    //	Functions section
public:
    CHW();
    ~CHW();

#ifndef USE_OGL
    void CreateD3D();
    void DestroyD3D();
#endif // !USE_OGL

    void CreateDevice(SDL_Window* m_sdlWnd);

    void DestroyDevice();

    void Reset();

#ifndef USE_OGL
    D3DFORMAT selectDepthStencil(D3DFORMAT);
    u32 selectPresentInterval();
    u32 selectGPU();
    BOOL support(D3DFORMAT fmt, DWORD type, DWORD usage);
#endif // !USE_OGL

#if defined(DEBUG) && defined(USE_DX9)
    void Validate()
    {
        VERIFY(pDevice);
        VERIFY(pD3D);
    };
#else
    void Validate() {}
#endif

    // Variables section
public:
    CHWCaps Caps;

#if defined(USE_OGL)
    CHW* pDevice;
    CHW* pContext;
    CHW* m_pSwapChain;
    GLuint pBaseRT;
    GLuint pBaseZB;
    GLuint pPP;
    GLuint pFB;
    GLuint pCFB;

    SDL_Window* m_hWnd;
    HDC m_hDC;
    SDL_GLContext m_hRC;
#else // General DirectX
    ID3DDevice* pDevice = nullptr; // render device
    ID3DRenderTargetView* pBaseRT = nullptr; // base render target
    ID3DDepthStencilView* pBaseZB = nullptr; // base depth-stencil buffer

    D3D_DRIVER_TYPE m_DriverType;
#ifndef USE_DX9
    IDXGIFactory1* m_pFactory = nullptr;
    IDXGIAdapter1* m_pAdapter = nullptr; // pD3D equivalent
    ID3DDeviceContext* pContext = nullptr;
    IDXGISwapChain* m_pSwapChain = nullptr;
    DXGI_SWAP_CHAIN_DESC m_ChainDesc; // DevPP equivalent
    D3D_FEATURE_LEVEL FeatureLevel;
#if defined(USE_DX10)
    ID3D10Device1* pDevice1 = nullptr;
    ID3D10Device1* pContext1 = nullptr;
#endif
#else // USE_DX9
#ifdef DEBUG
    IDirect3DStateBlock9* dwDebugSB = nullptr;
#endif
private:
    XRay::Module hD3D = nullptr;

public:
    IDirect3D9* pD3D = nullptr; // D3D

    UINT DevAdapter;
    D3DPRESENT_PARAMETERS DevPP;
#endif // USE_DX9
#endif // USE_OGL

#if !defined(_MAYA_EXPORT) && !defined(USE_OGL)
    stats_manager stats_manager;
#endif

#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    void UpdateViews();
#endif
#if defined(USE_DX10) || defined(USE_DX11)
    bool CheckFormatSupport(DXGI_FORMAT format, UINT feature) const;
    DXGI_FORMAT SelectFormat(D3D_FORMAT_SUPPORT feature, const DXGI_FORMAT formats[], size_t count) const;
    virtual void OnAppActivate();
    virtual void OnAppDeactivate();
#endif //	USE_DX10

#ifdef USE_OGL
    // TODO: OGL: Implement this into a compatibility layer?
    void ClearRenderTargetView(GLuint pRenderTargetView, const FLOAT ColorRGBA[4]);
    void ClearDepthStencilView(GLuint pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil);
    HRESULT Present(UINT SyncInterval, UINT Flags);
#endif // USE_OGL
};

extern ECORE_API CHW HW;
