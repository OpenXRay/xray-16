#include "stdafx.h"

#include <DirectXTex.h>
#include <wrl.h>

//--------------------------------------------------------------------------------------
// Return the BPP for a particular format
//--------------------------------------------------------------------------------------
IC size_t BitsPerPixel(_In_ DXGI_FORMAT fmt) noexcept
{
    switch (fmt)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT: return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT: return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216: return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
    case DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT:
    case DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:
    case DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
#endif
        return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
    case DXGI_FORMAT_V408:
#endif
#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
    case DXGI_FORMAT_D16_UNORM_S8_UINT:
    case DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X16_TYPELESS_G8_UINT:
#endif
        return 24;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
    case DXGI_FORMAT_P208:
    case DXGI_FORMAT_V208:
#endif
        return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11: return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)
    case DXGI_FORMAT_R4G4_UNORM:
#endif
        return 8;

    case DXGI_FORMAT_R1_UNORM: return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM: return 4;

    case DXGI_FORMAT_UNKNOWN:
    case DXGI_FORMAT_FORCE_UINT:
    default: return 0;
    }
}

IC HRESULT GetSurfaceInfo(_In_ size_t width, _In_ size_t height, _In_ DXGI_FORMAT fmt, _Out_opt_ size_t* outNumBytes,
    _Out_opt_ size_t* outRowBytes, _Out_opt_ size_t* outNumRows) noexcept
{
    uint64_t numBytes = 0;
    uint64_t rowBytes = 0;
    uint64_t numRows = 0;

    bool bc = false;
    bool packed = false;
    bool planar = false;
    size_t bpe = 0;
    switch (fmt)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        bc = true;
        bpe = 8;
        break;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        bc = true;
        bpe = 16;
        break;

    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_YUY2:
        packed = true;
        bpe = 4;
        break;

    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        packed = true;
        bpe = 8;
        break;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
        if ((height % 2) != 0)
        {
            // Requires a height alignment of 2.
            return E_INVALIDARG;
        }
        planar = true;
        bpe = 2;
        break;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

    case DXGI_FORMAT_P208:
        planar = true;
        bpe = 2;
        break;

#endif

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        if ((height % 2) != 0)
        {
            // Requires a height alignment of 2.
            return E_INVALIDARG;
        }
        planar = true;
        bpe = 4;
        break;

#if (defined(_XBOX_ONE) && defined(_TITLE)) || defined(_GAMING_XBOX)

    case DXGI_FORMAT_D16_UNORM_S8_UINT:
    case DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X16_TYPELESS_G8_UINT:
        planar = true;
        bpe = 4;
        break;

#endif

    default: break;
    }

    if (bc)
    {
        uint64_t numBlocksWide = 0;
        if (width > 0)
        {
            numBlocksWide = std::max<uint64_t>(1u, (uint64_t(width) + 3u) / 4u);
        }
        uint64_t numBlocksHigh = 0;
        if (height > 0)
        {
            numBlocksHigh = std::max<uint64_t>(1u, (uint64_t(height) + 3u) / 4u);
        }
        rowBytes = numBlocksWide * bpe;
        numRows = numBlocksHigh;
        numBytes = rowBytes * numBlocksHigh;
    }
    else if (packed)
    {
        rowBytes = ((uint64_t(width) + 1u) >> 1) * bpe;
        numRows = uint64_t(height);
        numBytes = rowBytes * height;
    }
    else if (fmt == DXGI_FORMAT_NV11)
    {
        rowBytes = ((uint64_t(width) + 3u) >> 2) * 4u;
        numRows = uint64_t(height) *
            2u; // Direct3D makes this simplifying assumption, although it is larger than the 4:1:1 data
        numBytes = rowBytes * numRows;
    }
    else if (planar)
    {
        rowBytes = ((uint64_t(width) + 1u) >> 1) * bpe;
        numBytes = (rowBytes * uint64_t(height)) + ((rowBytes * uint64_t(height) + 1u) >> 1);
        numRows = height + ((uint64_t(height) + 1u) >> 1);
    }
    else
    {
        const size_t bpp = BitsPerPixel(fmt);
        if (!bpp)
            return E_INVALIDARG;

        rowBytes = (uint64_t(width) * bpp + 7u) / 8u; // round up to nearest byte
        numRows = uint64_t(height);
        numBytes = rowBytes * height;
    }

#if defined(_M_IX86) || defined(_M_ARM) || defined(_M_HYBRID_X86_ARM64)
    static_assert(sizeof(size_t) == 4, "Not a 32-bit platform!");
    if (numBytes > UINT32_MAX || rowBytes > UINT32_MAX || numRows > UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
#else
    static_assert(sizeof(size_t) == 8, "Not a 64-bit platform!");
#endif

    if (outNumBytes)
    {
        *outNumBytes = static_cast<size_t>(numBytes);
    }
    if (outRowBytes)
    {
        *outRowBytes = static_cast<size_t>(rowBytes);
    }
    if (outNumRows)
    {
        *outNumRows = static_cast<size_t>(numRows);
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
DXGI_FORMAT EnsureNotTypeless(DXGI_FORMAT fmt) noexcept
{
    // Assumes UNORM or FLOAT; doesn't use UINT or SINT
    switch (fmt)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case DXGI_FORMAT_R32G32B32_TYPELESS: return DXGI_FORMAT_R32G32B32_FLOAT;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_UNORM;
    case DXGI_FORMAT_R32G32_TYPELESS: return DXGI_FORMAT_R32G32_FLOAT;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS: return DXGI_FORMAT_R10G10B10A2_UNORM;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_R16G16_TYPELESS: return DXGI_FORMAT_R16G16_UNORM;
    case DXGI_FORMAT_R32_TYPELESS: return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_R8G8_TYPELESS: return DXGI_FORMAT_R8G8_UNORM;
    case DXGI_FORMAT_R16_TYPELESS: return DXGI_FORMAT_R16_UNORM;
    case DXGI_FORMAT_R8_TYPELESS: return DXGI_FORMAT_R8_UNORM;
    case DXGI_FORMAT_BC1_TYPELESS: return DXGI_FORMAT_BC1_UNORM;
    case DXGI_FORMAT_BC2_TYPELESS: return DXGI_FORMAT_BC2_UNORM;
    case DXGI_FORMAT_BC3_TYPELESS: return DXGI_FORMAT_BC3_UNORM;
    case DXGI_FORMAT_BC4_TYPELESS: return DXGI_FORMAT_BC4_UNORM;
    case DXGI_FORMAT_BC5_TYPELESS: return DXGI_FORMAT_BC5_UNORM;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS: return DXGI_FORMAT_B8G8R8X8_UNORM;
    case DXGI_FORMAT_BC7_TYPELESS: return DXGI_FORMAT_BC7_UNORM;
    default: return fmt;
    }
}

//--------------------------------------------------------------------------------------
IC void TransitionResource(_In_ ID3D12GraphicsCommandList* commandList, _In_ ID3D12Resource* resource,
    _In_ D3D12_RESOURCE_STATES stateBefore, _In_ D3D12_RESOURCE_STATES stateAfter) noexcept
{
    assert(commandList != nullptr);
    assert(resource != nullptr);

    if (stateBefore == stateAfter)
        return;

    D3D12_RESOURCE_BARRIER desc = {};
    desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    desc.Transition.pResource = resource;
    desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    desc.Transition.StateBefore = stateBefore;
    desc.Transition.StateAfter = stateAfter;

    commandList->ResourceBarrier(1, &desc);
}

// HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW)
#define HRESULT_E_ARITHMETIC_OVERFLOW static_cast<HRESULT>(0x80070216L)

// HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)
#define HRESULT_E_NOT_SUPPORTED static_cast<HRESULT>(0x80070032L)

HRESULT CaptureTexture(_In_ ID3D12Device* device, _In_ ID3D12CommandQueue* pCommandQ, _In_ ID3D12Resource* pSource,
    _Out_ DirectX::ScratchImage& result, D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET,
    D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_RENDER_TARGET) noexcept
{
    Microsoft::WRL::ComPtr<ID3D12Resource> pStaging = NULL;

    if (!pCommandQ || !pSource)
        return E_INVALIDARG;

    const D3D12_RESOURCE_DESC desc = pSource->GetDesc();

    if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
        return HRESULT_E_NOT_SUPPORTED;

    UINT64 totalResourceSize = 0;
    UINT64 fpRowPitch = 0;
    UINT fpRowCount = 0;

    // From our source texture resource, get the offset and description information for all the subresources up
    // through the one we're trying to map.  Our staging resource will contain a buffer large enough for *all*
    // the subresources, so we need to get the correct offset into this buffer.  If we just get the Layout for
    // the one SubResource, it will come back with an offset of 0, which is incorrect.
    device->GetCopyableFootprints(&desc, 0, 1, 0, nullptr, &fpRowCount, &fpRowPitch, &totalResourceSize);

    // Round up the srcPitch to multiples of 256
    const UINT64 srcPitch = (fpRowPitch + 255) & ~0xFFu;

    if (srcPitch > UINT32_MAX)
        return HRESULT_E_ARITHMETIC_OVERFLOW;

    const UINT numberOfPlanes = D3D12GetFormatPlaneCount(device, desc.Format);
    if (numberOfPlanes != 1)
        return HRESULT_E_NOT_SUPPORTED;

    D3D12_HEAP_PROPERTIES sourceHeapProperties;
    HRESULT hr = pSource->GetHeapProperties(&sourceHeapProperties, nullptr);
    if (FAILED(hr))
        return hr;

    // Create a command allocator
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAlloc;
    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_ID3D12CommandAllocator,
        reinterpret_cast<void**>(commandAlloc.GetAddressOf()));
    if (FAILED(hr))
        return hr;

    // Spin up a new command list
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAlloc.Get(), nullptr,
        IID_ID3D12GraphicsCommandList, reinterpret_cast<void**>(commandList.GetAddressOf()));
    if (FAILED(hr))
        return hr;

    // Create a fence
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_ID3D12Fence, reinterpret_cast<void**>(fence.GetAddressOf()));
    if (FAILED(hr))
        return hr;

    assert((srcPitch & 0xFF) == 0);

    const CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
    const CD3DX12_HEAP_PROPERTIES readBackHeapProperties(D3D12_HEAP_TYPE_READBACK);

    // Readback resources must be buffers
    D3D12_RESOURCE_DESC bufferDesc12 = {};
    bufferDesc12.DepthOrArraySize = 1;
    bufferDesc12.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc12.Flags = D3D12_RESOURCE_FLAG_NONE;
    bufferDesc12.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc12.Height = 1;
    bufferDesc12.Width = srcPitch * desc.Height;
    bufferDesc12.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc12.MipLevels = 1;
    bufferDesc12.SampleDesc.Count = 1;

    Microsoft::WRL::ComPtr<ID3D12Resource> copySource(pSource);
    if (desc.SampleDesc.Count > 1)
    {
        // MSAA content must be resolved before being copied to a staging texture
        auto descCopy = desc;
        descCopy.SampleDesc.Count = 1;
        descCopy.SampleDesc.Quality = 0;
        descCopy.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

        Microsoft::WRL::ComPtr<ID3D12Resource> pTemp;
        hr = device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &descCopy,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_ID3D12Resource,
            reinterpret_cast<void**>(pTemp.GetAddressOf()));
        if (FAILED(hr))
            return hr;

        assert(pTemp);

        const DXGI_FORMAT fmt = EnsureNotTypeless(desc.Format);

        D3D12_FEATURE_DATA_FORMAT_SUPPORT formatInfo = {fmt, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE};
        hr = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatInfo, sizeof(formatInfo));
        if (FAILED(hr))
            return hr;

        if (!(formatInfo.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D))
            return E_FAIL;

        for (UINT item = 0; item < desc.DepthOrArraySize; ++item)
        {
            for (UINT level = 0; level < desc.MipLevels; ++level)
            {
                const UINT index = D3D12CalcSubresource(level, item, 0, desc.MipLevels, desc.DepthOrArraySize);
                commandList->ResolveSubresource(pTemp.Get(), index, pSource, index, fmt);
            }
        }

        copySource = pTemp;
    }

    // Create a staging texture
    hr = device->CreateCommittedResource(&readBackHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc12,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_ID3D12Resource, reinterpret_cast<void**>(pStaging.GetAddressOf()));

    if (FAILED(hr))
        return hr;

    assert(*pStaging.GetAddressOf());

    // Transition the resource if necessary
    TransitionResource(commandList.Get(), pSource, beforeState, D3D12_RESOURCE_STATE_COPY_SOURCE);

    // Get the copy target location
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
    bufferFootprint.Footprint.Width = static_cast<UINT>(desc.Width);
    bufferFootprint.Footprint.Height = desc.Height;
    bufferFootprint.Footprint.Depth = 1;
    bufferFootprint.Footprint.RowPitch = static_cast<UINT>(srcPitch);
    bufferFootprint.Footprint.Format = desc.Format;

    const CD3DX12_TEXTURE_COPY_LOCATION copyDest(*pStaging.GetAddressOf(), bufferFootprint);
    const CD3DX12_TEXTURE_COPY_LOCATION copySrc(copySource.Get(), 0);

    // Copy the texture
    commandList->CopyTextureRegion(&copyDest, 0, 0, 0, &copySrc, nullptr);

    // Transition the resource to the next state
    TransitionResource(commandList.Get(), pSource, D3D12_RESOURCE_STATE_COPY_SOURCE, afterState);

    hr = commandList->Close();
    if (FAILED(hr))
        return hr;

    // Execute the command list
    pCommandQ->ExecuteCommandLists(1, CommandListCast(commandList.GetAddressOf()));

    // Signal the fence
    hr = pCommandQ->Signal(fence.Get(), 1);
    if (FAILED(hr))
        return hr;

    // Block until the copy is complete
    while (fence->GetCompletedValue() < 1)
    {
        SwitchToThread();
    }

    const UINT64 imageSize = srcPitch * UINT64(desc.Height);

    if (imageSize > UINT32_MAX)
        return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

    void* pMappedMemory = nullptr;
    D3D12_RANGE readRange = {0, static_cast<SIZE_T>(imageSize)};
    D3D12_RANGE writeRange = {0, 0};
    hr = pStaging->Map(0, &readRange, &pMappedMemory);
    if (FAILED(hr))
        return hr;

    auto sptr = static_cast<const uint8_t*>(pMappedMemory);
    if (!sptr)
    {
        pStaging->Unmap(0, &writeRange);
        return E_POINTER;
    }

    size_t rowPitch, slicePitch, rowCount;
    hr = GetSurfaceInfo(static_cast<size_t>(desc.Width), desc.Height, desc.Format, &slicePitch, &rowPitch, &rowCount);
    if (FAILED(hr))
        return hr;

    // Setup pixels
    std::unique_ptr<uint8_t[]> pixels(new (std::nothrow) uint8_t[slicePitch]);
    if (!pixels)
        return E_OUTOFMEMORY;

    uint8_t* dptr = pixels.get();

    const size_t msize = std::min<size_t>(rowPitch, size_t(srcPitch));
    for (size_t h = 0; h < rowCount; ++h)
    {
        memcpy(dptr, sptr, msize);
        sptr += srcPitch;
        dptr += rowPitch;
    }

    DirectX::Image img;
    img.format = desc.Format;
    img.width = desc.Width;
    img.height = desc.Height;
    img.rowPitch = rowPitch;
    img.slicePitch = slicePitch;
    img.pixels = pixels.get();

    hr = result.InitializeFromImage(img);
    pStaging->Unmap(0, &writeRange);

    return hr;
}

HRESULT CaptureTextureInDX12(_In_ ID3D11Device* pDevice, _In_ ID3D11DeviceContext* pContext, _In_ ID3D11Resource* pSource,
    _Out_ DirectX::ScratchImage& result)
{
    ICryDX12Resource* dx12Resource = DX12_EXTRACT_ICRYDX12RESOURCE(pSource);
    DX12::Resource& resource = dx12Resource->GetDX12Resource();
   
    CCryDX12Device* dx12Device = reinterpret_cast<CCryDX12Device*>(pDevice);
    CCryDX12DeviceContext* dx12DeviceContext = reinterpret_cast<CCryDX12DeviceContext*>(pContext);
    
    return CaptureTexture(dx12Device->GetD3D12Device(),
        dx12DeviceContext->GetCoreGraphicsCommandList()->GetD3D12CommandQueue(), resource.GetD3D12Resource(), result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void generate_jitter(u32* dest, u32 elem_count)
{
    const int cmax = 8;
    svector<Ivector2, cmax> samples;
    while (samples.size() < elem_count * 2)
    {
        Ivector2 test;
        test.set(::Random.randI(0, 256), ::Random.randI(0, 256));
        BOOL valid = TRUE;
        for (u32 t = 0; t < samples.size(); t++)
        {
            int dist = _abs(test.x - samples[t].x) + _abs(test.y - samples[t].y);
            if (dist < 32)
            {
                valid = FALSE;
                break;
            }
        }
        if (valid)
            samples.push_back(test);
    }
    for (u32 it = 0; it < elem_count; it++, dest++)
        *dest = color_rgba(samples[2 * it].x, samples[2 * it].y, samples[2 * it + 1].y, samples[2 * it + 1].x);
}

void CRenderTarget::build_textures()
{
    // Texture for async sreenshots
    {
        D3D_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(D3D_TEXTURE2D_DESC));

        desc.Width = Device.dwWidth;
        desc.Height = Device.dwHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
        desc.Usage = D3D_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D_CPU_ACCESS_READ;
        desc.MiscFlags = 0;

        R_CHK(HW.pDevice->CreateTexture2D(&desc, NULL, &t_ss_async));
    }
   
    // Build material(s)
    {
        // Surface
        // Create immutable texture.
        // So we need to init data _before_ the creation.
        // Use DXGI_FORMAT_R8G8_UNORM

        u16 tempData[TEX_material_LdotN * TEX_material_LdotH * TEX_material_Count] = {};

        D3D_TEXTURE3D_DESC desc;
        ZeroMemory(&desc, sizeof(D3D_TEXTURE3D_DESC));

        desc.Width = TEX_material_LdotN;
        desc.Height = TEX_material_LdotH;
        desc.Depth = TEX_material_Count;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R8G8_UNORM;
        desc.Usage = D3D_USAGE_IMMUTABLE;
        desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        D3D_SUBRESOURCE_DATA subData;
        ZeroMemory(&subData, sizeof(D3D_SUBRESOURCE_DATA));

        subData.pSysMem = tempData;
        subData.SysMemPitch = desc.Width * 2;
        subData.SysMemSlicePitch = desc.Height * subData.SysMemPitch;

        // Fill it (addr: x=dot(L,N),y=dot(L,H))
        for (u32 slice = 0; slice < TEX_material_Count; slice++)
        {
            for (u32 y = 0; y < TEX_material_LdotH; y++)
            {
                for (u32 x = 0; x < TEX_material_LdotN; x++)
                {
                    u16* p = (u16*)((u8*)(subData.pSysMem) + slice * subData.SysMemSlicePitch +
                        y * subData.SysMemPitch + x * 2);
                    float ld = float(x) / float(TEX_material_LdotN - 1);
                    float ls = float(y) / float(TEX_material_LdotH - 1) + EPS_S;
                    ls *= powf(ld, 1 / 32.f);
                    float fd, fs;

                    switch (slice)
                    {
                    case 0: { // looks like OrenNayar
                        fd = powf(ld, 0.75f); // 0.75
                        fs = powf(ls, 16.f) * .5f;
                    }
                    break;
                    case 1: { // looks like Blinn
                        fd = powf(ld, 0.90f); // 0.90
                        fs = powf(ls, 24.f);
                    }
                    break;
                    case 2: { // looks like Phong
                        fd = ld; // 1.0
                        fs = powf(ls * 1.01f, 128.f);
                    }
                    break;
                    case 3: { // looks like Metal
                        float s0 = _abs(1 - _abs(0.05f * _sin(33.f * ld) + ld - ls));
                        float s1 = _abs(1 - _abs(0.05f * _cos(33.f * ld * ls) + ld - ls));
                        float s2 = _abs(1 - _abs(ld - ls));
                        fd = ld; // 1.0
                        fs = powf(_max(_max(s0, s1), s2), 24.f);
                        fs *= powf(ld, 1 / 7.f);
                    }
                    break;
                    default: fd = fs = 0;
                    }
                    s32 _d = clampr(iFloor(fd * 255.5f), 0, 255);
                    s32 _s = clampr(iFloor(fs * 255.5f), 0, 255);
                    if ((y == (TEX_material_LdotH - 1)) && (x == (TEX_material_LdotN - 1)))
                    {
                        _d = 255;
                        _s = 255;
                    }
                    *p = u16(_s * 256 + _d);
                }
            }
        }

        ID3DTexture3D* t_material_surf = NULL;
        R_CHK(HW.pDevice->CreateTexture3D(&desc, &subData, &t_material_surf));
        t_material = RImplementation.Resources->_CreateTexture(r2_material);
        t_material->surface_set(t_material_surf);
        _RELEASE(t_material_surf);
    }

    // Build noise table
    {
        // Surfaces
        // Use DXGI_FORMAT_R8G8B8A8_SNORM

        static const int sampleSize = 4;

        D3D_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(D3D_TEXTURE2D_DESC));

        desc.Width = TEX_jitter;
        desc.Height = TEX_jitter;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Format = DXGI_FORMAT_R8G8B8A8_SNORM;
        // desc.Usage = D3D_USAGE_IMMUTABLE;
        desc.Usage = D3D_USAGE_DEFAULT;
        desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        u32 tempData[TEX_jitter_count - 1][TEX_jitter * TEX_jitter] = {};

        {
            D3D_SUBRESOURCE_DATA subData[TEX_jitter_count - 1] = {};

            for (int it = 0; it < TEX_jitter_count - 1; it++)
            {
                subData[it].pSysMem = tempData[it];
                subData[it].SysMemPitch = desc.Width * sampleSize;
            }

            // Fill it,
            for (u32 y = 0; y < TEX_jitter; y++)
            {
                for (u32 x = 0; x < TEX_jitter; x++)
                {
                    u32 data[TEX_jitter_count - 1] = {};
                    generate_jitter(data, TEX_jitter_count - 1);
                    for (u32 it = 0; it < TEX_jitter_count - 1; it++)
                    {
                        u32* p = (u32*)((u8*)(subData[it].pSysMem) + y * subData[it].SysMemPitch + x * 4);

                        *p = data[it];
                    }
                }
            }

            ID3DTexture2D* t_noise_surf[TEX_jitter_count - 1] = {};

            for (int it = 0; it < TEX_jitter_count - 1; it++)
            {
                string_path name;
                xr_sprintf(name, "%s%d", r2_jitter, it);
                R_CHK(HW.pDevice->CreateTexture2D(&desc, &subData[it], &t_noise_surf[it]));
                t_noise[it] = RImplementation.Resources->_CreateTexture(name);
                t_noise[it]->surface_set(t_noise_surf[it]);
            }

            for (size_t it = 0; it < TEX_jitter_count - 1; ++it)
            {
                _RELEASE(t_noise_surf[it]);
            }
        }

        float tempDataHBAO[TEX_jitter * TEX_jitter * 4] = {0};

        // generate HBAO jitter texture (last)
        D3D_TEXTURE2D_DESC descHBAO;
        ZeroMemory(&descHBAO, sizeof(D3D_TEXTURE2D_DESC));

        descHBAO.Width = TEX_jitter;
        descHBAO.Height = TEX_jitter;
        descHBAO.MipLevels = 1;
        descHBAO.ArraySize = 1;
        descHBAO.SampleDesc.Count = 1;
        descHBAO.SampleDesc.Quality = 0;
        descHBAO.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        // desc.Usage = D3D_USAGE_IMMUTABLE;
        descHBAO.Usage = D3D_USAGE_DEFAULT;
        descHBAO.BindFlags = D3D_BIND_SHADER_RESOURCE;
        descHBAO.CPUAccessFlags = 0;
        descHBAO.MiscFlags = 0;

        int it = TEX_jitter_count - 1;

        D3D_SUBRESOURCE_DATA subData;
        ZeroMemory(&subData, sizeof(D3D_SUBRESOURCE_DATA));

        subData.pSysMem = tempDataHBAO;
        subData.SysMemPitch = descHBAO.Width * sampleSize * sizeof(float);

        // Fill it,
        for (u32 y = 0; y < TEX_jitter; y++)
        {
            for (u32 x = 0; x < TEX_jitter; x++)
            {
                float numDir = 1.0f;
                switch (ps_r_ssao)
                {
                case 1: numDir = 4.0f; break;
                case 2: numDir = 6.0f; break;
                case 3: numDir = 8.0f; break;
                case 4: numDir = 8.0f; break;
                }
                float angle = 2 * PI * ::Random.randF(0.0f, 1.0f) / numDir;
                float dist = ::Random.randF(0.0f, 1.0f);

                float* p = (float*)((u8*)(subData.pSysMem) + y * subData.SysMemPitch + x * 4 * sizeof(float));
                *p = (float)(_cos(angle));
                *(p + 1) = (float)(_sin(angle));
                *(p + 2) = (float)(dist);
                *(p + 3) = 0;
            }
        }

        ID3DTexture2D* t_noise_surf = NULL;

        string_path name;
        xr_sprintf(name, "%s%d", r2_jitter, it);
        R_CHK(HW.pDevice->CreateTexture2D(&descHBAO, &subData, &t_noise_surf));
        t_noise[it] = RImplementation.Resources->_CreateTexture(name);
        t_noise[it]->surface_set(t_noise_surf);
        _RELEASE(t_noise_surf);

        //	Create noise mipped
        {
            //	Autogen mipmaps
            ID3DBaseTexture* t_noise_surf_mipped = NULL;
            DirectX::ScratchImage mippedNoise = {};
            DirectX::Image img = {
                /*.width      =*/TEX_jitter,
                /*.height     =*/TEX_jitter,
                /*.format     =*/desc.Format,
                /*.rowPitch   =*/desc.Width * sampleSize,
                /*.slicePitch =*/0,
                /*.pixels     =*/(uint8_t*)tempData[0],
            };

            //	Update texture. Generate mips.
            // WIC produces bad texture, non-WIC gives 100% identical texture as from D3DX11FilterTexture
            GenerateMipMaps(img, DirectX::TEX_FILTER_POINT | DirectX::TEX_FILTER_FORCE_NON_WIC, 0, mippedNoise);

            R_CHK(CreateTexture(HW.pDevice, mippedNoise.GetImages(), mippedNoise.GetImageCount(),
                mippedNoise.GetMetadata(), &t_noise_surf_mipped));

            t_noise_mipped = RImplementation.Resources->_CreateTexture(r2_jitter_mipped);
            t_noise_mipped->surface_set(t_noise_surf_mipped);
            _RELEASE(t_noise_surf_mipped);
        }
    }
}
