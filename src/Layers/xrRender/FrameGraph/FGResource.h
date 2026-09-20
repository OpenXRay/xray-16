#pragma once

#include "FGTypes.h"
#include "../RenderContext/RenderContext.h"
#include "../ResourceManager/ResourceHandle.h"

namespace xray::render::framegraph {

struct PassNode;

struct ResourceDesc {
    enum class Type : u8 {
        Texture2D,
        Texture3D,
        TextureCube,
        Texture2DArray,
        Buffer,
        AccelerationStructure,
    };

    Type type = Type::Texture2D;

    u32 width = 0;
    u32 height = 0;
    u32 depth = 1;
    u32 arraySize = 1;
    u32 mipLevels = 1;
    nvrhi::Format format = nvrhi::Format::UNKNOWN;
    u32 sampleCount = 1;

    u64 bufferSize = 0;
    u32 structStride = 0;

    u64 accelerationStructureSize = 0;
    bool isTopLevelAccelerationStructure = false;

    bool isRenderTarget = false;
    bool isDepthStencil = false;
    bool isUAV = false;
    bool allowUAV = false;
    bool isIndirectArgs = false;

    bool isTransient = true;
    bool isImported = false;

    shared_str debugName;

    u64 ComputeMemorySize() const {
        if (type == Type::AccelerationStructure)
            return accelerationStructureSize;
        if (type == Type::Buffer) {
            return bufferSize;
        } else {
            u64 pixelSize = GetFormatBytesPerPixel(format);
            u64 pixels = u64(width) * u64(height) * u64(depth) * u64(arraySize);

            if (mipLevels > 1) {
                pixels += pixels / 3;
            }

            return pixels * pixelSize * sampleCount;
        }
    }

private:
    static u32 GetFormatBytesPerPixel(nvrhi::Format format) {
        switch (format) {
            case nvrhi::Format::RGBA32_FLOAT: return 16;
            case nvrhi::Format::RGBA16_FLOAT: return 8;
            case nvrhi::Format::RGBA8_UNORM: return 4;
            case nvrhi::Format::RG32_FLOAT: return 8;
            case nvrhi::Format::RG16_FLOAT: return 4;
            case nvrhi::Format::R32_FLOAT: return 4;
            case nvrhi::Format::R16_FLOAT: return 2;
            case nvrhi::Format::D24S8: return 4;
            case nvrhi::Format::D32: return 4;
            default: return 4;
        }
    }
};

struct ResourceNode {
    ResourceDesc desc;
    VirtualResourceHandle handle;

    PassNode* lastWriter = nullptr;
    u32 readersHead = INVALID_INDEX;

    u32 firstUsedPass = INVALID_INDEX;
    u32 lastUsedPass = INVALID_INDEX;
    u32 refCount = 0;

    xray::render::resources::TextureHandle resourceTexture;
    xray::render::resources::BufferHandle resourceBuffer;

    nvrhi::TextureHandle nvrhiTexture;
    nvrhi::BufferHandle nvrhiBuffer;
    nvrhi::rt::AccelStructHandle nvrhiAccelStruct;

    ResourceState currentState = ResourceState::Undefined;

    u64 memorySize = 0;

    bool isAllocated = false;
    bool isPersistent = false;

    ResourceNode() = default;

    explicit ResourceNode(const ResourceDesc& _desc)
        : desc(_desc)
        , memorySize(_desc.ComputeMemorySize())
        , isPersistent(!_desc.isTransient)
    {}
};

}
