#pragma once

#include "Layers/xrRender/HWCaps.h"
#include "xrCore/ModuleLookup.hpp"
#include "SDL.h"
#include "SDL_syswm.h"

#if !defined(_MAYA_EXPORT)
#include "Layers/xrRender/stats_manager.h"
#endif

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
    void CreateSwapChain(HWND hwnd);
    bool CreateSwapChain2(HWND hwnd);

    bool ThisInstanceIsGlobal() const;

public:
    void BeginPixEvent(LPCWSTR wszName) const;
    void EndPixEvent() const;

public:
    CHWCaps Caps;

    u32 BackBufferCount{};
    u32 CurrentBackBuffer{};

    ID3DDevice* pDevice = nullptr; // render device

    D3D_DRIVER_TYPE m_DriverType;

    IDXGIFactory1* m_pFactory = nullptr;
    IDXGIAdapter1* m_pAdapter = nullptr; // pD3D equivalent
    ID3DDeviceContext* pContext = nullptr;
    IDXGISwapChain* m_pSwapChain = nullptr;
    D3D_FEATURE_LEVEL FeatureLevel;
    bool Valid = true;
    bool ComputeShadersSupported;
    bool DoublePrecisionFloatShaderOps;
    bool SAD4ShaderInstructions;
    bool ExtendedDoublesShaderInstructions;

    bool DX10Only = false;
#ifdef HAS_DX11_2
    IDXGISwapChain2* m_pSwapChain2 = nullptr;
#endif
#ifdef HAS_DX11_3
    ID3D11Device3* pDevice3 = nullptr;
#endif
    ID3D11DeviceContext1* pContext1 = nullptr;
    ID3DUserDefinedAnnotation* pAnnotation = nullptr;

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
