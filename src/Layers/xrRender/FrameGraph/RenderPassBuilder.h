// xrRender/FrameGraph/RenderPassBuilder.h
#pragma once

#include "FGTypes.h"
#include "FGResource.h"
#include "FrameGraph.h"

namespace xray::render::framegraph {

// Forward declaration
class FrameGraph;

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  RENDER PASS BUILDER
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
// Provides pass-scoped resource declaration API
// Used in the setup lambda of addCallbackPass

class RenderPassBuilder {
public:
    RenderPassBuilder(FrameGraph& frameGraph, PassHandle passHandle)
        : m_frameGraph(frameGraph)
        , m_passHandle(passHandle)
    {}

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  RESOURCE ACCESS DECLARATION
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    // Read access (shader resource view, constant buffer)
    VirtualResourceHandle read(VirtualResourceHandle resource,
                               ResourceState state = ResourceState::ShaderResource)
    {
        m_frameGraph.PassRead(m_passHandle, resource, state);
        return resource;
    }

    // Write access (render target, depth stencil, UAV)
    VirtualResourceHandle write(VirtualResourceHandle resource,
                                ResourceState state = ResourceState::RenderTarget)
    {
        m_frameGraph.PassWrite(m_passHandle, resource, state);
        return resource;
    }

    // Read-write access (UAV)
    VirtualResourceHandle readWrite(VirtualResourceHandle resource,
                                    ResourceState state = ResourceState::UnorderedAccess)
    {
        m_frameGraph.PassReadWrite(m_passHandle, resource, state);
        return resource;
    }

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  RESOURCE CREATION
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    // Create texture resource
    VirtualResourceHandle createTexture(const char* name, const ResourceDesc& desc)
    {
        VirtualResourceHandle handle = m_frameGraph.CreateTexture(name, desc);
        // Automatically mark as write for this pass (typical pattern)
        m_frameGraph.PassWrite(m_passHandle, handle,
            desc.isDepthStencil ? ResourceState::DepthStencilWrite : ResourceState::RenderTarget);
        return handle;
    }

    // Create texture with common settings
    VirtualResourceHandle createTexture2D(
        const char* name,
        u32 width,
        u32 height,
        nvrhi::Format format,
        bool isRenderTarget = true)
    {
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.depth = 1;
        desc.mipLevels = 1;
        desc.arraySize = 1;
        desc.format = format;
        desc.isRenderTarget = isRenderTarget;
        desc.isDepthStencil = false;
        desc.isUAV = false;
        desc.debugName = name;  // Set debug name

        return createTexture(name, desc);
    }

    // Create depth buffer
    VirtualResourceHandle createDepthBuffer(
        const char* name,
        u32 width,
        u32 height,
        nvrhi::Format format = nvrhi::Format::D32)
    {
        ResourceDesc desc;
        desc.type = ResourceDesc::Type::Texture2D;
        desc.width = width;
        desc.height = height;
        desc.depth = 1;
        desc.mipLevels = 1;
        desc.arraySize = 1;
        desc.format = format;
        desc.isRenderTarget = false;
        desc.isDepthStencil = true;
        desc.isUAV = false;
        desc.debugName = name;  // Set debug name

        return createTexture(name, desc);
    }

    // Create buffer resource
    VirtualResourceHandle createBuffer(const char* name, const ResourceDesc& desc)
    {
        VirtualResourceHandle handle = m_frameGraph.CreateBuffer(name, desc);
        // Automatically mark as write for this pass
        m_frameGraph.PassWrite(m_passHandle, handle, ResourceState::UnorderedAccess);
        return handle;
    }

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  UTILITY
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    // Mark this pass as async compute (runs on compute queue)
    void asyncCompute() {
        m_frameGraph.SetPassAsyncCompute(m_passHandle);
    }

    void sideEffects() {
        m_frameGraph.SetPassHasSideEffects(m_passHandle);
    }

    // Get the underlying FrameGraph
    FrameGraph& getFrameGraph() { return m_frameGraph; }
    const FrameGraph& getFrameGraph() const { return m_frameGraph; }

    // Get the pass handle
    PassHandle getPassHandle() const { return m_passHandle; }

private:
    FrameGraph& m_frameGraph;
    PassHandle m_passHandle;
};

} // namespace xray::render::framegraph
