// xrRender/FrameGraph/FGResource.cpp
#include "stdafx.h"
#include "FGResource.h"

namespace xray::render::framegraph {

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  RESOURCE BUILDER (FLUENT API)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

class ResourceBuilder {
public:
    explicit ResourceBuilder(const char* name) {
        m_desc.debugName = name;
    }

    // Texture configuration
    ResourceBuilder& Texture2D(u32 width, u32 height, nvrhi::Format format) {
        m_desc.type = ResourceDesc::Type::Texture2D;
        m_desc.width = width;
        m_desc.height = height;
        m_desc.format = format;
        return *this;
    }

    ResourceBuilder& Texture3D(u32 width, u32 height, u32 depth, nvrhi::Format format) {
        m_desc.type = ResourceDesc::Type::Texture3D;
        m_desc.width = width;
        m_desc.height = height;
        m_desc.depth = depth;
        m_desc.format = format;
        return *this;
    }

    ResourceBuilder& TextureArray(u32 width, u32 height, u32 arraySize, nvrhi::Format format) {
        m_desc.type = ResourceDesc::Type::Texture2DArray;
        m_desc.width = width;
        m_desc.height = height;
        m_desc.arraySize = arraySize;
        m_desc.format = format;
        return *this;
    }

    ResourceBuilder& Buffer(u64 size) {
        m_desc.type = ResourceDesc::Type::Buffer;
        m_desc.bufferSize = size;
        return *this;
    }

    ResourceBuilder& StructuredBuffer(u64 size, u32 stride) {
        m_desc.type = ResourceDesc::Type::Buffer;
        m_desc.bufferSize = size;
        m_desc.structStride = stride;
        return *this;
    }

    // Usage flags
    ResourceBuilder& RenderTarget() {
        m_desc.isRenderTarget = true;
        return *this;
    }

    ResourceBuilder& DepthStencil() {
        m_desc.isDepthStencil = true;
        return *this;
    }

    ResourceBuilder& UAV() {
        m_desc.isUAV = true;
        m_desc.allowUAV = true;
        return *this;
    }

    ResourceBuilder& AllowUAV() {
        m_desc.allowUAV = true;
        return *this;
    }

    ResourceBuilder& Mips(u32 mipLevels) {
        m_desc.mipLevels = mipLevels;
        return *this;
    }

    ResourceBuilder& MSAA(u32 sampleCount) {
        m_desc.sampleCount = sampleCount;
        return *this;
    }

    // Lifetime hints
    ResourceBuilder& Transient() {
        m_desc.isTransient = true;
        return *this;
    }

    ResourceBuilder& Persistent() {
        m_desc.isTransient = false;
        return *this;
    }

    ResourceBuilder& Imported() {
        m_desc.isImported = true;
        m_desc.isTransient = false;
        return *this;
    }

    const ResourceDesc& Build() const { return m_desc; }

private:
    ResourceDesc m_desc;
};

} // namespace xray::render::framegraph
