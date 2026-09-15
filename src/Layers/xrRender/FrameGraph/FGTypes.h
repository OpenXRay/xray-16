#pragma once

#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {

    constexpr u32 INVALID_INDEX = 0xFFFFFFFF;
    constexpr u32 MAX_RENDER_TARGETS = 8;
    constexpr u32 MAX_PASS_DEPENDENCIES = 32;

    struct VirtualResourceHandle {
        u32 index = INVALID_INDEX;
        u32 generation = 0;

        bool is_valid() const { return index != INVALID_INDEX; }
        bool operator==(const VirtualResourceHandle& other) const {
            return index == other.index && generation == other.generation;
        }
        bool operator!=(const VirtualResourceHandle& other) const { return !(*this == other); }
    };

    struct PassHandle {
        u32 index = INVALID_INDEX;

        bool is_valid() const { return index != INVALID_INDEX; }
        bool operator==(const PassHandle& other) const { return index == other.index; }
        bool operator!=(const PassHandle& other) const { return index != other.index; }
    };

    enum class ResourceState : u8 {
        Undefined,
        RenderTarget,
        DepthStencilWrite,
        DepthStencilRead,
        ShaderResource,
        UnorderedAccess,
        IndirectArgument,
        CopySource,
        CopyDest,
        Present,
        Common,
        DepthStencilReadShaderResource,
    };

    inline const char* ResourceStateToString(ResourceState state) {
        switch (state) {
        case ResourceState::Undefined: return "Undefined";
        case ResourceState::RenderTarget: return "RenderTarget";
        case ResourceState::DepthStencilWrite: return "DepthStencilWrite";
        case ResourceState::DepthStencilRead: return "DepthStencilRead";
        case ResourceState::DepthStencilReadShaderResource: return "DepthStencilReadShaderResource";
        case ResourceState::ShaderResource: return "ShaderResource";
        case ResourceState::UnorderedAccess: return "UnorderedAccess";
        case ResourceState::IndirectArgument: return "IndirectArgument";
        case ResourceState::CopySource: return "CopySource";
        case ResourceState::CopyDest: return "CopyDest";
        case ResourceState::Present: return "Present";
        case ResourceState::Common: return "Common";
        default: return "Unknown";
        }
    }

}
