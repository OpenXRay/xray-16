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

#include "DX12.hpp"
#include <vector>

namespace DX12
{
    class ResourceBarrierCache
    {
    public:
        ResourceBarrierCache()
        {
            m_Barriers.reserve(BarrierCountMax);
            m_TransitionChains.reserve(BarrierCountMax);
            m_TransitionNodes.reserve(BarrierCountMax);
            m_TransitionBarriers.reserve(BarrierCountMax);
        }

        void EnqueueTransition(ID3D12GraphicsCommandList* commandList, const D3D12_RESOURCE_BARRIER_FLAGS flags, const D3D12_RESOURCE_TRANSITION_BARRIER& barrier);
        void EnqueueUAV(ID3D12GraphicsCommandList* commandList, Resource& resource);
        void Flush(ID3D12GraphicsCommandList* commandList);

        inline bool IsFlushNeeded() const
        {
            return m_TransitionBarriers.size() || m_Barriers.size();
        }

    private:
        static const u32 BarrierCountMax = 32;
        static const u8 InvalidNodeIndex = -1;

        struct TransitionNode
        {
            u8 selfIdx;
            u8 nextIdx;
            u8 barrierIdx;
        };

        struct TransitionChainRoot
        {
            ID3D12Resource* resource;
            u8 headIdx;
            u8 tailIdx;
        };

        inline void TryCapacityFlush(ID3D12GraphicsCommandList* commandList)
        {
            if (u32(m_Barriers.size() + m_TransitionBarriers.size()) >= BarrierCountMax)
            {
                Flush(commandList);
            }
        }

        std::vector<TransitionChainRoot> m_TransitionChains;
        std::vector<TransitionNode> m_TransitionNodes;
        std::vector<D3D12_RESOURCE_BARRIER> m_TransitionBarriers;
        std::vector<D3D12_RESOURCE_BARRIER> m_Barriers;
    };
}
