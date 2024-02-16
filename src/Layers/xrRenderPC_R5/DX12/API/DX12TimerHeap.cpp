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
#include "DX12TimerHeap.h"
#include "DX12Device.hpp"
#include "DX12CommandList.hpp"

namespace DX12
{
    TimerHeap::TimerHeap(Device& device)
        : m_TimerCountMax{}
        , m_TimestampDownloadBuffer{}
        , m_TimestampHeap{ &device }
    {}

    void TimerHeap::Init(u32 timerCountMax)
    {
        Shutdown();

        D3D12_QUERY_HEAP_DESC m_Desc;
        m_Desc.NodeMask = 1;
        m_Desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        m_Desc.Count = timerCountMax * 2;

        Device& device = *m_TimestampHeap.GetDevice();

        m_TimerCountMax = timerCountMax;
        m_TimestampHeap.Init(m_TimestampHeap.GetDevice(), m_Desc);

        CD3DX12_HEAP_PROPERTIES propertyReadback(D3D12_HEAP_TYPE_READBACK);
        CD3DX12_RESOURCE_DESC buffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT64) * m_TimestampHeap.GetCapacity()); 

        ID3D12Resource* resource = nullptr;
        if (S_OK != device.GetD3D12Device()->CreateCommittedResource(&propertyReadback,
            D3D12_HEAP_FLAG_NONE,
            &buffer,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&resource)))
        {
            DX12_ERROR("Could not create intermediate timestamp download buffer!");
        }

        m_TimestampDownloadBuffer.attach(resource);
        m_Timers.reserve(timerCountMax);
    }

    void TimerHeap::Shutdown()
    {
        m_TimestampHeap.Reset();
        m_TimestampDownloadBuffer.reset();
        m_Timers.clear();
        m_TimerCountMax = 0;
    }

    void TimerHeap::Begin()
    {
        m_Timers.clear();
    }

    void TimerHeap::End(CommandList& commandList)
    {
        commandList.ResolveQueryData(m_TimestampHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, TimerHandle(m_Timers.size()) * 2, m_TimestampDownloadBuffer, 0);
    }

    TimerHandle TimerHeap::BeginTimer(CommandList& commandList, const char* name)
    {
        if (m_Timers.size() < m_TimerCountMax)
        {
            TimerHandle handle = TimerHandle(m_Timers.size());

            m_Timers.emplace_back();

            Timer& timer = m_Timers.back();
            timer.m_Name = name;
            timer.m_Duration = 0;
            timer.m_Timestamp = 0;

            commandList.EndQuery(m_TimestampHeap, D3D12_QUERY_TYPE_TIMESTAMP, handle * 2);
            return handle;
        }

        return TimerHandle(-1);
    }

    void TimerHeap::EndTimer(CommandList& commandList, TimerHandle handle)
    {
        if (handle != TimerHandle(-1))
        {
            commandList.EndQuery(m_TimestampHeap, D3D12_QUERY_TYPE_TIMESTAMP, handle * 2 + 1);
        }
    }

    void TimerHeap::ReadbackTimers()
    {
        u64* timestamps = nullptr;
        const D3D12_RANGE readRange = { 0, sizeof(UINT64) * m_TimestampHeap.GetCapacity() };
        m_TimestampDownloadBuffer->Map(0, &readRange, (void**)&timestamps);

        Device& device = *m_TimestampHeap.GetDevice();

        for (u64 i = 0; i < m_Timers.size(); ++i)
        {
            u64 timestampBegin = device.MakeCpuTimestampMicroseconds(timestamps[i * 2]);
            u64 timestampEnd = device.MakeCpuTimestampMicroseconds(timestamps[i * 2 + 1]);

            m_Timers[i].m_Timestamp = timestampBegin;
            m_Timers[i].m_Duration = u32(timestampEnd - timestampBegin);
        }

        m_TimestampDownloadBuffer->Unmap(0, nullptr);
    }
}
