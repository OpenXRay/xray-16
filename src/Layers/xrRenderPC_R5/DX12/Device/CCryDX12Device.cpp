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
#include "CCryDX12Device.hpp"

#include "CCryDX12DeviceContext.hpp"

#include "DX12/Resource/Misc/CCryDX12Buffer.hpp"
#include "DX12/Resource/Misc/CCryDX12InputLayout.hpp"
#include "DX12/Resource/Misc/CCryDX12Query.hpp"
#include "DX12/Resource/Misc/CCryDX12Shader.hpp"

#include "DX12/Resource/State/CCryDX12BlendState.hpp"
#include "DX12/Resource/State/CCryDX12DepthStencilState.hpp"
#include "DX12/Resource/State/CCryDX12RasterizerState.hpp"
#include "DX12/Resource/State/CCryDX12SamplerState.hpp"

#include "DX12/Resource/Texture/CCryDX12Texture1D.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture2D.hpp"
#include "DX12/Resource/Texture/CCryDX12Texture3D.hpp"

#include "DX12/Resource/View/CCryDX12DepthStencilView.hpp"
#include "DX12/Resource/View/CCryDX12RenderTargetView.hpp"
#include "DX12/Resource/View/CCryDX12ShaderResourceView.hpp"
#include "DX12/Resource/View/CCryDX12UnorderedAccessView.hpp"

CCryDX12Device* CCryDX12Device::Create(IDXGIAdapter* pAdapter, D3D_FEATURE_LEVEL* pFeatureLevel)
{
    _smart_ptr<DX12::Device> device;
    device.attach(DX12::Device::Create(pAdapter, pFeatureLevel));

    if (!device)
    {
        DX12_ERROR("Could not create DX12 Device!");
        return NULL;
    }

    return DX12::PassAddRef(new CCryDX12Device(device));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCryDX12Device::CCryDX12Device(DX12::Device* device)
    : Super()
    , m_pDevice(device)
{
    DX12_FUNC_LOG
    m_pContext.attach(CCryDX12DeviceContext::Create(this, false));
}

CCryDX12Device::~CCryDX12Device()
{
    DX12_FUNC_LOG
}

#pragma region /* ID3D11Device implementation */

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateBuffer(
    _In_  const D3D11_BUFFER_DESC* pDesc,
    _In_opt_  const D3D11_SUBRESOURCE_DATA* pInitialData,
    _Out_opt_  ID3D11Buffer** ppBuffer)
{
    DX12_FUNC_LOG
    
    *ppBuffer = CCryDX12Buffer::Create(this, pDesc, pInitialData);
    return *ppBuffer ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateTexture1D(
    _In_  const D3D11_TEXTURE1D_DESC* pDesc,
    _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize)) const D3D11_SUBRESOURCE_DATA* pInitialData,
    _Out_opt_  ID3D11Texture1D** ppTexture1D)
{
    DX12_FUNC_LOG
    
    * ppTexture1D = CCryDX12Texture1D::Create(this, nullptr, pDesc, pInitialData);
    return *ppTexture1D ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateTexture2D(
    _In_  const D3D11_TEXTURE2D_DESC* pDesc,
    _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize)) const D3D11_SUBRESOURCE_DATA* pInitialData,
    _Out_opt_  ID3D11Texture2D** ppTexture2D)
{
    DX12_FUNC_LOG
    
    * ppTexture2D = CCryDX12Texture2D::Create(this, nullptr, pDesc, pInitialData);
    return *ppTexture2D ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateTexture3D(
    _In_  const D3D11_TEXTURE3D_DESC* pDesc,
    _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))  const D3D11_SUBRESOURCE_DATA* pInitialData,
    _Out_opt_  ID3D11Texture3D** ppTexture3D)
{
    DX12_FUNC_LOG
    
    * ppTexture3D = CCryDX12Texture3D::Create(this, nullptr, pDesc, pInitialData);
    return *ppTexture3D ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateShaderResourceView(
    _In_  ID3D11Resource* pResource,
    _In_opt_  const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
    _Out_opt_  ID3D11ShaderResourceView** ppSRView)
{
    DX12_FUNC_LOG
    
    * ppSRView = NULL;

    D3D11_SHADER_RESOURCE_VIEW_DESC* pCreatedDesc = NULL;
    if (!pDesc)
    {
        D3D11_RESOURCE_DIMENSION type;
        pResource->GetType(&type);

        if (type == D3D11_RESOURCE_DIMENSION_TEXTURE1D)
        {
            D3D11_TEXTURE1D_DESC ptr_desc;
            DX12_EXTRACT_TEXTURE1D(pResource)->GetDesc(&ptr_desc);

            pCreatedDesc = new D3D11_SHADER_RESOURCE_VIEW_DESC{};
            pCreatedDesc->Format = ptr_desc.Format;
            pCreatedDesc->ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
            pCreatedDesc->Texture1D.MipLevels = ptr_desc.MipLevels;
            pCreatedDesc->Texture1D.MostDetailedMip = 0; // ptr_desc->MostDetailedMip;
        }
        else if (type == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        {
            D3D11_TEXTURE2D_DESC ptr_desc;
            DX12_EXTRACT_TEXTURE2D(pResource)->GetDesc(&ptr_desc);

            pCreatedDesc = new D3D11_SHADER_RESOURCE_VIEW_DESC{};
            pCreatedDesc->Format = ptr_desc.Format;

            if (ptr_desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
            {
                pCreatedDesc->ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                pCreatedDesc->TextureCube.MipLevels = ptr_desc.MipLevels;
                pCreatedDesc->TextureCube.MostDetailedMip = 0; // ptr_desc->MostDetailedMip;
            }
            else
            {
                const bool isArray = ptr_desc.ArraySize > 1;
                if (ptr_desc.SampleDesc.Count <= 1)
                {
                    pCreatedDesc->ViewDimension =
                        isArray ? D3D11_SRV_DIMENSION_TEXTURE2DARRAY : D3D11_SRV_DIMENSION_TEXTURE2D;
                    if (isArray)
                    {
                        pCreatedDesc->Texture2DArray.MipLevels = ptr_desc.MipLevels;
                        pCreatedDesc->Texture2DArray.ArraySize = ptr_desc.ArraySize;
                    }
                    else
                    {
                        pCreatedDesc->Texture2D.MipLevels = ptr_desc.MipLevels;
                        pCreatedDesc->Texture2D.MostDetailedMip = 0; // ptr_desc->MostDetailedMip;
                    }
                }
                else
                {
                    pCreatedDesc->ViewDimension =
                        isArray ? D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY : D3D11_SRV_DIMENSION_TEXTURE2DMS;
                    if (isArray)
                    {
                        pCreatedDesc->Texture2DMSArray.ArraySize = ptr_desc.ArraySize;
                    }
                }
            }
        }
        else if (type == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
        {
            D3D11_TEXTURE3D_DESC ptr_desc;
            DX12_EXTRACT_TEXTURE3D(pResource)->GetDesc(&ptr_desc);

            pCreatedDesc = new D3D11_SHADER_RESOURCE_VIEW_DESC{};
            pCreatedDesc->Format = ptr_desc.Format;
            pCreatedDesc->ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            pCreatedDesc->Texture3D.MipLevels = ptr_desc.MipLevels;
            pCreatedDesc->Texture3D.MostDetailedMip = 0; // ptr_desc->MostDetailedMip;
        }
    }

    if (CCryDX12ShaderResourceView* pResult = CCryDX12ShaderResourceView::Create(this, pResource, pDesc ? pDesc : pCreatedDesc))
    {
        auto descriptorHandle = GetDX12Device()->CacheShaderResourceView(&pResult->GetDX12View().GetSRVDesc(), DX12_EXTRACT_D3D12RESOURCE(pResource));
        pResult->GetDX12View().SetDescriptorHandle(descriptorHandle);

        if (INVALID_CPU_DESCRIPTOR_HANDLE == descriptorHandle)
        {
            SAFE_RELEASE(pResult);
        }

        *ppSRView = pResult;
    }

    if (pCreatedDesc)
        delete pCreatedDesc;

    return *ppSRView ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateUnorderedAccessView(
    _In_  ID3D11Resource* pResource,
    _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc,
    _Out_opt_  ID3D11UnorderedAccessView** ppUAView)
{
    DX12_FUNC_LOG
    
    * ppUAView = NULL;

    if (CCryDX12UnorderedAccessView* pResult = CCryDX12UnorderedAccessView::Create(this, pResource, pDesc))
    {
        auto descriptorHandle = GetDX12Device()->CacheUnorderedAccessView(&pResult->GetDX12View().GetUAVDesc(), DX12_EXTRACT_D3D12RESOURCE(pResource));
        pResult->GetDX12View().SetDescriptorHandle(descriptorHandle);

        if (INVALID_CPU_DESCRIPTOR_HANDLE == descriptorHandle)
        {
            SAFE_RELEASE(pResult);
        }

        *ppUAView = pResult;
    }

    return *ppUAView ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateRenderTargetView(
    _In_  ID3D11Resource* pResource,
    _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
    _Out_opt_  ID3D11RenderTargetView** ppRTView)
{
    DX12_FUNC_LOG
    
    * ppRTView = NULL;

    D3D11_RENDER_TARGET_VIEW_DESC* pCreatedDesc = NULL;

    if (!pDesc)
    {
        D3D11_RESOURCE_DIMENSION type;
        pResource->GetType(&type);

        if (type == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        {
            D3D11_TEXTURE2D_DESC ptr_desc;
            DX12_EXTRACT_TEXTURE2D(pResource)->GetDesc(&ptr_desc);

            pCreatedDesc = new D3D11_RENDER_TARGET_VIEW_DESC{};
            pCreatedDesc->Format = ptr_desc.Format;

            const bool isArray = ptr_desc.ArraySize > 1;

            if (ptr_desc.SampleDesc.Count <= 1)
            {
                pCreatedDesc->ViewDimension =
                    isArray ? D3D11_RTV_DIMENSION_TEXTURE2DARRAY : D3D11_RTV_DIMENSION_TEXTURE2D;
                if (isArray)
                {
                    pCreatedDesc->Texture2DArray.ArraySize = ptr_desc.ArraySize;
                }
            }
            else
            {
                pCreatedDesc->ViewDimension =
                    isArray ? D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D11_RTV_DIMENSION_TEXTURE2DMS;
                if (isArray)
                {
                    pCreatedDesc->Texture2DMSArray.ArraySize = ptr_desc.ArraySize;
                }
            }
        }
    }

    if (CCryDX12RenderTargetView* pResult = CCryDX12RenderTargetView::Create(this, pResource, pDesc ? pDesc : pCreatedDesc))
    {
        auto descriptorHandle = GetDX12Device()->CacheRenderTargetView(&pResult->GetDX12View().GetRTVDesc(), DX12_EXTRACT_D3D12RESOURCE(pResource));
        pResult->GetDX12View().SetDescriptorHandle(descriptorHandle);

        if (INVALID_CPU_DESCRIPTOR_HANDLE == descriptorHandle)
        {
            SAFE_RELEASE(pResult);
        }

        *ppRTView = pResult;
    }

    if (pCreatedDesc)
        delete pCreatedDesc;

    return *ppRTView ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateDepthStencilView(
    _In_  ID3D11Resource* pResource,
    _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
    _Out_opt_  ID3D11DepthStencilView** ppDSView)
{
    DX12_FUNC_LOG
    
    * ppDSView = NULL;

    if (CCryDX12DepthStencilView* pResult = CCryDX12DepthStencilView::Create(this, pResource, pDesc))
    {
        auto descriptorHandle = GetDX12Device()->CacheDepthStencilView(&pResult->GetDX12View().GetDSVDesc(), DX12_EXTRACT_D3D12RESOURCE(pResource));
        pResult->GetDX12View().SetDescriptorHandle(descriptorHandle);

        if (INVALID_CPU_DESCRIPTOR_HANDLE == descriptorHandle)
        {
            SAFE_RELEASE(pResult);
        }

        *ppDSView = pResult;
    }

    return *ppDSView ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateInputLayout(
    _In_reads_(NumElements)  const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
    _In_range_(0, D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT)  UINT NumElements,
    _In_  const void* pShaderBytecodeWithInputSignature,
    _In_  SIZE_T BytecodeLength,
    _Out_opt_  ID3D11InputLayout** ppInputLayout)
{
    DX12_FUNC_LOG
    * ppInputLayout = CCryDX12InputLayout::Create(this, pInputElementDescs, NumElements, pShaderBytecodeWithInputSignature, BytecodeLength);
    return *ppInputLayout ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateVertexShader(
    _In_  const void* pShaderBytecode,
    _In_  SIZE_T BytecodeLength,
    _In_opt_  ID3D11ClassLinkage* pClassLinkage,
    _Out_opt_  ID3D11VertexShader** ppVertexShader)
{
    DX12_FUNC_LOG
    * ppVertexShader = reinterpret_cast<ID3D11VertexShader*>(CCryDX12Shader::Create(this, pShaderBytecode, BytecodeLength, pClassLinkage));
    return *ppVertexShader ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateGeometryShader(
    _In_  const void* pShaderBytecode,
    _In_  SIZE_T BytecodeLength,
    _In_opt_  ID3D11ClassLinkage* pClassLinkage,
    _Out_opt_  ID3D11GeometryShader** ppGeometryShader)
{
    DX12_FUNC_LOG
    * ppGeometryShader = reinterpret_cast<ID3D11GeometryShader*>(CCryDX12Shader::Create(this, pShaderBytecode, BytecodeLength, pClassLinkage));
    return *ppGeometryShader ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateGeometryShaderWithStreamOutput(
    _In_  const void* pShaderBytecode,
    _In_  SIZE_T BytecodeLength,
    _In_reads_opt_(NumEntries)  const D3D11_SO_DECLARATION_ENTRY* pSODeclaration,
    _In_range_(0, D3D11_SO_STREAM_COUNT * D3D11_SO_OUTPUT_COMPONENT_COUNT)  UINT NumEntries,
    _In_reads_opt_(NumStrides)  const UINT* pBufferStrides,
    _In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumStrides,
    _In_  UINT RasterizedStream,
    _In_opt_  ID3D11ClassLinkage* pClassLinkage,
    _Out_opt_  ID3D11GeometryShader** ppGeometryShader)
{
    DX12_FUNC_LOG
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreatePixelShader(
    _In_  const void* pShaderBytecode,
    _In_  SIZE_T BytecodeLength,
    _In_opt_  ID3D11ClassLinkage* pClassLinkage,
    _Out_opt_  ID3D11PixelShader** ppPixelShader)
{
    DX12_FUNC_LOG
    * ppPixelShader = reinterpret_cast<ID3D11PixelShader*>(CCryDX12Shader::Create(this, pShaderBytecode, BytecodeLength, pClassLinkage));
    return *ppPixelShader ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateHullShader(
    _In_  const void* pShaderBytecode,
    _In_  SIZE_T BytecodeLength,
    _In_opt_  ID3D11ClassLinkage* pClassLinkage,
    _Out_opt_  ID3D11HullShader** ppHullShader)
{
    DX12_FUNC_LOG
    * ppHullShader = reinterpret_cast<ID3D11HullShader*>(CCryDX12Shader::Create(this, pShaderBytecode, BytecodeLength, pClassLinkage));
    return *ppHullShader ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateDomainShader(
    _In_  const void* pShaderBytecode,
    _In_  SIZE_T BytecodeLength,
    _In_opt_  ID3D11ClassLinkage* pClassLinkage,
    _Out_opt_  ID3D11DomainShader** ppDomainShader)
{
    DX12_FUNC_LOG
    * ppDomainShader = reinterpret_cast<ID3D11DomainShader*>(CCryDX12Shader::Create(this, pShaderBytecode, BytecodeLength, pClassLinkage));
    return *ppDomainShader ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateComputeShader(
    _In_  const void* pShaderBytecode,
    _In_  SIZE_T BytecodeLength,
    _In_opt_  ID3D11ClassLinkage* pClassLinkage,
    _Out_opt_  ID3D11ComputeShader** ppComputeShader)
{
    DX12_FUNC_LOG
    * ppComputeShader = reinterpret_cast<ID3D11ComputeShader*>(CCryDX12Shader::Create(this, pShaderBytecode, BytecodeLength, pClassLinkage));
    return *ppComputeShader ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateClassLinkage(
    _Out_  ID3D11ClassLinkage** ppLinkage)
{
    DX12_FUNC_LOG
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateBlendState(
    _In_  const D3D11_BLEND_DESC* pBlendStateDesc,
    _Out_opt_  ID3D11BlendState** ppBlendState)
{
    DX12_FUNC_LOG
    * ppBlendState = CCryDX12BlendState::Create(pBlendStateDesc);
    return *ppBlendState ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateDepthStencilState(
    _In_  const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
    _Out_opt_  ID3D11DepthStencilState** ppDepthStencilState)
{
    DX12_FUNC_LOG
    * ppDepthStencilState = CCryDX12DepthStencilState::Create(pDepthStencilDesc);
    return *ppDepthStencilState ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateRasterizerState(
    _In_  const D3D11_RASTERIZER_DESC* pRasterizerDesc,
    _Out_opt_  ID3D11RasterizerState** ppRasterizerState)
{
    DX12_FUNC_LOG
    * ppRasterizerState = CCryDX12RasterizerState::Create(pRasterizerDesc);
    return *ppRasterizerState ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateSamplerState(
    _In_  const D3D11_SAMPLER_DESC* pSamplerDesc,
    _Out_opt_  ID3D11SamplerState** ppSamplerState)
{
    DX12_FUNC_LOG
    * ppSamplerState = NULL;

    if (CCryDX12SamplerState* pResult = CCryDX12SamplerState::Create(pSamplerDesc))
    {
        auto descriptorHandle = GetDX12Device()->CacheSampler(&pResult->GetDX12SamplerState().GetSamplerDesc());
        pResult->GetDX12SamplerState().SetDescriptorHandle(descriptorHandle);

        if (INVALID_CPU_DESCRIPTOR_HANDLE == descriptorHandle)
        {
            SAFE_RELEASE(pResult);
        }

        *ppSamplerState = pResult;
    }

    return *ppSamplerState ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateQuery(
    _In_  const D3D11_QUERY_DESC* pQueryDesc,
    _Out_opt_  ID3D11Query** ppQuery)
{
    *ppQuery = CCryDX12Query::Create(GetD3D12Device(), pQueryDesc);
    return *ppQuery ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreatePredicate(
    _In_  const D3D11_QUERY_DESC* pPredicateDesc,
    _Out_opt_  ID3D11Predicate** ppPredicate)
{
    DX12_FUNC_LOG
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateCounter(
    _In_  const D3D11_COUNTER_DESC* pCounterDesc,
    _Out_opt_  ID3D11Counter** ppCounter)
{
    DX12_FUNC_LOG
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateDeferredContext(
    UINT ContextFlags,
    _Out_opt_  ID3D11DeviceContext** ppDeferredContext)
{
    DX12_FUNC_LOG

    if (ppDeferredContext)
    {
        *ppDeferredContext = CCryDX12DeviceContext::Create(this, true);
        return S_OK;
    }
   
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::OpenSharedResource(
    _In_  HANDLE hResource,
    _In_  REFIID ReturnedInterface,
    _Out_opt_  void** ppResource)
{
    DX12_FUNC_LOG
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CheckFormatSupport(
    _In_  DXGI_FORMAT Format,
    _Out_  UINT* pFormatSupport)
{
    DX12_FUNC_LOG
    D3D12_FEATURE_DATA_FORMAT_SUPPORT data;
    data.Format = Format;

    if (S_OK != m_pDevice->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT)))
    {
        return S_FALSE;
    }

    *pFormatSupport = data.Support1;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CheckMultisampleQualityLevels(
    _In_  DXGI_FORMAT Format,
    _In_  UINT SampleCount,
    _Out_  UINT* pNumQualityLevels)
{
    DX12_FUNC_LOG
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE CCryDX12Device::CheckCounterInfo(
    _Out_  D3D11_COUNTER_INFO* pCounterInfo)
{
    DX12_FUNC_LOG
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CheckCounter(
    _In_  const D3D11_COUNTER_DESC* pDesc,
    _Out_  D3D11_COUNTER_TYPE* pType,
    _Out_  UINT* pActiveCounters,
    _Out_writes_opt_(*pNameLength)  LPSTR szName,
    _Inout_opt_  UINT* pNameLength,
    _Out_writes_opt_(*pUnitsLength)  LPSTR szUnits,
    _Inout_opt_  UINT* pUnitsLength,
    _Out_writes_opt_(*pDescriptionLength)  LPSTR szDescription,
    _Inout_opt_  UINT* pDescriptionLength)
{
    DX12_FUNC_LOG
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CheckFeatureSupport(
    D3D11_FEATURE Feature,
    _Out_writes_bytes_(FeatureSupportDataSize)  void* pFeatureSupportData,
    UINT FeatureSupportDataSize)
{
    DX12_FUNC_LOG

    switch (Feature)
    {
    case D3D11_FEATURE_D3D11_OPTIONS2:
    {
        D3D11_FEATURE_DATA_D3D11_OPTIONS2* dx11FeatureDataOptions2 = static_cast<D3D11_FEATURE_DATA_D3D11_OPTIONS2*>(pFeatureSupportData);
        HRESULT result;

        D3D12_FEATURE_DATA_D3D12_OPTIONS dx12FeatureDataOptions;
        result = m_pDevice->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &dx12FeatureDataOptions, sizeof(dx12FeatureDataOptions));
        if (result == S_OK)
        {
            dx11FeatureDataOptions2->PSSpecifiedStencilRefSupported = dx12FeatureDataOptions.PSSpecifiedStencilRefSupported;
            dx11FeatureDataOptions2->TypedUAVLoadAdditionalFormats = dx12FeatureDataOptions.TypedUAVLoadAdditionalFormats;
            dx11FeatureDataOptions2->ROVsSupported = dx12FeatureDataOptions.ROVsSupported;
            dx11FeatureDataOptions2->MapOnDefaultTextures = true; // Assumed true in DX12
            dx11FeatureDataOptions2->StandardSwizzle = dx12FeatureDataOptions.StandardSwizzle64KBSupported;

            switch (dx12FeatureDataOptions.ConservativeRasterizationTier)
            {
            case D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED:
                dx11FeatureDataOptions2->ConservativeRasterizationTier = D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED;
                break;
            case D3D12_CONSERVATIVE_RASTERIZATION_TIER_1:
                dx11FeatureDataOptions2->ConservativeRasterizationTier = D3D11_CONSERVATIVE_RASTERIZATION_TIER_1;
                break;
            case D3D12_CONSERVATIVE_RASTERIZATION_TIER_2:
                dx11FeatureDataOptions2->ConservativeRasterizationTier = D3D11_CONSERVATIVE_RASTERIZATION_TIER_2;
                break;
            case D3D12_CONSERVATIVE_RASTERIZATION_TIER_3:
                dx11FeatureDataOptions2->ConservativeRasterizationTier = D3D11_CONSERVATIVE_RASTERIZATION_TIER_3;
                break;
            }

            switch (dx12FeatureDataOptions.TiledResourcesTier)
            {
            case D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED:
                dx11FeatureDataOptions2->TiledResourcesTier = D3D11_TILED_RESOURCES_NOT_SUPPORTED;
                break;
            case D3D12_TILED_RESOURCES_TIER_1:
                dx11FeatureDataOptions2->TiledResourcesTier = D3D11_TILED_RESOURCES_TIER_1;
                break;
            case D3D12_TILED_RESOURCES_TIER_2:
                dx11FeatureDataOptions2->TiledResourcesTier = D3D11_TILED_RESOURCES_TIER_2;
                break;
            case D3D12_TILED_RESOURCES_TIER_3:
                dx11FeatureDataOptions2->TiledResourcesTier = D3D11_TILED_RESOURCES_TIER_3;
                break;
            }
        }
        else
        {
            return E_INVALIDARG;
        }

        D3D12_FEATURE_DATA_ARCHITECTURE dx12FeatureDataArchitecture;
        dx12FeatureDataArchitecture.NodeIndex = 0;
        result = m_pDevice->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &dx12FeatureDataArchitecture, sizeof(dx12FeatureDataArchitecture));
        if (result == S_OK)
        {
            dx11FeatureDataOptions2->UnifiedMemoryArchitecture = dx12FeatureDataArchitecture.UMA;
        }
        else
        {
            return E_INVALIDARG;
        }

        break;
    }
    default:
        R_ASSERT2(false, "No conversion to DX12 has been written for this feature support check.");
        return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::GetPrivateData(
    _In_  REFGUID guid,
    _Inout_  UINT* pDataSize,
    _Out_writes_bytes_opt_(*pDataSize)  void* pData)
{
    DX12_FUNC_LOG
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::SetPrivateData(
    _In_  REFGUID guid,
    _In_  UINT DataSize,
    _In_reads_bytes_opt_(DataSize)  const void* pData)
{
    DX12_FUNC_LOG
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::SetPrivateDataInterface(
    _In_  REFGUID guid,
    _In_opt_  const IUnknown* pData)
{
    DX12_FUNC_LOG
    return E_NOTIMPL;
}

D3D_FEATURE_LEVEL STDMETHODCALLTYPE CCryDX12Device::GetFeatureLevel()
{
    DX12_FUNC_LOG
    return D3D_FEATURE_LEVEL_11_1;
}

UINT STDMETHODCALLTYPE CCryDX12Device::GetCreationFlags()
{
    DX12_FUNC_LOG
    return 0;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::GetDeviceRemovedReason()
{
    DX12_FUNC_LOG
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE CCryDX12Device::GetImmediateContext(
    _Out_  ID3D11DeviceContext** ppImmediateContext)
{
    DX12_FUNC_LOG
    if (ppImmediateContext)
    {
        *ppImmediateContext = m_pContext;
        (*ppImmediateContext)->AddRef();
    }
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::SetExceptionMode(
    UINT RaiseFlags)
{
    DX12_FUNC_LOG
    return E_NOTIMPL;
}

UINT STDMETHODCALLTYPE CCryDX12Device::GetExceptionMode()
{
    DX12_FUNC_LOG
    return 0;
}

#pragma endregion

#pragma region /* ID3D11Device1 implementation */

void STDMETHODCALLTYPE CCryDX12Device::GetImmediateContext1(
    _Out_  ID3D11DeviceContext1** ppImmediateContext)
{
    DX12_FUNC_LOG

    if (ppImmediateContext)
    {
        *ppImmediateContext = m_pContext;
        (*ppImmediateContext)->AddRef();
    }
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateDeferredContext1(
    UINT ContextFlags,
    /* [annotation] */
    _COM_Outptr_opt_  ID3D11DeviceContext1** ppDeferredContext)
{
    DX12_FUNC_LOG

    if (ppDeferredContext)
    {
        *ppDeferredContext = CCryDX12DeviceContext::Create(this, true);
        return S_OK;
    }
  
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateBlendState1(
    /* [annotation] */
    _In_  const D3D11_BLEND_DESC1* pBlendStateDesc,
    /* [annotation] */
    _COM_Outptr_opt_  ID3D11BlendState1** ppBlendState)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateRasterizerState1(
    /* [annotation] */
    _In_  const D3D11_RASTERIZER_DESC1* pRasterizerDesc,
    /* [annotation] */
    _COM_Outptr_opt_  ID3D11RasterizerState1** ppRasterizerState)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateDeviceContextState(
    UINT Flags,
    /* [annotation] */
    _In_reads_(FeatureLevels)  const D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels,
    UINT SDKVersion,
    REFIID EmulatedInterface,
    /* [annotation] */
    _Out_opt_  D3D_FEATURE_LEVEL* pChosenFeatureLevel,
    /* [annotation] */
    _Out_opt_  ID3DDeviceContextState** ppContextState)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::OpenSharedResource1(
    /* [annotation] */
    _In_  HANDLE hResource,
    /* [annotation] */
    _In_  REFIID returnedInterface,
    /* [annotation] */
    _COM_Outptr_  void** ppResource)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::OpenSharedResourceByName(
    /* [annotation] */
    _In_  LPCWSTR lpName,
    /* [annotation] */
    _In_  DWORD dwDesiredAccess,
    /* [annotation] */
    _In_  REFIID returnedInterface,
    /* [annotation] */
    _COM_Outptr_  void** ppResource)
{
    DX12_FUNC_LOG
        DX12_NOT_IMPLEMENTED
    return E_FAIL;
}

#pragma endregion

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateTexture1D(
    _In_  const D3D11_TEXTURE1D_DESC* pDesc,
    _In_  const FLOAT cClearValue[4],
    _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize)) const D3D11_SUBRESOURCE_DATA* pInitialData,
    _Out_opt_  ID3D11Texture1D** ppTexture1D)
{
    DX12_FUNC_LOG
    * ppTexture1D = CCryDX12Texture1D::Create(this, cClearValue, pDesc, pInitialData);
    return *ppTexture1D ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateTexture2D(
    _In_  const D3D11_TEXTURE2D_DESC* pDesc,
    _In_  const FLOAT cClearValue[4],
    _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize)) const D3D11_SUBRESOURCE_DATA* pInitialData,
    _Out_opt_  ID3D11Texture2D** ppTexture2D)
{
    DX12_FUNC_LOG
    * ppTexture2D = CCryDX12Texture2D::Create(this, cClearValue, pDesc, pInitialData);
    return *ppTexture2D ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateTexture3D(
    _In_  const D3D11_TEXTURE3D_DESC* pDesc,
    _In_  const FLOAT cClearValue[4],
    _In_reads_opt_(_Inexpressible_(pDesc->MipLevels))  const D3D11_SUBRESOURCE_DATA* pInitialData,
    _Out_opt_  ID3D11Texture3D** ppTexture3D)
{
    DX12_FUNC_LOG
    * ppTexture3D = CCryDX12Texture3D::Create(this, cClearValue, pDesc, pInitialData);
    return *ppTexture3D ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::CreateStagingResource(
    _In_  ID3D11Resource* pInputResource,
    _Out_ ID3D11Resource** ppStagingResource,
    _In_  BOOL Upload)
{
    ICryDX12Resource* dx12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pInputResource);
    DX12::Resource& rResource = dx12Resource->GetDX12Resource();

    D3D12_RESOURCE_DESC resourceDesc = rResource.GetDesc();
    UINT64 requiredSize;
    UINT numSubResources = resourceDesc.MipLevels * (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : resourceDesc.DepthOrArraySize);
    GetD3D12Device()->GetCopyableFootprints(&resourceDesc, 0, numSubResources, 0, nullptr, nullptr, nullptr, &requiredSize);

    D3D12_RESOURCE_STATES initialState = Upload ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;
    D3D12_HEAP_TYPE heapType = Upload ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_READBACK;

    CD3DX12_HEAP_PROPERTIES stagingProperty(heapType);
    CD3DX12_RESOURCE_DESC stagingBuffer = CD3DX12_RESOURCE_DESC::Buffer(requiredSize);

    ID3D12Resource* stagingResource = NULL;
    HRESULT result = GetDX12Device()->GetD3D12Device()->CreateCommittedResource(
            &stagingProperty,
            D3D12_HEAP_FLAG_NONE,
            &stagingBuffer,
            initialState,
            nullptr,
            IID_PPV_ARGS(&stagingResource));

    if (result == S_OK && stagingResource != nullptr)
    {
        *ppStagingResource = CCryDX12Buffer::Create(this, stagingResource, initialState);
        stagingResource->Release();

        return S_OK;
    }

    return result;
}

HRESULT STDMETHODCALLTYPE CCryDX12Device::ReleaseStagingResource(
    _In_  ID3D11Resource* pStagingResource)
{
    ICryDX12Resource* dx12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pStagingResource);
    DX12::Resource& rResource = dx12Resource->GetDX12Resource();
    ID3D12Resource* d3d12Resource = rResource.GetD3D12Resource();

    d3d12Resource->AddRef();
    d3d12Resource->SetName(L"StagingResource");
    GetDX12Device()->ReleaseLater(d3d12Resource, rResource.GetCurrentState(), rResource.GetAnnouncedState());

    pStagingResource->Release();
    return S_OK;
}
