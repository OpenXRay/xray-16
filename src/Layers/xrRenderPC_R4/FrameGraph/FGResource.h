// xrRender/FrameGraph/FGResource.h
#pragma once

#include "FGTypes.h"
#include "../RenderContext/RenderContext.h"

namespace xray::render::framegraph {

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  RESOURCE DESCRIPTION (LOGICAL)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

struct ResourceDesc {
    enum class Type : u8 {
        Texture2D,
        Texture3D,
        TextureCube,
        Texture2DArray,
        Buffer,
    };

    Type type = Type::Texture2D;

    // Texture properties
    u32 width = 0;
    u32 height = 0;
    u32 depth = 1;           // For 3D textures
    u32 arraySize = 1;       // For arrays/cubemaps
    u32 mipLevels = 1;
    nvrhi::Format format = nvrhi::Format::UNKNOWN;
    u32 sampleCount = 1;     // MSAA sample count

    // Buffer properties
    u64 bufferSize = 0;
    u32 structStride = 0;    // For structured buffers

    // Usage flags
    bool isRenderTarget = false;
    bool isDepthStencil = false;
    bool isUAV = false;
    bool allowUAV = false;   // Can be bound as UAV

    // Memory management hints
    bool isTransient = true; // Can be aliased/destroyed
    bool isImported = false; // External resource

    shared_str debugName;

    // Compute memory size
    u64 ComputeMemorySize() const {
        if (type == Type::Buffer) {
            return bufferSize;
        } else {
            // Texture memory estimation
            u64 pixelSize = GetFormatBytesPerPixel(format);
            u64 pixels = width * height * depth * arraySize;

            // Account for mip chain (~33% more)
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
            // Add more as needed
            default: return 4; // Conservative estimate
        }
    }
};

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  RESOURCE NODE (INTERNAL STATE)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

struct ResourceNode {
    ResourceDesc desc;
    VirtualResourceHandle handle;

    // Lifetime tracking
    u32 firstUsedPass = INVALID_INDEX;  // First pass that accesses this
    u32 lastUsedPass = INVALID_INDEX;   // Last pass that accesses this
    u32 refCount = 0;                   // Number of passes using this

    // Physical resource (allocated during compile)
    xray::render::ng::TextureHandle physicalTexture;
    xray::render::ng::BufferHandle physicalBuffer;

    // Direct NVRHI handles (until ResourceManager integration)
    nvrhi::TextureHandle nvrhiTexture;
    nvrhi::BufferHandle nvrhiBuffer;

    // Memory aliasing
    bool canAlias = true;               // Can share memory with others
    u32 aliasedWith = INVALID_INDEX;    // Index of resource we alias with
    u64 memoryOffset = 0;               // Offset in aliased memory

    // State tracking
    ResourceState currentState = ResourceState::Undefined;

    // Statistics
    u64 memorySize = 0;

    // Flags
    bool isAllocated = false;
    bool isPersistent = false;  // Lives across frames

    ResourceNode() = default;

    explicit ResourceNode(const ResourceDesc& _desc)
        : desc(_desc)
        , memorySize(_desc.ComputeMemorySize())
        , canAlias(_desc.isTransient && !_desc.isImported)
        , isPersistent(!_desc.isTransient)
    {}

    // Check if resource lifetime overlaps with another
    bool OverlapsWith(const ResourceNode& other) const {
        if (firstUsedPass == INVALID_INDEX || other.firstUsedPass == INVALID_INDEX) {
            return false;
        }

        // Check for overlap: [first1, last1] ) [first2, last2] != 
        return !(lastUsedPass < other.firstUsedPass ||
                 other.lastUsedPass < firstUsedPass);
    }

    // Get lifetime span
    u32 GetLifetimeSpan() const {
        if (firstUsedPass == INVALID_INDEX || lastUsedPass == INVALID_INDEX) {
            return 0;
        }
        return lastUsedPass - firstUsedPass + 1;
    }
};

} // namespace xray::render::framegraph
