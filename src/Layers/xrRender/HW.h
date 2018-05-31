#pragma once

#include "HWCaps.h"
#include "xrCore/ModuleLookup.hpp"

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

    void CreateDevice(HWND hw, bool move_window);

    void DestroyDevice();

    void Reset(HWND hw);

#ifndef USE_OGL
    void selectResolution(u32& dwWidth, u32& dwHeight, BOOL bWindowed);
    D3DFORMAT selectDepthStencil(D3DFORMAT);
    u32 selectPresentInterval();
    u32 selectGPU();
    u32 selectRefresh(u32 dwWidth, u32 dwHeight, D3DFORMAT fmt);
    BOOL support(D3DFORMAT fmt, DWORD type, DWORD usage);
#endif // !USE_OGL

    void updateWindowProps(HWND hw);
#ifdef DEBUG
#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    void Validate(void) {};
#else //	USE_DX10
    void Validate(void)
    {
        VERIFY(pDevice);
        VERIFY(pD3D);
    };
#endif //	USE_DX10
#else
    void Validate(void){};
#endif

//	Variables section
#if defined(USE_OGL)
    CHW* pDevice;
    CHW* pContext;
    CHW* m_pSwapChain;
    GLuint pBaseRT;
    GLuint pBaseZB;
    GLuint pPP;
    GLuint pFB;
    GLuint pCFB;

    CHWCaps Caps;

    HWND m_hWnd;
    HDC m_hDC;
    HGLRC m_hRC;
#elif defined(USE_DX11)
public:
    IDXGIFactory1*       m_pFactory = nullptr;
    IDXGIAdapter1*       m_pAdapter = nullptr; //	pD3D equivalent
    ID3D11Device*           pDevice = nullptr; //	combine with DX9 pDevice via typedef
    ID3D11DeviceContext*   pContext = nullptr; //	combine with DX9 pDevice via typedef
    IDXGISwapChain*    m_pSwapChain = nullptr;
    ID3D11RenderTargetView* pBaseRT = nullptr; //	combine with DX9 pBaseRT via typedef
    ID3D11DepthStencilView* pBaseZB = nullptr;

    CHWCaps Caps;

    D3D_DRIVER_TYPE m_DriverType;
    DXGI_SWAP_CHAIN_DESC m_ChainDesc; //	DevPP equivalent
    D3D_FEATURE_LEVEL FeatureLevel;
#elif defined(USE_DX10)
public:
    IDXGIFactory1*       m_pFactory = nullptr;
    IDXGIAdapter1*       m_pAdapter = nullptr; //	pD3D equivalent
    ID3D10Device1*         pDevice1 = nullptr; //	combine with DX9 pDevice via typedef
    ID3D10Device*           pDevice = nullptr; //	combine with DX9 pDevice via typedef
    ID3D10Device1*        pContext1 = nullptr; //	combine with DX9 pDevice via typedef
    ID3D10Device*          pContext = nullptr; //	combine with DX9 pDevice via typedef
    IDXGISwapChain*    m_pSwapChain = nullptr;
    ID3D10RenderTargetView* pBaseRT = nullptr; //	combine with DX9 pBaseRT via typedef
    ID3D10DepthStencilView* pBaseZB = nullptr;

    CHWCaps Caps;

    D3D10_DRIVER_TYPE m_DriverType;
    DXGI_SWAP_CHAIN_DESC m_ChainDesc; //	DevPP equivalent
    D3D_FEATURE_LEVEL FeatureLevel;
#else
private:
#ifdef DEBUG
    IDirect3DStateBlock9* dwDebugSB = nullptr;
#endif
    XRay::Module hD3D = nullptr;

public:
    IDirect3D9*           pD3D = nullptr; // D3D
    IDirect3DDevice9*  pDevice = nullptr; // render device
    IDirect3DSurface9* pBaseRT = nullptr;
    IDirect3DSurface9* pBaseZB = nullptr;

    CHWCaps Caps;

    UINT DevAdapter;
    D3DDEVTYPE m_DriverType;
    D3DPRESENT_PARAMETERS DevPP;
#endif //	USE_DX10

#if !defined(_MAYA_EXPORT) && !defined(USE_OGL)
    stats_manager stats_manager;
#endif

#if defined(USE_DX10) || defined(USE_DX11) || defined(USE_OGL)
    void UpdateViews();
#endif
#if defined(USE_DX10) || defined(USE_DX11)
    DXGI_RATIONAL selectRefresh(u32 dwWidth, u32 dwHeight, DXGI_FORMAT fmt);

    virtual void OnAppActivate();
    virtual void OnAppDeactivate();
#endif //	USE_DX10

#ifdef USE_OGL
    // TODO: OGL: Implement this into a compatibility layer?
    void ClearRenderTargetView(GLuint pRenderTargetView, const FLOAT ColorRGBA[4]);
    void ClearDepthStencilView(GLuint pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil);
    HRESULT Present(UINT SyncInterval, UINT Flags);
#endif // USE_OGL

    int maxRefreshRate = 200; //ECO_RENDER add

private:
    bool m_move_window = true;
};

extern ECORE_API CHW HW;
