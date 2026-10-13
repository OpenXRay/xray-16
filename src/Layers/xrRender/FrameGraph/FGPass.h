#pragma once

#include "FGTypes.h"
#include "FGResource.h"
#include "../RenderContext/RenderContext.h"

#include <utility>

namespace xray::render::framegraph {

class FrameGraph;

struct IPassCallback {
    virtual ~IPassCallback() = default;
    virtual void operator()(xray::render::fg::RenderContext& ctx, const FrameGraph& fg) = 0;
};

template <typename F>
struct PassCallback final : IPassCallback {
    F fn;

    explicit PassCallback(F&& f) : fn(std::move(f)) {}
    explicit PassCallback(const F& f) : fn(f) {}

    void operator()(xray::render::fg::RenderContext& ctx, const FrameGraph& fg) override
    {
        fn(ctx, fg);
    }
};

template <typename Data, typename F>
struct DataPassCallback final : IPassCallback {
    Data data;
    F fn;

    explicit DataPassCallback(F&& f) : fn(std::move(f)) {}
    explicit DataPassCallback(const F& f) : fn(f) {}

    void operator()(xray::render::fg::RenderContext& ctx, const FrameGraph& fg) override
    {
        fn(data, fg, &ctx);
    }
};

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
    float clearDepth = 0.0f;
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

enum class EdgeKind : u8 {
    ReadAfterWrite,
    WriteAfterRead,
    WriteAfterWrite,
};

struct PassDependency {
    PassNode* pass = nullptr;
    EdgeKind kind = EdgeKind::ReadAfterWrite;
};

enum class PassQueue : u8 {
    Graphics,
    Compute,
};

struct PassNode {
    PassHandle handle;
    shared_str name;

    xr_vector<ResourceAccess> resourceAccesses;

    xr_vector<ResourceBarrier> barriersBeforePass;

    xr_vector<PassDependency> dependsOn;
    xr_vector<ResourceNode*> devirtualize;
    xr_vector<ResourceNode*> destroy;

    IPassCallback* executeCallback = nullptr;

    bool culled = false;
    u32 executionOrder = INVALID_INDEX;
    u32 depth = 0;

    u32 timestampQueryStart = INVALID_INDEX;
    u32 timestampQueryEnd = INVALID_INDEX;
    float lastExecutionTimeMs = 0.0f;

    PassQueue queue = PassQueue::Graphics;
    bool forkAfter = false;
    u32 segment = INVALID_INDEX;
    bool hasSideEffects = false;

    PassNode() = default;

    explicit PassNode(const char* _name)
        : name(_name)
    {}

    void Recycle(const char* _name)
    {
        name = _name;
        resourceAccesses.clear();
        barriersBeforePass.clear();
        dependsOn.clear();
        devirtualize.clear();
        destroy.clear();
        executeCallback = nullptr;
        culled = false;
        executionOrder = INVALID_INDEX;
        depth = 0;
        timestampQueryStart = INVALID_INDEX;
        timestampQueryEnd = INVALID_INDEX;
        lastExecutionTimeMs = 0.0f;
        queue = PassQueue::Graphics;
        forkAfter = false;
        segment = INVALID_INDEX;
        hasSideEffects = false;
    }

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
                          float clearDepth = 0.0f, u8 clearStencil = 0) {
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
        for (const auto& dep : dependsOn) {
            if (dep.pass == other) return true;
        }
        return false;
    }

    PassDependency* FindDependency(const PassNode* other) {
        for (auto& dep : dependsOn) {
            if (dep.pass == other) return &dep;
        }
        return nullptr;
    }
};

}
