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

#pragma once
#ifndef __CCRYDX12GISWAPCHAIN__
#define __CCRYDX12GISWAPCHAIN__

#include "DX12/CCryDX12Object.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture2D.hpp"

#include "DX12/API/DX12SwapChain.hpp"

#include <vector>

class CCryDX12SwapChain
    : public CCryDX12GIObject<IDXGISwapChain3>
{
public:
    DX12_OBJECT(CCryDX12SwapChain, CCryDX12GIObject<IDXGISwapChain3>);

    static CCryDX12SwapChain* Create(CCryDX12Device* pDevice, IDXGIFactory4* factory, DXGI_SWAP_CHAIN_DESC* pDesc);

    virtual ~CCryDX12SwapChain();

    IC CCryDX12Device* GetDevice() const
    {
        return m_Device;
    }

    IC IDXGISwapChain3* GetDXGISwapChain() const
    {
        return m_SwapChain->GetDXGISwapChain();
    }

    IC DX12::SwapChain* GetDX12SwapChain()
    {
        return m_SwapChain;
    }

    #pragma region /* IDXGIDeviceSubObject implementation */

    virtual HRESULT STDMETHODCALLTYPE GetDevice(
        _In_  REFIID riid,
        _Out_  void** ppDevice) final;

    #pragma endregion

    #pragma region /* IDXGISwapChain implementation */

    virtual HRESULT STDMETHODCALLTYPE Present(
        UINT SyncInterval,
        UINT Flags) final;

    virtual HRESULT STDMETHODCALLTYPE GetBuffer(
        UINT Buffer,
        _In_  REFIID riid,
        _Out_  void** ppSurface) final;

    virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(
        BOOL Fullscreen,
        _In_opt_  IDXGIOutput* pTarget) final;

    virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(
        _Out_opt_  BOOL* pFullscreen,
        _Out_opt_  IDXGIOutput** ppTarget) final;

    virtual HRESULT STDMETHODCALLTYPE GetDesc(
        _Out_  DXGI_SWAP_CHAIN_DESC* pDesc) final;

    virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(
        UINT BufferCount,
        UINT Width,
        UINT Height,
        DXGI_FORMAT NewFormat,
        UINT SwapChainFlags) final;

    virtual HRESULT STDMETHODCALLTYPE ResizeTarget(
        _In_  const DXGI_MODE_DESC* pNewTargetParameters) final;

    virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(
        _Out_  IDXGIOutput** ppOutput) final;

    virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(
        _Out_  DXGI_FRAME_STATISTICS* pStats) final;

    virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(
        _Out_  UINT* pLastPresentCount) final;

    #pragma endregion

#pragma region /* IDXGISwapChain1 implementation */

    virtual HRESULT STDMETHODCALLTYPE GetDesc1(
        /* [annotation][out] */
        _Out_ DXGI_SWAP_CHAIN_DESC1* pDesc) final
    {
        return m_SwapChain->GetDXGISwapChain()->GetDesc1(pDesc);
    }

    virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc(
        /* [annotation][out] */
        _Out_ DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc) final
    {
        return m_SwapChain->GetDXGISwapChain()->GetFullscreenDesc(pDesc);
    }

    virtual HRESULT STDMETHODCALLTYPE GetHwnd(
        /* [annotation][out] */
        _Out_ HWND* pHwnd) final
    {
        return m_SwapChain->GetDXGISwapChain()->GetHwnd(pHwnd);
    }

    virtual HRESULT STDMETHODCALLTYPE GetCoreWindow(
        /* [annotation][in] */
        _In_ REFIID refiid,
        /* [annotation][out] */
        _COM_Outptr_ void** ppUnk) final
    {
        return m_SwapChain->GetDXGISwapChain()->GetCoreWindow(refiid, ppUnk);
    }

    virtual HRESULT STDMETHODCALLTYPE Present1(
        /* [in] */ UINT SyncInterval,
        /* [in] */ UINT PresentFlags,
        /* [annotation][in] */
        _In_ const DXGI_PRESENT_PARAMETERS* pPresentParameters) final;

    virtual BOOL STDMETHODCALLTYPE IsTemporaryMonoSupported() final
    {
        return m_SwapChain->GetDXGISwapChain()->IsTemporaryMonoSupported();
    }

    virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput(
        /* [annotation][out] */
        _Out_ IDXGIOutput** ppRestrictToOutput) final
    {
        return m_SwapChain->GetDXGISwapChain()->GetRestrictToOutput(ppRestrictToOutput);
    }

    virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor(
        /* [annotation][in] */
        _In_ const DXGI_RGBA* pColor) final
    {
        return m_SwapChain->GetDXGISwapChain()->SetBackgroundColor(pColor);
    }

    virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor(
        /* [annotation][out] */
        _Out_ DXGI_RGBA* pColor) final
    {
        return m_SwapChain->GetDXGISwapChain()->GetBackgroundColor(pColor);
    }

    virtual HRESULT STDMETHODCALLTYPE SetRotation(
        /* [annotation][in] */
        _In_ DXGI_MODE_ROTATION Rotation) final
    {
        return m_SwapChain->GetDXGISwapChain()->SetRotation(Rotation);
    }

    virtual HRESULT STDMETHODCALLTYPE GetRotation(
        /* [annotation][out] */
        _Out_ DXGI_MODE_ROTATION* pRotation) final
    {
        return m_SwapChain->GetDXGISwapChain()->GetRotation(pRotation);
    }

#pragma endregion

#pragma region /* IDXGISwapChain2 implementation */

    virtual HRESULT STDMETHODCALLTYPE SetSourceSize(UINT Width, UINT Height) final
    {
        return m_SwapChain->GetDXGISwapChain()->SetSourceSize(Width, Height);
    }

    virtual HRESULT STDMETHODCALLTYPE GetSourceSize(
        /* [annotation][out] */
        _Out_ UINT* pWidth,
        /* [annotation][out] */
        _Out_ UINT* pHeight) final
    {
        return m_SwapChain->GetDXGISwapChain()->GetSourceSize(pWidth, pHeight);
    }

    virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency) final
    {
        return m_SwapChain->GetDXGISwapChain()->SetMaximumFrameLatency(MaxLatency);
    }

    virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(
        /* [annotation][out] */
        _Out_ UINT* pMaxLatency) final
    {
        return m_SwapChain->GetDXGISwapChain()->GetMaximumFrameLatency(pMaxLatency);
    }

    virtual HANDLE STDMETHODCALLTYPE GetFrameLatencyWaitableObject() final
    {
        return m_SwapChain->GetDXGISwapChain()->GetFrameLatencyWaitableObject();
    }

    virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform(const DXGI_MATRIX_3X2_F* pMatrix) final
    {
        return m_SwapChain->GetDXGISwapChain()->SetMatrixTransform(pMatrix);
    }

    virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform(
        /* [annotation][out] */
        _Out_ DXGI_MATRIX_3X2_F* pMatrix) final
    {
        return m_SwapChain->GetDXGISwapChain()->GetMatrixTransform(pMatrix);
    }

#pragma endregion

#pragma region /* IDXGISwapChain3 implementation */

    virtual UINT STDMETHODCALLTYPE GetCurrentBackBufferIndex() final;

    virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport(
        /* [annotation][in] */
        _In_ DXGI_COLOR_SPACE_TYPE ColorSpace,
        /* [annotation][out] */
        _Out_ UINT* pColorSpaceSupport) final
    {
        return m_SwapChain->GetDXGISwapChain()->CheckColorSpaceSupport(ColorSpace, pColorSpaceSupport);
    }

    virtual HRESULT STDMETHODCALLTYPE SetColorSpace1(
        /* [annotation][in] */
        _In_ DXGI_COLOR_SPACE_TYPE ColorSpace) final
    {
        return m_SwapChain->GetDXGISwapChain()->SetColorSpace1(ColorSpace);
    }

    virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1(
        /* [annotation][in] */
        _In_ UINT BufferCount,
        /* [annotation][in] */
        _In_ UINT Width,
        /* [annotation][in] */
        _In_ UINT Height,
        /* [annotation][in] */
        _In_ DXGI_FORMAT Format,
        /* [annotation][in] */
        _In_ UINT SwapChainFlags,
        /* [annotation][in] */
        _In_reads_(BufferCount) const UINT* pCreationNodeMask,
        /* [annotation][in] */
        _In_reads_(BufferCount) IUnknown* const* ppPresentQueue) final;

#pragma endregion

protected:
    CCryDX12SwapChain(CCryDX12Device* pDevice, DX12::SwapChain* pSwapChain);

private:
    void AcquireBuffers();
    void ForfeitBuffers();

    CCryDX12Device* m_Device;

    DX12::SwapChain* m_SwapChain;
    std::vector<_smart_ptr<CCryDX12Texture2D>> m_BackBuffers;
};

#endif // __CCRYDX12GISWAPCHAIN__
