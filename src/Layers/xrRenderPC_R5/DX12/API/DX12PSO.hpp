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

#include "DX12RootSignature.hpp"
#include <unordered_map>

namespace DX12
{
    class PipelineState : public DeviceObject
    {
    public:
        PipelineState(Device* device) : DeviceObject(device) {}
        ~PipelineState() {}

        bool Init(const RootSignature* rootSignature, ID3D12PipelineState* pipelineState);

        THash GetHash() const
        {
            return m_Hash;
        }

        ID3D12PipelineState* GetD3D12PipelineState() const
        {
            return m_PipelineState;
        }

    private:
        THash m_Hash;
        _smart_ptr<ID3D12PipelineState> m_PipelineState;
        const RootSignature* m_RootSignature;
    };

    class GraphicsPipelineState : public PipelineState
    {
    public:
        GraphicsPipelineState(Device* device) : PipelineState(device) {}
        ~GraphicsPipelineState() {}

        struct InitParams
        {
            const RootSignature* rootSignature;
            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
        };

        bool Init(const InitParams& params);

        inline D3D12_GRAPHICS_PIPELINE_STATE_DESC& GetDesc()
        {
            return m_Desc;
        }

        inline const D3D12_GRAPHICS_PIPELINE_STATE_DESC& GetDesc() const
        {
            return m_Desc;
        }

    private:
        D3D12_GRAPHICS_PIPELINE_STATE_DESC m_Desc;
    };

    class ComputePipelineState : public PipelineState
    {
    public:
        ComputePipelineState(Device* device) : PipelineState(device) {}
        ~ComputePipelineState() {}

        struct InitParams
        {
            const RootSignature* rootSignature;
            D3D12_COMPUTE_PIPELINE_STATE_DESC desc;
        };

        bool Init(const InitParams& params);

        inline D3D12_COMPUTE_PIPELINE_STATE_DESC& GetDesc()
        {
            return m_Desc;
        }

        inline const D3D12_COMPUTE_PIPELINE_STATE_DESC& GetDesc() const
        {
            return m_Desc;
        }

    private:
        D3D12_COMPUTE_PIPELINE_STATE_DESC m_Desc;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class PipelineStateCache
    {
    public:
        PipelineStateCache();
        ~PipelineStateCache();

        bool Init(Device* device);

        GraphicsPipelineState* AcquirePipelineState(const GraphicsPipelineState::InitParams& params);
        ComputePipelineState* AcquirePipelineState(const ComputePipelineState::InitParams& params);

    private:
        Device* m_Device;
        std::unordered_map<THash, _smart_ptr<PipelineState>> m_Cache;
    };
}
