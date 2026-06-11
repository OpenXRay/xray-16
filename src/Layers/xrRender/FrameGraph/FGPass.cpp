// xrRender/FrameGraph/FGPass.cpp
#include "stdafx.h"
#include "FGPass.h"

namespace xray::render::framegraph {

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  PASS BUILDER (FLUENT API)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

class PassBuilder {
public:
    explicit PassBuilder(PassNode* node)
        : m_node(node)
    {}

    // Resource access
    PassBuilder& Read(VirtualResourceHandle resource,
                     ResourceState state = ResourceState::ShaderResource) {
        m_node->Read(resource, state);
        return *this;
    }

    PassBuilder& Write(VirtualResourceHandle resource,
                      ResourceState state = ResourceState::RenderTarget) {
        m_node->Write(resource, state);
        return *this;
    }

    PassBuilder& ReadWrite(VirtualResourceHandle resource,
                          ResourceState state = ResourceState::UnorderedAccess) {
        m_node->ReadWrite(resource, state);
        return *this;
    }

    // Render targets
    PassBuilder& RenderTarget(VirtualResourceHandle resource, u32 index = 0) {
        m_node->WriteRenderTarget(resource, index, false, nullptr);
        return *this;
    }

    PassBuilder& RenderTargetClear(VirtualResourceHandle resource, u32 index,
                                   const float clearColor[4]) {
        m_node->WriteRenderTarget(resource, index, true, clearColor);
        return *this;
    }

    PassBuilder& DepthStencil(VirtualResourceHandle resource) {
        m_node->WriteDepthStencil(resource, false, 1.0f, 0);
        return *this;
    }

    PassBuilder& DepthStencilClear(VirtualResourceHandle resource,
                                   float clearDepth = 1.0f, u8 clearStencil = 0) {
        m_node->WriteDepthStencil(resource, true, clearDepth, clearStencil);
        return *this;
    }

    // Queue hints
    PassBuilder& Graphics() {
        m_node->isGraphics = true;
        m_node->isCompute = false;
        m_node->isCopy = false;
        return *this;
    }

    PassBuilder& Compute() {
        m_node->isGraphics = false;
        m_node->isCompute = true;
        m_node->isCopy = false;
        return *this;
    }

    PassBuilder& AsyncCompute() {
        Compute();
        m_node->isAsync = true;
        return *this;
    }

    PassBuilder& Copy() {
        m_node->isGraphics = false;
        m_node->isCompute = false;
        m_node->isCopy = true;
        return *this;
    }

    // Execution callback
    PassBuilder& Execute(IPassCallback* callback) {
        m_node->executeCallback = callback;
        return *this;
    }

private:
    PassNode* m_node;
};

} // namespace xray::render::framegraph
