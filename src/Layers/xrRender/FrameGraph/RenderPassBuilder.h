#pragma once

#include "FGTypes.h"
#include "FGResource.h"
#include "FrameGraph.h"

namespace xray::render::framegraph {

class FrameGraph;

class RenderPassBuilder {
public:
    RenderPassBuilder(FrameGraph& frameGraph, PassHandle passHandle)
        : m_frameGraph(frameGraph)
        , m_passHandle(passHandle)
    {}

    VirtualResourceHandle read(VirtualResourceHandle resource,
                               ResourceState state = ResourceState::ShaderResource)
    {
        m_frameGraph.PassRead(m_passHandle, resource, state);
        return resource;
    }

    VirtualResourceHandle write(VirtualResourceHandle resource,
                                ResourceState state = ResourceState::RenderTarget)
    {
        m_frameGraph.PassWrite(m_passHandle, resource, state);
        return resource;
    }

    VirtualResourceHandle readWrite(VirtualResourceHandle resource,
                                    ResourceState state = ResourceState::UnorderedAccess)
    {
        m_frameGraph.PassReadWrite(m_passHandle, resource, state);
        return resource;
    }

    VirtualResourceHandle createTexture(const char* name, const ResourceDesc& desc)
    {
        VirtualResourceHandle handle = m_frameGraph.CreateTexture(name, desc);
        m_frameGraph.PassWrite(m_passHandle, handle,
            desc.isDepthStencil ? ResourceState::DepthStencilWrite : ResourceState::RenderTarget);
        return handle;
    }

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
        desc.debugName = name;

        return createTexture(name, desc);
    }

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
        desc.debugName = name;

        return createTexture(name, desc);
    }

    VirtualResourceHandle createBuffer(const char* name, const ResourceDesc& desc)
    {
        VirtualResourceHandle handle = m_frameGraph.CreateBuffer(name, desc);
        m_frameGraph.PassWrite(m_passHandle, handle, ResourceState::UnorderedAccess);
        return handle;
    }

    FrameGraph& getFrameGraph() { return m_frameGraph; }
    const FrameGraph& getFrameGraph() const { return m_frameGraph; }

    PassHandle getPassHandle() const { return m_passHandle; }

private:
    FrameGraph& m_frameGraph;
    PassHandle m_passHandle;
};

}