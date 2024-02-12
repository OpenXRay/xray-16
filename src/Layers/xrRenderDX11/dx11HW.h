#pragma once

#include "xrCore/ModuleLookup.hpp"

#include "Layers/xrRender/HWCaps.h"
#include "Layers/xrRender/stats_manager.h"

#include <SDL.h>

class CHW : public pureAppActivate, public pureAppDeactivate
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

    std::pair<u32, u32> GetSurfaceSize() const;

    bool CheckFormatSupport(DXGI_FORMAT format, u32 feature) const;
    DXGI_FORMAT SelectFormat(D3D_FORMAT_SUPPORT feature, const DXGI_FORMAT formats[], size_t count) const;
    template <size_t count>
    inline DXGI_FORMAT SelectFormat(D3D_FORMAT_SUPPORT feature, const DXGI_FORMAT (&formats)[count]) const
    {
        return SelectFormat(feature, formats, count);
    }
    bool UsingFlipPresentationModel() const;
    DeviceState GetDeviceState() const;

public:
    void BeginScene();
    void EndScene();
    void Present();

public:
    void OnAppActivate() override;
    void OnAppDeactivate() override;

private:
    bool CreateSwapChain(HWND hwnd);

#if defined(USE_DX11)
    bool CreateSwapChainOnDX11_2(HWND hwnd);
#else
    bool CreateSwapChainOnDX12(HWND hwnd);
#endif

    bool ThisInstanceIsGlobal() const;

public:

#if defined(USE_DX12)
    ICF ID3D11DeviceContext1* get_context(u32 context_id)
    {
        VERIFY(context_id < R__NUM_CONTEXTS);
        return d3d_contexts_pool[context_id];
    }
#else 
    ICF ID3DDeviceContext* get_context(u32 context_id)
    {
        VERIFY(context_id < R__NUM_CONTEXTS);
        return d3d_contexts_pool[context_id];
    }
#endif 

public:
    static constexpr auto IMM_CTX_ID = R__NUM_PARALLEL_CONTEXTS;

    CHWCaps Caps;

    u32 BackBufferCount = 0;
#if defined(USE_DX11) 
    u32 CurrentBackBuffer = 0;
#endif
    ID3DDevice* pDevice = nullptr; // render device

    D3D_DRIVER_TYPE m_DriverType;

#if defined(USE_DX12)
    IDXGIFactory4* m_pFactory = nullptr;
    IDXGIAdapter1* m_pAdapter = nullptr; // pD3D equivalent
    IDXGISwapChain3* m_pSwapChain = nullptr;
#else 
    IDXGIFactory1* m_pFactory = nullptr;
    IDXGIAdapter1* m_pAdapter = nullptr; // pD3D equivalent
    IDXGISwapChain* m_pSwapChain = nullptr;
#endif

    D3D_FEATURE_LEVEL FeatureLevel;
    bool Valid = true;
    bool ComputeShadersSupported;
    bool DoublePrecisionFloatShaderOps;
    bool SAD4ShaderInstructions;
    bool ExtendedDoublesShaderInstructions;

#if defined(USE_DX12)
    ID3D11DeviceContext1* d3d_contexts_pool[R__NUM_CONTEXTS]{};
#else
    ID3DDeviceContext* d3d_contexts_pool[R__NUM_CONTEXTS]{};
#endif 

#if defined(USE_DX11)   
    bool DX10Only = false;
#ifdef HAS_DX11_2
    IDXGISwapChain2* m_pSwapChain2 = nullptr;
#endif
#ifdef HAS_DX11_3
    ID3D11Device3* pDevice3 = nullptr;
#endif
    ID3D11DeviceContext1* pContext1 = nullptr;
#endif

    IC u32 GetCurrentBackBufferIndex() const { 
#if defined(USE_DX11)   
        return CurrentBackBuffer;
#else
        return m_pSwapChain->GetCurrentBackBufferIndex();
#endif
    }

    using D3DCompileFunc = decltype(&D3DCompile);
    D3DCompileFunc D3DCompile = nullptr;

#if !defined(_MAYA_EXPORT)
    stats_manager stats_manager;
#endif
private:
    DXGI_SWAP_CHAIN_DESC m_ChainDesc; // DevPP equivalent
    XRay::Module hD3DCompiler;
    XRay::Module hDXGI;
    XRay::Module hD3D;
};

extern ECORE_API CHW HW;
