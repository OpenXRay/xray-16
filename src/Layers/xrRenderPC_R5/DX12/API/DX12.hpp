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

#include "DX12/Includes/concqueue.hpp"
#include "DX12/Includes/critical_section.h"
#include "DX12/Includes/crc32.h"
#include "DX12/Includes/d3dx12.h"
#include "DX12/Includes/fasthash.inl"
#include "DX12/Includes/smartptr.h"
#include "DX12/Includes/range.h"
#include "DX12/Includes/reference_counted.h"

#include <atomic>

#ifdef DEBUG
#define DX12_ENABLE_DEBUG_LAYER 2 // Recommend 2
#define DX12_DEBUG_BARRIER 0 // Recommend 1
#define DX12_GFX_DEBUG
#define DX12_STATS
#endif

#define DX12_USE_DXC 0 // Shader model 6.0 or 5.1
#define DX12_DEFERRED_CONTEXT 0 // Recommend 0

#ifndef NDEBUG
extern int g_nPrintDX12;

#define DX12_FUNC_LOG do {                              \
        if (g_nPrintDX12)                               \
        { Msg("DX12 function call: %s", __func__); } \
} while (0);

#define DX12_LOG(...) do {  \
if (g_nPrintDX12)           \
  { Msg("DX12 Log: " __VA_ARGS__); } \
} while (0)
#define DX12_ERROR(...) do { Msg("DX12 Error: " __VA_ARGS__); } while (0)
#define DX12_ASSERT(cond, ...) \
  do { if (!(cond)) { DX12_ERROR(__VA_ARGS__); R_ASSERT(0); __debugbreak(); } } while(0)

#else
#define DX12_FUNC_LOG do { } while (0);
#define DX12_LOG(...) do { } while (0)
#define DX12_ERROR(...) do { } while (0)
#define DX12_ASSERT(cond, ...) 
#endif

#define DX12_NOT_IMPLEMENTED R_ASSERT2(0, "Not implemented!");

#ifndef DX12_ARRAY_SIZE
/// Return an array size for static arrays.
#define DX12_ARRAY_SIZE(__a) (sizeof(__a) / sizeof(__a[0]))
#endif

#define DX12_GPU_VIRTUAL_ADDRESS_NULL 0ULL
#define INVALID_CPU_DESCRIPTOR_HANDLE CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT)
#define INVALID_GPU_DESCRIPTOR_HANDLE CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT)

// Define BIT macro for use in enums and bit masks.
#define BIT(x) (1 << (x))
#define BIT64(x) (1ll << (x))
#define TYPED_BIT(type, x) (type(1) << (x))

namespace DX12
{
    class CommandList;
    class CommandListPool;
    class DescriptorHeap;
    class Device;
    class GraphicsPipelineState;
    class PipelineStateCache;
    class Resource;
    class ResourceView;

    template <typename T>
    static T* PassAddRef(T* ptr)
    {
        if (ptr)
        {
            ptr->AddRef();
        }

        return ptr;
    }

    template <typename T>
    static T* PassAddRef(const _smart_ptr<T>& ptr)
    {
        if (ptr)
        {
            ptr.get()->AddRef();
        }

        return ptr.get();
    }

    typedef uint32_t THash;
    template<size_t length>
    inline THash ComputeSmallHash(const void* data, UINT seed = 666)
    {
        return fasthash::fasthash32<length>(data, seed);
    }

    UINT GetDXGIFormatSize(DXGI_FORMAT format);
    D3D12_CLEAR_VALUE GetDXGIFormatClearValue(DXGI_FORMAT format, bool depth);

    inline bool IsDXGIFormatCompressed(DXGI_FORMAT format)
    {
        return
            (format >= DXGI_FORMAT_BC1_TYPELESS  && format <= DXGI_FORMAT_BC5_SNORM) ||
            (format >= DXGI_FORMAT_BC6H_TYPELESS && format <= DXGI_FORMAT_BC7_UNORM_SRGB);
    }

    inline bool IsDXGIFormatCompressed4bpp(DXGI_FORMAT format)
    {
        return
            (format >= DXGI_FORMAT_BC1_TYPELESS  && format <= DXGI_FORMAT_BC1_UNORM_SRGB) ||
            (format >= DXGI_FORMAT_BC4_TYPELESS  && format <= DXGI_FORMAT_BC4_SNORM);
    }

    inline bool IsDXGIFormatCompressed8bpp(DXGI_FORMAT format)
    {
        return
            (format >= DXGI_FORMAT_BC2_TYPELESS  && format <= DXGI_FORMAT_BC3_UNORM_SRGB) ||
            (format >= DXGI_FORMAT_BC5_TYPELESS  && format <= DXGI_FORMAT_BC5_SNORM) ||
            (format >= DXGI_FORMAT_BC6H_TYPELESS && format <= DXGI_FORMAT_BC7_UNORM_SRGB);
    }

    class DeviceObject : public ReferenceCounted
    {
    public:
        inline Device* GetDevice() const
        {
            return m_Device;
        }

        inline bool IsInitialized() const
        {
            return m_IsInitialized;
        }

    protected:
        DeviceObject(Device* device);
        virtual ~DeviceObject();

        DeviceObject(DeviceObject&& r)
            : ReferenceCounted(std::move(r))
            , m_IsInitialized(std::move(r.m_IsInitialized))
            , m_Device(std::move(r.m_Device))
        {}

        DeviceObject& operator=(DeviceObject&& r)
        {
            ReferenceCounted::operator=(std::move(r));

            m_IsInitialized = std::move(r.m_IsInitialized);
            m_Device = std::move(r.m_Device);

            return *this;
        }

        inline void SetDevice(Device* device)
        {
            m_Device = device;
        }

        inline void IsInitialized(bool is)
        {
            m_IsInitialized = is;
        }

    private:
        Device* m_Device;
        bool m_IsInitialized;
    };

    static const UINT CONSTANT_BUFFER_ELEMENT_SIZE = 16U;

    enum CommandMode
    {
        CommandModeCompute = 0,
        CommandModeGraphics,
        CommandModeCount
    };
}
