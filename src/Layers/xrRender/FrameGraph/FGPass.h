// xrRender/FrameGraph/FGPass.h
#pragma once

#include "FGTypes.h"
#include "FGResource.h"
#include "../RenderContext/RenderContext.h"

namespace xray::render::framegraph {

// Forward declarations
class FrameGraph;

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  PASS EXECUTION CALLBACK
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

// User-defined function executed during render
using PassExecuteCallback = std::function<void(xray::render::ng::RenderContext&, const FrameGraph&)>;

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  RESOURCE ACCESS
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

struct ResourceAccess {
    enum class Type : u8 {
        Read,       // Read-only (SRV, constant buffer)
        Write,      // Write-only (RTV, DSV, UAV)
        ReadWrite,  // Both read and write (UAV)
    };

    VirtualResourceHandle resource;
    Type accessType;
    ResourceState state;

    // For render targets
    bool isRenderTarget = false;
    bool isDepthStencil = false;
    u32 renderTargetIndex = 0;  // Which RT slot (0-7)

    // For load/store ops
    bool clearOnLoad = false;
    float clearColor[4] = {0, 0, 0, 0};
    float clearDepth = 1.0f;
    u8 clearStencil = 0;

    ResourceAccess() = default;

    ResourceAccess(VirtualResourceHandle _resource, Type _type, ResourceState _state)
        : resource(_resource)
        , accessType(_type)
        , state(_state)
    {}

    bool IsRead() const { return accessType == Type::Read || accessType == Type::ReadWrite; }
    bool IsWrite() const { return accessType == Type::Write || accessType == Type::ReadWrite; }
};

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  RESOURCE BARRIERS
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

struct ResourceBarrier {
    VirtualResourceHandle resource;
    ResourceState stateBefore;
    ResourceState stateAfter;

    ResourceBarrier() = default;

    ResourceBarrier(VirtualResourceHandle _resource, ResourceState _before, ResourceState _after)
        : resource(_resource)
        , stateBefore(_before)
        , stateAfter(_after)
    {}
};

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  PASS NODE (INTERNAL STATE)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

struct PassNode {
    PassHandle handle;
    shared_str name;

    // Resource dependencies
    xr_vector<ResourceAccess> resourceAccesses;

    // Resource barriers (inserted during compilation)
    xr_vector<ResourceBarrier> barriersBeforePass;

    // Computed dependencies (filled during compile)
    xr_vector<PassNode*> dependsOn;      // Passes this depends on
    xr_vector<PassNode*> dependents;     // Passes that depend on this

    // Execution
    PassExecuteCallback executeCallback;

    // Compilation state
    bool culled = false;                 // Removed during optimization
    u32 executionOrder = INVALID_INDEX;  // Order in sorted passes
    u32 depth = 0;                       // Depth in dependency tree

    // Profiling
    u32 timestampQueryStart = INVALID_INDEX;
    u32 timestampQueryEnd = INVALID_INDEX;
    float lastExecutionTimeMs = 0.0f;

    // Flags
    bool isAsync = false;                // Can run on async compute queue
    bool isGraphics = true;              // Uses graphics pipeline
    bool isCompute = false;              // Uses compute pipeline
    bool isCopy = false;                 // Copy/blit operation

    PassNode() = default;

    explicit PassNode(const char* _name)
        : name(_name)
    {}

    // Add resource access
    void Read(VirtualResourceHandle resource, ResourceState state = ResourceState::ShaderResource) {
        ResourceAccess access;
        access.resource = resource;
        access.accessType = ResourceAccess::Type::Read;
        access.state = state;
        resourceAccesses.push_back(access);
    }

    void Write(VirtualResourceHandle resource, ResourceState state = ResourceState::RenderTarget) {
        ResourceAccess access;
        access.resource = resource;
        access.accessType = ResourceAccess::Type::Write;
        access.state = state;
        resourceAccesses.push_back(access);
    }

    void ReadWrite(VirtualResourceHandle resource, ResourceState state = ResourceState::UnorderedAccess) {
        ResourceAccess access;
        access.resource = resource;
        access.accessType = ResourceAccess::Type::ReadWrite;
        access.state = state;
        resourceAccesses.push_back(access);
    }

    // Add render target
    void WriteRenderTarget(VirtualResourceHandle resource, u32 index = 0,
                          bool clear = false, const float clearColor[4] = nullptr) {
        ResourceAccess access;
        access.resource = resource;
        access.accessType = ResourceAccess::Type::Write;
        access.state = ResourceState::RenderTarget;
        access.isRenderTarget = true;
        access.renderTargetIndex = index;
        access.clearOnLoad = clear;
        if (clearColor) {
            memcpy(access.clearColor, clearColor, sizeof(float) * 4);
        }
        resourceAccesses.push_back(access);
    }

    // Add depth/stencil target
    void WriteDepthStencil(VirtualResourceHandle resource, bool clear = false,
                          float clearDepth = 1.0f, u8 clearStencil = 0) {
        ResourceAccess access;
        access.resource = resource;
        access.accessType = ResourceAccess::Type::Write;
        access.state = ResourceState::DepthStencilWrite;
        access.isDepthStencil = true;
        access.clearOnLoad = clear;
        access.clearDepth = clearDepth;
        access.clearStencil = clearStencil;
        resourceAccesses.push_back(access);
    }

    // Get all resources this pass reads
    void GetReadResources(xr_vector<VirtualResourceHandle>& outResources) const {
        for (const auto& access : resourceAccesses) {
            if (access.IsRead()) {
                outResources.push_back(access.resource);
            }
        }
    }

    // Get all resources this pass writes
    void GetWriteResources(xr_vector<VirtualResourceHandle>& outResources) const {
        for (const auto& access : resourceAccesses) {
            if (access.IsWrite()) {
                outResources.push_back(access.resource);
            }
        }
    }

    // Check if this pass depends on another
    bool DependsOn(const PassNode* other) const {
        for (const auto* dep : dependsOn) {
            if (dep == other) return true;
        }
        return false;
    }
};

} // namespace xray::render::framegraph
