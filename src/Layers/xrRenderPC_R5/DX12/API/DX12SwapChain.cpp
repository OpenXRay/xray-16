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
        // - flip_discard is win 10
        // - flip_sequentially is win 8
        HRESULT hr = S_OK;

        if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_DISCARD)
        {
            pDesc->SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            pDesc->BufferCount = std::max(2U, pDesc->BufferCount);
            hr = pFactory->CreateSwapChain(commandQueue, pDesc, &dxgiSwapChain);
        }
        else if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL)
        {
            pDesc->SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            pDesc->BufferCount = std::max(2U, pDesc->BufferCount);
            hr = pFactory->CreateSwapChain(commandQueue, pDesc, &dxgiSwapChain);
        }
        else
        {
            hr = pFactory->CreateSwapChain(commandQueue, pDesc, &dxgiSwapChain);
        }

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

        // If discard isn't implemented/supported/fails, try the newer swap-types
        // - flip_discard is win 10
        // - flip_sequentially is win 8
        HRESULT hr = S_OK;
        
        if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_DISCARD)
        {
            DXGI_SWAP_CHAIN_DESC1 desc1 = *pDesc;
            desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            desc1.BufferCount = std::max(2U, pDesc->BufferCount);

            hr = pFactory->CreateSwapChainForHwnd(commandQueue, hWnd, &desc1, pFullscreenDesc, pRestrictToOutput, &dxgiSwapChain1);
        }
        else if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL)
        {
            DXGI_SWAP_CHAIN_DESC1 desc1 = *pDesc;

            desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            desc1.BufferCount = std::max(2U, pDesc->BufferCount);   

            hr = pFactory->CreateSwapChainForHwnd(commandQueue, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, &dxgiSwapChain1);
        }
        else
        {
            hr = pFactory->CreateSwapChainForHwnd(commandQueue, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, &dxgiSwapChain1);
        }

        if (hr == S_OK && dxgiSwapChain1)
        {
            hr = dxgiSwapChain1->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain3));
            dxgiSwapChain1->Release();

            if (hr == S_OK && dxgiSwapChain3)
            {
                DXGI_SWAP_CHAIN_DESC desc;
                dxgiSwapChain3->GetDesc(&desc);
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

        // If discard isn't implemented/supported/fails, try the newer swap-types
        // - flip_discard is win 10
        // - flip_sequentially is win 8
        HRESULT hr = S_OK;

        if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_DISCARD)
        {
            DXGI_SWAP_CHAIN_DESC1 desc1 = *pDesc;

            desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            desc1.BufferCount = std::max(2U, pDesc->BufferCount);
           
            hr = pFactory->CreateSwapChainForCoreWindow(commandQueue, pWindow, pDesc, pRestrictToOutput, &dxgiSwapChain1);
        }
        else if (pDesc->SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL)
        {
            DXGI_SWAP_CHAIN_DESC1 desc1 = *pDesc;

            desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            desc1.BufferCount = std::max(2U, pDesc->BufferCount);
           
            hr = pFactory->CreateSwapChainForCoreWindow(commandQueue, pWindow, pDesc, pRestrictToOutput, &dxgiSwapChain1);
        }
        else
        {
            hr = pFactory->CreateSwapChainForCoreWindow(commandQueue, pWindow, pDesc, pRestrictToOutput, &dxgiSwapChain1);
        }

        if (hr == S_OK && dxgiSwapChain1)
        {
            hr = dxgiSwapChain1->QueryInterface(IID_PPV_ARGS(&dxgiSwapChain3));
            dxgiSwapChain1->Release();

            if (hr == S_OK && dxgiSwapChain3)
            {
                DXGI_SWAP_CHAIN_DESC desc;
                dxgiSwapChain3->GetDesc(&desc);
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

    HRESULT SwapChain::Present(u32 SyncInterval, u32 Flags)
    {
        m_AsyncQueue.Flush();

        HRESULT hr = m_NativeSwapChain->Present(SyncInterval, Flags);
        return hr;
    }

    HRESULT SwapChain::Present1(u32 SyncInterval, u32 Flags, 
        const DXGI_PRESENT_PARAMETERS* pPresentParameters)
    {
        m_AsyncQueue.Flush();

        HRESULT hr = m_NativeSwapChain->Present1(SyncInterval, Flags, pPresentParameters);
        return hr;
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
