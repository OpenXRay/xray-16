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
#include "DX12PSO.hpp"
#include "DX12Device.hpp"

namespace DX12
{
    bool PipelineState::Init(const RootSignature* rootSignature, ID3D12PipelineState* pipelineState)
    {
        m_RootSignature = rootSignature;
        m_PipelineState = pipelineState;

        return true;
    }

    bool GraphicsPipelineState::Init(const InitParams& params)
    {
        
        m_Desc = params.desc;
        m_Desc.pRootSignature = params.rootSignature->GetD3D12RootSignature();

        ID3D12PipelineState* pipelineState12 = nullptr;
        HRESULT result = GetDevice()->GetD3D12Device()->CreateGraphicsPipelineState(&m_Desc, IID_PPV_ARGS(&pipelineState12));

        if (result != S_OK)
        {
            DX12_ERROR("Could not create graphics pipeline state!");
            return false;
        }

        PipelineState::Init(params.rootSignature, pipelineState12);
        pipelineState12->Release();

        return true;
    }

    bool ComputePipelineState::Init(const InitParams& params)
    {
        
        m_Desc = params.desc;
        m_Desc.pRootSignature = params.rootSignature->GetD3D12RootSignature();

        ID3D12PipelineState* pipelineState12 = nullptr;
        HRESULT result = GetDevice()->GetD3D12Device()->CreateComputePipelineState(&m_Desc, IID_PPV_ARGS(&pipelineState12));

        if (result != S_OK)
        {
            DX12_ERROR("Could not create graphics pipeline state!");
            return false;
        }

        PipelineState::Init(params.rootSignature, pipelineState12);
        pipelineState12->Release();

        return true;
    }

    PipelineStateCache::PipelineStateCache()
        : m_Device(nullptr)
    {
    }

    PipelineStateCache::~PipelineStateCache()
    {
    }

    bool PipelineStateCache::Init(Device* device)
    {
        m_Device = device;
        return true;
    }

    GraphicsPipelineState* PipelineStateCache::AcquirePipelineState(const GraphicsPipelineState::InitParams& params)
    {
        // LSB cleared marks graphics pipeline states, in case graphics and compute hashes collide
        THash hash = (~1) & ComputeSmallHash<sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC)>(&params.desc);
        auto iter = m_Cache.find(hash);

        if (iter != m_Cache.end())
        {
            return static_cast<GraphicsPipelineState*>(iter->second.get());
        }

        GraphicsPipelineState* result = new GraphicsPipelineState(m_Device);
        if (!result->Init(params))
        {
            DX12_ERROR("Could not create PSO!");
            return nullptr;
        }

        m_Cache[hash] = result;
        return result;
    }

    ComputePipelineState* PipelineStateCache::AcquirePipelineState(const ComputePipelineState::InitParams& params)
    {
        // LSB filled marks compute pipeline states, in case graphics and compute hashes collide
        THash hash = (1) | ComputeSmallHash<sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC)>(&params.desc);
        auto iter = m_Cache.find(hash);

        if (iter != m_Cache.end())
        {
            return static_cast<ComputePipelineState*>(iter->second.get());
        }

        ComputePipelineState* result = new ComputePipelineState(m_Device);
        if (!result->Init(params))
        {
            DX12_ERROR("Could not create PSO!");
            return nullptr;
        }

        m_Cache[hash] = result;
        return result;
    }
}
