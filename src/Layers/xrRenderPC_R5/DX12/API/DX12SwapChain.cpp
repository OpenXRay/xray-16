/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.
#include "StdAfx.h"
#include "DX12SwapChain.hpp"

namespace DX12
{
    SwapChain* SwapChain::Create(
        CommandList* commandList,
        IDXGIFactory4* pFactory,
        DXGI_SWAP_CHAIN_DESC* pDesc)
    {
        IDXGISwapChain* dxgiSwapChain = NULL;
        IDXGISwapChain3* dxgiSwapChain3 = NULL;
        ID3D12CommandQueue* commandQueue = commandList->GetD3D12CommandQueue();

	// If discard isn't implemented/supported/fails, try the newer swap-types
#if defined(__dxgi1_4_h__) || defined(__d3d11_x_h__)
        if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL)
        {
            // - flip_sequentially is win 8
            pDesc->SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            pDesc->BufferCount = std::max(2U, pDesc->BufferCount);
        }
#ifdef __dxgi1_4_h__
        else if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_DISCARD)
        {
            // - flip_discard is win 10
            pDesc->SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            pDesc->BufferCount = std::max(2U, pDesc->BufferCount);
        }
#endif
#endif    
        HRESULT hr = pFactory->CreateSwapChain(commandQueue, pDesc, &dxgiSwapChain);

#ifdef __dxgi1_5_h__
        BOOL bAllowTearing = FALSE;
        {
            IDXGIFactory5* pFactory5 = nullptr;
            if (S_OK == pFactory->QueryInterface(IID_PPV_ARGS(&pFactory5)) &&
                S_OK ==
                    pFactory5->CheckFeatureSupport(
                        DXGI_FEATURE_PRESENT_ALLOW_TEARING, &bAllowTearing, sizeof(bAllowTearing)))
            {
            }

            if (bAllowTearing)
            {
                pDesc->Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }
        }
#else
        BOOL bAllowTearing = FALSE;
#endif
        
        if (hr == S_OK && dxgiSwapChain)
        {
            hr = dxgiSwapChain->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain3));
            dxgiSwapChain->Release();

            if (hr == S_OK && dxgiSwapChain3)
            {
                return DX12::PassAddRef(new SwapChain(commandList, dxgiSwapChain3, pDesc));
            }
        }

        return nullptr;
    }

    SwapChain* SwapChain::Create(CommandList* commandList, IDXGIFactory4* pFactory, HWND hWnd,
        const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
        IDXGIOutput* pRestrictToOutput)
    {
        IDXGISwapChain1* dxgiSwapChain1 = NULL;
        IDXGISwapChain3* dxgiSwapChain3 = NULL;
        ID3D12CommandQueue* commandQueue = commandList->GetD3D12CommandQueue();
        
        DXGI_SWAP_CHAIN_DESC1 desc1 = *pDesc;

        // If discard isn't implemented/supported/fails, try the newer swap-types
#if defined(__dxgi1_4_h__) || defined(__d3d11_x_h__)
        if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_DISCARD)
        {
            desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            desc1.BufferCount = std::max(2U, pDesc->BufferCount);
        }
#ifdef __dxgi1_4_h__
        else if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL)
        {
            desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            desc1.BufferCount = std::max(2U, pDesc->BufferCount);   

        }
#endif
#endif
        HRESULT hr = pFactory->CreateSwapChainForHwnd(commandQueue, hWnd, &desc1, pFullscreenDesc, pRestrictToOutput, &dxgiSwapChain1);

#ifdef __dxgi1_5_h__
        BOOL bAllowTearing = FALSE;
        {
            IDXGIFactory5* pFactory5 = nullptr;
            if (S_OK == pFactory->QueryInterface(IID_PPV_ARGS(&pFactory5)) &&
                S_OK ==
                    pFactory5->CheckFeatureSupport(
                        DXGI_FEATURE_PRESENT_ALLOW_TEARING, &bAllowTearing, sizeof(bAllowTearing)))
            {
            }

            if (bAllowTearing)
            {
                desc1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }
        }
#else
        BOOL bAllowTearing = FALSE;
#endif

        if (hr == S_OK && dxgiSwapChain1)
        {
            hr = dxgiSwapChain1->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain3));
            dxgiSwapChain1->Release();

            if (hr == S_OK && dxgiSwapChain3)
            {
                DXGI_SWAP_CHAIN_DESC desc;
                dxgiSwapChain3->GetDesc(&desc);
                if (bAllowTearing)
                {
                    desc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                }
                desc.BufferCount = desc1.BufferCount;
                desc.SwapEffect = desc1.SwapEffect;
                return DX12::PassAddRef(new SwapChain(commandList, dxgiSwapChain3, &desc));
            }
        }

        return nullptr;
    }

    SwapChain* SwapChain::Create(CommandList* commandList, IDXGIFactory4* pFactory, IUnknown* pWindow,
        const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput)
    {
        IDXGISwapChain1* dxgiSwapChain1 = NULL;
        IDXGISwapChain3* dxgiSwapChain3 = NULL;
        ID3D12CommandQueue* commandQueue = commandList->GetD3D12CommandQueue();

        DXGI_SWAP_CHAIN_DESC1 desc1 = *pDesc;
    
        // If discard isn't implemented/supported/fails, try the newer swap-types
#if defined(__dxgi1_4_h__) || defined(__d3d11_x_h__)
        if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_DISCARD)
        {
            desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            desc1.BufferCount = std::max(2U, pDesc->BufferCount);
        }
#ifdef __dxgi1_4_h__
        else if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL)
        {
            desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            desc1.BufferCount = std::max(2U, pDesc->BufferCount);
        }
#endif
#endif

        HRESULT hr = pFactory->CreateSwapChainForCoreWindow(commandQueue, pWindow, &desc1, pRestrictToOutput, &dxgiSwapChain1);

#ifdef __dxgi1_5_h__
        BOOL bAllowTearing = FALSE;
        {
            IDXGIFactory5* pFactory5 = nullptr;
            if (S_OK == pFactory->QueryInterface(IID_PPV_ARGS(&pFactory5)) &&
                S_OK ==
                    pFactory5->CheckFeatureSupport(
                        DXGI_FEATURE_PRESENT_ALLOW_TEARING, &bAllowTearing, sizeof(bAllowTearing)))
            {
            }

            if (bAllowTearing)
            {
                desc1.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }
        }
#else
        BOOL bAllowTearing = FALSE;
#endif

        if (hr == S_OK && dxgiSwapChain1)
        {
            hr = dxgiSwapChain1->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain3));
            dxgiSwapChain1->Release();

            if (hr == S_OK && dxgiSwapChain3)
            {
                DXGI_SWAP_CHAIN_DESC desc;
                dxgiSwapChain3->GetDesc(&desc);
                if (bAllowTearing)
                {
                    desc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
                }
                desc.BufferCount = desc1.BufferCount;
                desc.SwapEffect = desc1.SwapEffect;
                return DX12::PassAddRef(new SwapChain(commandList, dxgiSwapChain3, &desc));
            }
        }

        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    SwapChain::SwapChain(CommandList* commandList, IDXGISwapChain3* dxgiSwapChain, DXGI_SWAP_CHAIN_DESC* pDesc)
        : ReferenceCounted()
        , m_CommandList(commandList)
        , m_AsyncQueue(commandList->GetCommandListPool().GetAsyncCommandQueue())
    {
        if (pDesc)
        {
            m_Desc = *pDesc;
        }
        else
        {
            dxgiSwapChain->GetDesc(&m_Desc);
        }

        m_NativeSwapChain = dxgiSwapChain;
        dxgiSwapChain->Release();
    }

    SwapChain::~SwapChain()
    {
        //m_NativeSwapChain->Release();
        ForfeitBuffers();
    }

    void SwapChain::Present(u32 SyncInterval, u32 Flags)
    {
        m_AsyncQueue.Present(
            m_NativeSwapChain, &m_PresentResult, SyncInterval, Flags, m_Desc, GetCurrentBackBufferIndex());
        m_bChangedBackBufferIndex = true;
    }

    void SwapChain::Present1(u32 SyncInterval, u32 Flags, 
        const DXGI_PRESENT_PARAMETERS* pPresentParameters)
    {
        m_AsyncQueue.Present(
            m_NativeSwapChain, &m_PresentResult, SyncInterval, Flags, m_Desc, GetCurrentBackBufferIndex());
        m_bChangedBackBufferIndex = true;
    }

    HRESULT SwapChain::ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters)
    {
        m_bChangedBackBufferIndex = true;

        return m_NativeSwapChain->ResizeTarget(pNewTargetParameters);
    }

    HRESULT SwapChain::ResizeBuffers(
        UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
    {
        m_bChangedBackBufferIndex = true;

        m_NativeSwapChain->GetDesc(&m_Desc);

        // DXGI ERROR: IDXGISwapChain::ResizeBuffers: Cannot add or remove the DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING flag
        // using ResizeBuffers.
        m_Desc.BufferCount = BufferCount ? BufferCount : m_Desc.BufferCount;
        m_Desc.BufferDesc.Width = Width ? Width : m_Desc.BufferDesc.Width;
        m_Desc.BufferDesc.Height = Height ? Height : m_Desc.BufferDesc.Height;
        m_Desc.BufferDesc.Format = NewFormat != DXGI_FORMAT_UNKNOWN ? NewFormat : m_Desc.BufferDesc.Format;
#ifdef __dxgi1_3_h__
        m_Desc.Flags = (m_Desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) |
            (SwapChainFlags & ~DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING);
#endif
        return m_NativeSwapChain->ResizeBuffers(m_Desc.BufferCount, m_Desc.BufferDesc.Width, m_Desc.BufferDesc.Height,
            m_Desc.BufferDesc.Format, m_Desc.Flags);
    }

    void SwapChain::AcquireBuffers(std::vector<Resource*>&& buffers)
    {
        R_ASSERT2(m_BackBuffers.empty(), "must forfeit buffers before assigning");

        m_BackBuffers = std::move(buffers);
        m_BackBufferViews.reserve(buffers.size());
        for (Resource* buffer : m_BackBuffers)
        {
            m_BackBufferViews.emplace_back();
            m_BackBufferViews.back().Init(*buffer, EVT_RenderTargetView);
        }
    }

    void SwapChain::ForfeitBuffers()
    {       
        m_BackBuffers.clear();
        m_BackBufferViews.clear();
    }
}
