#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/ShaderVariant/VariantPartitionConfig.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
    struct DefaultOutputLayout;
}

namespace xray::render::fg::passes {

struct TransparentPassConfig {
    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
    nvrhi::IBuffer* instanceBuffer = nullptr;
    nvrhi::IBuffer* compactDrawArgsBuffer = nullptr;
    nvrhi::IBuffer* compactBatchIndicesBuffer = nullptr;
    nvrhi::IBuffer* compactMaterialIDBuffer = nullptr;
    nvrhi::IBuffer* compactCountBuffer = nullptr;
    u32 objectCount = 0;

    VariantPartitionConfig variantPartition;

    // Cascaded shadow maps + sky cubes for water reflections
    nvrhi::ITexture* shadowMapArray = nullptr;
    nvrhi::ITexture* shadowCascades[3] = {};
    nvrhi::ITexture* localShadowAtlas = nullptr;
    nvrhi::ITexture* localShadowESM = nullptr;
    nvrhi::ITexture* envSky0 = nullptr;
    nvrhi::ITexture* envSky1 = nullptr;
    nvrhi::ITexture* contactDepth = nullptr; // unused (history-only contact)
    nvrhi::ITexture* contactHistory = nullptr; // g_ContactHistory @ t28
    nvrhi::ITexture* shadowHZB[3] = {};
    framegraph::VirtualResourceHandle shadowHZBHandles[3];
    nvrhi::ITexture* shadowMask = nullptr;
    framegraph::VirtualResourceHandle shadowMaskHandle;

    bool IsValid() const {
        return objectCount > 0 && compactDrawArgsBuffer && megaVertexBuffer && megaIndexBuffer;
    }
};

struct TransparentPassState {
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::GraphicsPipelineHandle multiplyPipeline;
    nvrhi::BindingLayoutHandle layout;
    nvrhi::GraphicsPipelineHandle waterPipeline;
    nvrhi::BindingLayoutHandle waterLayout;
    nvrhi::GraphicsPipelineHandle waterDistortPipeline;
    nvrhi::BindingLayoutHandle waterDistortLayout;
    nvrhi::InputLayoutHandle inputLayout;
    nvrhi::InputLayoutHandle waterDistortInputLayout;
    nvrhi::SamplerHandle sampler;
    nvrhi::ShaderHandle vs;
    nvrhi::ShaderHandle ps;
    nvrhi::ShaderHandle waterVs;
    nvrhi::ShaderHandle waterPs;
    nvrhi::ShaderHandle waterDistortVs;
    nvrhi::ShaderHandle waterDistortPs;
    nvrhi::ITexture* foamTexture = nullptr;
    nvrhi::TextureHandle waterSsrColor;
    nvrhi::TextureHandle waterSceneDepth;
    nvrhi::TextureHandle waterSceneWorldPos;
    bool initialized = false;
};

struct TransparentPassData {
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle color;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle worldPos;
    framegraph::VirtualResourceHandle distortion;
    fg::RenderDevice* device;
    TransparentPassConfig config;
    TransparentPassState* passState;
    u32 width, height;
};

void InitializeTransparentResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, TransparentPassState& state);

framegraph::DefaultOutputLayout setupTransparentPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    const framegraph::DefaultOutputLayout& inputs,
    const TransparentPassConfig& config,
    u32 width, u32 height,
    TransparentPassState& state
);

} // namespace xray::render::fg::passes
