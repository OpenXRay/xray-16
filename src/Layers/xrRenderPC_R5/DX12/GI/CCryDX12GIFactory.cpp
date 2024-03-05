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
#include "CCryDX12GIFactory.hpp"
#include "CCryDX12SwapChain.hpp"

#include "DX12/Device/CCryDX12Device.hpp"
#include "DX12/Device/CCryDX12DeviceContext.hpp"

#include "DX12/API/DX12SwapChain.hpp"

CCryDX12GIFactory* CCryDX12GIFactory::Create()
{
    _smart_ptr<IDXGIFactory4> dxgiFactory4;
#if DEBUG
    if (S_OK != CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(dxgiFactory4.get_address_of())))
#else
    if (S_OK != CreateDXGIFactory2(0, IID_PPV_ARGS(&pDXGIFactory4)))
#endif
    {
        DX12_ASSERT("Failed to create underlying DXGI factory!");
        return NULL;
    }
    return DX12::PassAddRef(new CCryDX12GIFactory(dxgiFactory4.get()));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12GIFactory::CCryDX12GIFactory(IDXGIFactory4* pDXGIFactory4)
    : Super()
    , m_pDXGIFactory4(pDXGIFactory4)
{
    DX12_FUNC_LOG 
}

CCryDX12GIFactory::~CCryDX12GIFactory()
{
    DX12_FUNC_LOG
}

HRESULT STDMETHODCALLTYPE CCryDX12GIFactory::EnumAdapters(UINT Adapter, _Out_ IDXGIAdapter** ppAdapter)
{
    DX12_FUNC_LOG
    return m_pDXGIFactory4->EnumAdapters(Adapter, ppAdapter);
}

HRESULT STDMETHODCALLTYPE CCryDX12GIFactory::MakeWindowAssociation(HWND WindowHandle, UINT Flags)
{
    DX12_FUNC_LOG
    return m_pDXGIFactory4->MakeWindowAssociation(WindowHandle, Flags);
}

HRESULT STDMETHODCALLTYPE CCryDX12GIFactory::GetWindowAssociation(_Out_ HWND* pWindowHandle)
{
    DX12_FUNC_LOG
    return m_pDXGIFactory4->GetWindowAssociation(pWindowHandle);
}

HRESULT STDMETHODCALLTYPE CCryDX12GIFactory::CreateSwapChain(_In_ IUnknown* pDevice, _In_ DXGI_SWAP_CHAIN_DESC* pDesc, _Out_ IDXGISwapChain** ppSwapChain)
{
    DX12_FUNC_LOG
    * ppSwapChain = CCryDX12SwapChain::Create(static_cast<CCryDX12Device*>(pDevice), m_pDXGIFactory4, pDesc);
    return ppSwapChain ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12GIFactory::CreateSoftwareAdapter(HMODULE Module, _Out_ IDXGIAdapter** ppAdapter)
{
    DX12_FUNC_LOG
    return m_pDXGIFactory4->CreateSoftwareAdapter(Module, ppAdapter);
}

HRESULT STDMETHODCALLTYPE CCryDX12GIFactory::EnumAdapters1(UINT Adapter, _Out_ IDXGIAdapter1** ppAdapter)
{
    DX12_FUNC_LOG
    return m_pDXGIFactory4->EnumAdapters1(Adapter, ppAdapter);
}

BOOL STDMETHODCALLTYPE CCryDX12GIFactory::IsCurrent()
{
    DX12_FUNC_LOG
    return m_pDXGIFactory4->IsCurrent();
}

#pragma region /* IDXGIFactory2 implementation */

HRESULT STDMETHODCALLTYPE CCryDX12GIFactory::CreateSwapChainForHwnd(IUnknown* pDevice, HWND hWnd,
    const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
    IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
{
    DX12_FUNC_LOG
    *ppSwapChain = CCryDX12SwapChain::CreateForHwnd(static_cast<CCryDX12Device*>(pDevice), m_pDXGIFactory4, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput);
    return ppSwapChain ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12GIFactory::CreateSwapChainForCoreWindow(IUnknown* pDevice, IUnknown* pWindow,
    const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
{
    DX12_FUNC_LOG
    *ppSwapChain = CCryDX12SwapChain::CreateForCoreWindow(static_cast<CCryDX12Device*>(pDevice), m_pDXGIFactory4, pWindow, pDesc, pRestrictToOutput);
    return ppSwapChain ? S_OK : E_FAIL;
}
