// xrRender/FrameGraph/FGTypes.h
#pragma once

#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {

    // ══════════════════════════════════════════════════════════
    //  CONSTANTS (MUST BE DEFINED FIRST)
    // ══════════════════════════════════════════════════════════

    constexpr u32 INVALID_INDEX = 0xFFFFFFFF;
    constexpr u32 MAX_RENDER_TARGETS = 8;
    constexpr u32 MAX_PASS_DEPENDENCIES = 32;

    // ══════════════════════════════════════════════════════════
    //  HANDLES
    // ══════════════════════════════════════════════════════════

    // Virtual resource handle (lightweight, just an index)
    struct VirtualResourceHandle {
        u32 index = INVALID_INDEX;

        bool is_valid() const { return index != INVALID_INDEX; }
        bool operator==(const VirtualResourceHandle& other) const { return index == other.index; }
        bool operator!=(const VirtualResourceHandle& other) const { return index != other.index; }
    };

    // Pass handle (lightweight, just an index)
    struct PassHandle {
        u32 index = INVALID_INDEX;

        bool is_valid() const { return index != INVALID_INDEX; }
        bool operator==(const PassHandle& other) const { return index == other.index; }
        bool operator!=(const PassHandle& other) const { return index != other.index; }
    };

    // ══════════════════════════════════════════════════════════
    //  RESOURCE STATES
    // ══════════════════════════════════════════════════════════

    enum class ResourceState : u8 {
        Undefined,           // Initial state
        RenderTarget,        // Color attachment
        DepthStencilWrite,   // Depth/stencil write
        DepthStencilRead,    // Depth/stencil read-only
        ShaderResource,      // Texture/buffer SRV
        UnorderedAccess,     // UAV
        CopySource,          // Copy source
        CopyDest,            // Copy destination
        Present,             // Presentable
        Common,              // Generic read state
    };

    // Convert to string for debugging
    inline const char* ResourceStateToString(ResourceState state) {
        switch (state) {
        case ResourceState::Undefined: return "Undefined";
        case ResourceState::RenderTarget: return "RenderTarget";
        case ResourceState::DepthStencilWrite: return "DepthStencilWrite";
        case ResourceState::DepthStencilRead: return "DepthStencilRead";
        case ResourceState::ShaderResource: return "ShaderResource";
        case ResourceState::UnorderedAccess: return "UnorderedAccess";
        case ResourceState::CopySource: return "CopySource";
        case ResourceState::CopyDest: return "CopyDest";
        case ResourceState::Present: return "Present";
        case ResourceState::Common: return "Common";
        default: return "Unknown";
        }
    }

} // namespace xray::render::framegraph
