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

struct SDescriptorBlock
{
    const uint32_t blockID;
    void* pBuffer;
    uint32_t size;
    uint32_t offset;

    SDescriptorBlock(uint32_t id) : blockID(id), pBuffer(NULL), size(0), offset(~0u) {}
};

namespace DX12
{
    struct Cursor
    {
        Cursor() : m_Cursor(0) {}

        inline void SetCursor(u32 value)          { m_Cursor = value; }
        inline u32 GetCursor() const              { return m_Cursor; }
        inline void IncrementCursor(u32 step = 1) { m_Cursor += step; }
        inline void Reset()                        { m_Cursor = 0; }

        u32 m_Cursor;
    };

    class DescriptorHeap
        : public DeviceObject
        , public Cursor
    {
    public:
        DescriptorHeap(Device* device);
        virtual ~DescriptorHeap();

        bool Init(const D3D12_DESCRIPTOR_HEAP_DESC& desc);

        inline ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const
        {
            return m_pDescriptorHeap;
        }

        inline u32 GetDescriptorSize() const
        {
            return m_DescSize;
        }

        inline u32 GetCapacity() const
        {
            return m_Desc12.NumDescriptors;
        }

        inline CD3DX12_CPU_DESCRIPTOR_HANDLE GetHandleOffsetCPU(INT offset) const
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_HeapStartCPU, m_Cursor + offset, m_DescSize);
        }

        inline CD3DX12_CPU_DESCRIPTOR_HANDLE GetHandleOffsetCPU_R(INT offset) const
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_HeapStartCPU, offset, m_DescSize);
        }

        inline CD3DX12_GPU_DESCRIPTOR_HANDLE GetHandleOffsetGPU(INT offset) const
        {
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_HeapStartGPU, m_Cursor + offset, m_DescSize);
        }

        inline CD3DX12_GPU_DESCRIPTOR_HANDLE GetHandleOffsetGPU_R(INT offset) const
        {
            return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_HeapStartGPU, offset, m_DescSize);
        }

        inline D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPUFromCPU(D3D12_CPU_DESCRIPTOR_HANDLE handle) const
        {
            R_ASSERT2(GetHandleOffsetCPU(0).ptr <= handle.ptr && handle.ptr < GetHandleOffsetCPU(GetCapacity()).ptr, "Out of bounds");
            D3D12_GPU_DESCRIPTOR_HANDLE rebase;
            rebase.ptr = m_HeapStartGPU.ptr + handle.ptr - m_HeapStartCPU.ptr;
            return rebase;
        }

        inline D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPUFromGPU(D3D12_GPU_DESCRIPTOR_HANDLE handle) const
        {
            R_ASSERT2(GetHandleOffsetGPU(0).ptr <= handle.ptr && handle.ptr < GetHandleOffsetGPU(GetCapacity()).ptr, "Out of bounds");
            D3D12_CPU_DESCRIPTOR_HANDLE rebase;
            rebase.ptr = m_HeapStartCPU.ptr + handle.ptr - m_HeapStartGPU.ptr;
            return rebase;
        }

        inline INT GetOffsetFromCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
        {
            return static_cast<INT>((handle.ptr - m_HeapStartCPU.ptr) / m_DescSize);
        }

    private:
        _smart_ptr<ID3D12DescriptorHeap> m_pDescriptorHeap;
        D3D12_DESCRIPTOR_HEAP_DESC m_Desc12;
        D3D12_CPU_DESCRIPTOR_HANDLE m_HeapStartCPU;
        D3D12_GPU_DESCRIPTOR_HANDLE m_HeapStartGPU;
        u32 m_DescSize;
    };

    class DescriptorBlock : public Cursor
    {
    public:
        DescriptorBlock()
            : m_BlockStart(0)
            , m_Capacity(0)
        {}

        DescriptorBlock(DescriptorHeap* pHeap, u32 cursor, u32 capacity)
            : m_pDescriptorHeap(pHeap)
            , m_BlockStart(cursor)
            , m_Capacity(capacity)
        {
            R_ASSERT2(cursor + capacity <= pHeap->GetCapacity(), "Out of bounds");
        }

        DescriptorBlock(const SDescriptorBlock& block);

        inline DescriptorHeap* GetDescriptorHeap() const
        {
            return m_pDescriptorHeap.get();
        }

        inline u32 GetDescriptorSize() const
        {
            return m_pDescriptorHeap->GetDescriptorSize();
        }

        inline u32 GetCapacity() const
        {
            return m_Capacity;
        }

        inline D3D12_CPU_DESCRIPTOR_HANDLE GetHandleOffsetCPU(INT offset) const
        {
            R_ASSERT2((offset < 0) || (m_Cursor + offset < m_Capacity), "Out of bounds");
            return m_pDescriptorHeap->GetHandleOffsetCPU_R(static_cast<INT>(m_BlockStart + m_Cursor) + offset);
        }

        inline D3D12_GPU_DESCRIPTOR_HANDLE GetHandleOffsetGPU(INT offset) const
        {
            R_ASSERT2((offset < 0) || (m_Cursor + offset < m_Capacity), "Out of bounds");
            return m_pDescriptorHeap->GetHandleOffsetGPU_R(static_cast<INT>(m_BlockStart + m_Cursor) + offset);
        }

        inline D3D12_GPU_DESCRIPTOR_HANDLE GetHandleGPUFromCPU(D3D12_CPU_DESCRIPTOR_HANDLE handle) const
        {
            R_ASSERT2(m_pDescriptorHeap->GetHandleOffsetCPU(m_BlockStart).ptr <= handle.ptr && handle.ptr < m_pDescriptorHeap->GetHandleOffsetCPU(m_BlockStart + GetCapacity()).ptr, "Out of bounds");
            return m_pDescriptorHeap->GetHandleGPUFromCPU(handle);
        }

        inline D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPUFromGPU(D3D12_GPU_DESCRIPTOR_HANDLE handle) const
        {
            R_ASSERT2(m_pDescriptorHeap->GetHandleOffsetGPU(m_BlockStart).ptr <= handle.ptr && handle.ptr < m_pDescriptorHeap->GetHandleOffsetGPU(m_BlockStart + GetCapacity()).ptr, "Out of bounds");
            return m_pDescriptorHeap->GetHandleCPUFromGPU(handle);
        }

        inline u32 GetStartOffset() const { return m_BlockStart; }

    private:
        _smart_ptr<DescriptorHeap> m_pDescriptorHeap;
        u32 m_BlockStart;
        u32 m_Capacity;
    };
}
