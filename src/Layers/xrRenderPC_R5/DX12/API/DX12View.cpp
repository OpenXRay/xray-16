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
#include "DX12View.hpp"
#include "DX12Device.hpp"
#include "DX12Resource.hpp"
#include "DX12SwapChain.hpp"

namespace DX12
{
    //---------------------------------------------------------------------------------------------------------------------
    ResourceView::ResourceView()
        : ReferenceCounted()
        , m_pResource(nullptr)
        , m_DescriptorHandle(INVALID_CPU_DESCRIPTOR_HANDLE)
        , m_Type(EVT_Unknown)
        , m_HasDesc(true)
        , m_Size(0)
    {
        // clear before use
        memset(&m_unDSVDesc, 0, sizeof(m_unDSVDesc));
        memset(&m_unRTVDesc, 0, sizeof(m_unRTVDesc));
        memset(&m_unVBVDesc, 0, sizeof(m_unVBVDesc));
        memset(&m_unCBVDesc, 0, sizeof(m_unCBVDesc));
        memset(&m_unSRVDesc, 0, sizeof(m_unSRVDesc));
        memset(&m_unUAVDesc, 0, sizeof(m_unUAVDesc));
    }

    ResourceView::ResourceView(ResourceView&& v)
        : m_pResource(std::move(v.m_pResource))
        , m_DescriptorHandle(std::move(v.m_DescriptorHandle))
        , m_Type(std::move(v.m_Type))
        , m_HasDesc(std::move(v.m_HasDesc))
        , m_Size(std::move(v.m_Size))
    {
        m_unDSVDesc = std::move(v.m_unDSVDesc);
        m_unRTVDesc = std::move(v.m_unRTVDesc);
        m_unVBVDesc = std::move(v.m_unVBVDesc);
        m_unCBVDesc = std::move(v.m_unCBVDesc);
        m_unSRVDesc = std::move(v.m_unSRVDesc);
        m_unUAVDesc = std::move(v.m_unUAVDesc);

        v.m_pResource = nullptr;
        v.m_DescriptorHandle = INVALID_CPU_DESCRIPTOR_HANDLE;
    }

    ResourceView& ResourceView::operator=(ResourceView&& v)
    {
        m_pResource = std::move(v.m_pResource);
        m_DescriptorHandle = std::move(v.m_DescriptorHandle);
        m_Type = std::move(v.m_Type);
        m_HasDesc = std::move(v.m_HasDesc);
        m_Size = std::move(v.m_Size);

        m_unDSVDesc = std::move(v.m_unDSVDesc);
        m_unRTVDesc = std::move(v.m_unRTVDesc);
        m_unVBVDesc = std::move(v.m_unVBVDesc);
        m_unCBVDesc = std::move(v.m_unCBVDesc);
        m_unSRVDesc = std::move(v.m_unSRVDesc);
        m_unUAVDesc = std::move(v.m_unUAVDesc);

        v.m_pResource = nullptr;
        v.m_DescriptorHandle = INVALID_CPU_DESCRIPTOR_HANDLE;

        return *this;
    }

    //---------------------------------------------------------------------------------------------------------------------
    ResourceView::~ResourceView()
    {
    }

    //---------------------------------------------------------------------------------------------------------------------
    //template<class T = ID3D12Resource>
    ID3D12Resource* ResourceView::GetD3D12Resource() const
    {
        return m_pResource->GetD3D12Resource();
    }

    //---------------------------------------------------------------------------------------------------------------------
    bool ResourceView::Init(Resource& resource, EViewType type, UINT64 size)
    {
        DX12_ASSERT(type != EVT_Unknown);
        const D3D12_RESOURCE_DESC& desc = resource.GetDesc();

        m_pResource = &resource;
        m_Type = type;
        m_Size = size;

        switch (type)
        {
        case EVT_VertexBufferView:
            m_unVBVDesc.BufferLocation = resource.GetGPUVirtualAddress();
            m_unVBVDesc.SizeInBytes = static_cast<UINT>(m_Size);
            break;

        case EVT_IndexBufferView:
            m_unIBVDesc.BufferLocation = resource.GetGPUVirtualAddress();
            m_unIBVDesc.SizeInBytes = static_cast<UINT>(m_Size);
            break;

        case EVT_ConstantBufferView:
            // Align to 256 bytes as required by DX12
            m_Size = (m_Size + 255) & ~255;

            m_unCBVDesc.BufferLocation = resource.GetGPUVirtualAddress();
            m_unCBVDesc.SizeInBytes = static_cast<UINT>(m_Size);
            break;

        // The *View descriptions have all the same structure and identical enums
        case EVT_ShaderResourceView:
        case EVT_UnorderedAccessView:
        case EVT_DepthStencilView:
        case EVT_RenderTargetView:
            m_unSRVDesc.Format = desc.Format;

            switch (desc.Dimension)
            {
            case D3D12_RESOURCE_DIMENSION_BUFFER:
                m_unSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                break;
            case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
                m_unSRVDesc.ViewDimension = desc.DepthOrArraySize <= 1 ? D3D12_SRV_DIMENSION_TEXTURE1D : D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                break;
            case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
                m_unSRVDesc.ViewDimension = desc.DepthOrArraySize <= 1 ? (desc.SampleDesc.Count <= 1 ? D3D12_SRV_DIMENSION_TEXTURE2D : D3D12_SRV_DIMENSION_TEXTURE2DMS) : (desc.SampleDesc.Count <= 1 ? D3D12_SRV_DIMENSION_TEXTURE2DARRAY : D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY);
                break;
            case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
                m_unSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                break;

            default:
                DX12_NOT_IMPLEMENTED;
                break;
            }
            break;
        }

        return true;
    }
}
