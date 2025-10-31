#pragma once

#include "FGTypes.h"
#include "FGResource.h"
#include "../RenderContext/RenderContext.h"

namespace xray::render::framegraph {

class FrameGraph;

using PassExecuteCallback = std::function<void(xray::render::ng::RenderContext&, const FrameGraph&)>;

struct ResourceAccess {
    enum class Type : u8 {
        Read,
        Write,
        ReadWrite,
    };

    VirtualResourceHandle resource;
    Type accessType;
    ResourceState state;

    bool isRenderTarget = false;
    bool isDepthStencil = false;
    u32 renderTargetIndex = 0;

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

struct PassNode {
    PassHandle handle;
    shared_str name;

    xr_vector<ResourceAccess> resourceAccesses;

    xr_vector<ResourceBarrier> barriersBeforePass;

    xr_vector<PassNode*> dependsOn;
    xr_vector<PassNode*> dependents;

    PassExecuteCallback executeCallback;

    bool culled = false;
    u32 executionOrder = INVALID_INDEX;
    u32 depth = 0;

    u32 timestampQueryStart = INVALID_INDEX;
    u32 timestampQueryEnd = INVALID_INDEX;
    float lastExecutionTimeMs = 0.0f;

    bool isAsync = false;
    bool isGraphics = true;
    bool isCompute = false;
    bool isCopy = false;

    PassNode() = default;

    explicit PassNode(const char* _name)
        : name(_name)
    {}

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

    void GetReadResources(xr_vector<VirtualResourceHandle>& outResources) const {
        for (const auto& access : resourceAccesses) {
            if (access.IsRead()) {
                outResources.push_back(access.resource);
            }
        }
    }

    void GetWriteResources(xr_vector<VirtualResourceHandle>& outResources) const {
        for (const auto& access : resourceAccesses) {
            if (access.IsWrite()) {
                outResources.push_back(access.resource);
            }
        }
    }

    bool DependsOn(const PassNode* other) const {
        for (const auto* dep : dependsOn) {
            if (dep == other) return true;
        }
        return false;
    }
};

}
