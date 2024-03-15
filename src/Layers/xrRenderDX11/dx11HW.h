#pragma once

#include "xrCore/ModuleLookup.hpp"

#include "Layers/xrRender/HWCaps.h"
#include "Layers/xrRender/stats_manager.h"

#if USE_DX12
#include "dx11ConstantAllocator.h"
#endif

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
    DeviceState GetDeviceState();

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
    ICF ID3DDeviceContext* get_context(u32 context_id)
    {
        VERIFY(context_id < R__NUM_CONTEXTS);
#if DX12_DEFERRED_CONTEXT
        return d3d_contexts_pool[context_id];
#else
        return d3d_contexts_pool[CHW::IMM_CTX_ID];
#endif
    }
#else 
    ICF ID3D11DeviceContext* get_context(u32 context_id)
    {
        VERIFY(context_id < R__NUM_CONTEXTS);
        return d3d_contexts_pool[context_id];
    }
#endif 

#if defined(USE_DX12)
    ICF HRESULT CreateFence(u64& query)
    {
        query = reinterpret_cast<u64>(new UINT64);
        HRESULT hr = query ? S_OK : S_FALSE;
        if (!FAILED(hr))
        {
            IssueFence(query);
        }
        return hr;
    }

    ICF HRESULT ReleaseFence(u64 query)
    {
        HRESULT hr = S_FALSE;
        delete reinterpret_cast<UINT64*>(query);
        hr = S_OK;
        return hr;
    }

    ICF HRESULT IssueFence(u64 query)
    {
        UINT64* handle = reinterpret_cast<UINT64*>(query);
        if (handle)
        {
            auto pDeviceContext = 
                reinterpret_cast<CCryDX12DeviceContext*>(d3d_contexts_pool[CHW::IMM_CTX_ID]);    
            R_ASSERT(pDeviceContext);
            *handle = pDeviceContext->InsertFence();
            return S_OK;
        }

        return S_FALSE;
    }

    ICF HRESULT SyncFence(u64 query, bool block = true)
    { 
        UINT64* handle = reinterpret_cast<UINT64*>(query);
        if (handle)
        {
            auto pDeviceContext = 
                reinterpret_cast<CCryDX12DeviceContext *>(d3d_contexts_pool[CHW::IMM_CTX_ID]);    
            R_ASSERT(pDeviceContext);     
            HRESULT hr = pDeviceContext->TestForFence(*handle);
            if (hr != S_OK)
            {
                if (block)
                {
                    hr = pDeviceContext->WaitForFence(*handle);
                }
            }
            return hr;
        }
        return S_FALSE;
    }
#endif
public:
    static constexpr auto IMM_CTX_ID = R__NUM_PARALLEL_CONTEXTS;

    CHWCaps Caps;

    u32 BackBufferCount = 0;
   
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

    ID3DDeviceContext* d3d_contexts_pool[R__NUM_CONTEXTS]{};
 
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

    ICF u32 GetCurrentBackBufferIndex() const
    {
#if defined(USE_DX11)
        return CurrentBackBuffer;
#else
        return m_pSwapChain->GetCurrentBackBufferIndex();
#endif
    }

#if USE_DX12
    template <class T>
    ICF void* AllocateConstantBuffer(T* buffer)
    {
        DeallocateConstantBuffer(buffer);
       
        if (m_constant_allocator.Allocate(buffer))
        {
            return buffer->GetBufferCPUPtr();
        }
       
        return nullptr;
    }

    template <class T>
    ICF void DeallocateConstantBuffer(T* buffer)
    {
        if (buffer->IsUsedBuffer())
        {
            m_constant_allocator.Free(buffer);
        }
    }
#endif

    using D3DCompileFunc = decltype(&D3DCompile);
    D3DCompileFunc D3DCompile = nullptr;

#if !defined(_MAYA_EXPORT)
    stats_manager stats_manager;
#endif
private:
    DXGI_SWAP_CHAIN_DESC m_ChainDesc; // DevPP equivalent
    bool doPresentTest{};
    XRay::Module hD3DCompiler;
    XRay::Module hDXGI;
    XRay::Module hD3D;

#if defined(USE_DX11)
    u32 CurrentBackBuffer = 0;
#endif

#if USE_DX12
    u64 m_frameQuery[PoolConfig::POOL_FRAME_QUERY_COUNT];
    u32 m_frame_id = 0;
    dx11ConstantBufferAllocator m_constant_allocator;
#endif
};

extern ECORE_API CHW HW;
