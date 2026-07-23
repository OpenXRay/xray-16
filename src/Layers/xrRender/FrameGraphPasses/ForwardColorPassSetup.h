#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/ShaderVariant/VariantPartitionConfig.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    class GeometryCollector;
    class MaterialCache;
    namespace fg {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

struct BindlessDrawSet {
    nvrhi::IBuffer* compactDrawArgsBuffer = nullptr;
    nvrhi::IBuffer* compactMaterialIDBuffer = nullptr;
    nvrhi::IBuffer* compactBatchIndicesBuffer = nullptr;
    nvrhi::IBuffer* instanceBuffer = nullptr;
    nvrhi::IBuffer* compactCountBuffer = nullptr;
    u32 totalObjectCount = 0;

    bool IsValid() const {
        return compactDrawArgsBuffer && compactMaterialIDBuffer &&
            compactBatchIndicesBuffer && instanceBuffer && compactCountBuffer &&
            totalObjectCount > 0;
    }
};

struct BindlessForwardConfig {
    BindlessDrawSet staticSet;
    BindlessDrawSet dynamicSet;

    bool enabled = false;

    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    nvrhi::IBuffer* megaIndexBuffer = nullptr;
    bool megaBuffersReady = false;

    nvrhi::IBuffer* terrainDrawArgsBuffer = nullptr;
    nvrhi::IBuffer* terrainMaterialIDBuffer = nullptr;
    nvrhi::IBuffer* terrainInstanceBuffer = nullptr;
    nvrhi::IBuffer* terrainBatchIndicesBuffer = nullptr;
    nvrhi::IBuffer* terrainCompactDrawArgsBuffer = nullptr;
    nvrhi::IBuffer* terrainCompactBatchIndicesBuffer = nullptr;
    nvrhi::IBuffer* terrainCompactMaterialIDBuffer = nullptr;
    nvrhi::IBuffer* terrainCompactCountBuffer = nullptr;
    u32 terrainObjectCount = 0;

    bool UseGPUCulling() const {
        return enabled && (staticSet.IsValid() || dynamicSet.IsValid());
    }

    bool UseMegaBuffers() const {
        return megaBuffersReady && megaVertexBuffer && megaIndexBuffer;
    }

    bool UseTerrainCompaction() const {
        return terrainCompactDrawArgsBuffer && terrainCompactBatchIndicesBuffer &&
            terrainCompactMaterialIDBuffer && terrainCompactCountBuffer;
    }

    bool HasTerrain() const {
        return terrainObjectCount > 0 && (terrainDrawArgsBuffer || terrainCompactDrawArgsBuffer);
    }

    VariantPartitionConfig variantPartition;
};

struct ForwardColorPassState {
    nvrhi::GraphicsPipelineHandle bindlessPipeline;
    nvrhi::BindingLayoutHandle bindlessLayout;
    nvrhi::InputLayoutHandle bindlessInputLayout;
    nvrhi::SamplerHandle linearSampler;
    nvrhi::ShaderHandle bindlessVS;
    nvrhi::ShaderHandle bindlessPS;
    bool bindlessInitialized = false;
    nvrhi::GraphicsPipelineHandle terrainPipeline;
    nvrhi::BindingLayoutHandle terrainLayout;
    nvrhi::ShaderHandle terrainPS;
    bool terrainInitialized = false;
};

struct ForwardColorPassData {
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle color;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle baseColor;
    framegraph::VirtualResourceHandle drawArgsBuffer;
    fg::RenderDevice* device;
    const GeometryCollector* geometry;
    MaterialCache* materialCache;
    ForwardColorPassState* passState;
    framegraph::DefaultOutputLayout outputs;
    u32 width;
    u32 height;
    BindlessForwardConfig bindlessConfig;
};

void InitializeForwardResources(fg::RenderDevice* device, const nvrhi::FramebufferInfoEx& fbInfo, ForwardColorPassState& state);

framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    framegraph::VirtualResourceHandle colorInput,
    framegraph::VirtualResourceHandle normalInput,
    framegraph::VirtualResourceHandle baseColorInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    framegraph::VirtualResourceHandle drawArgsBuffer = framegraph::VirtualResourceHandle(),
    const BindlessForwardConfig& bindlessConfig = BindlessForwardConfig(),
    ForwardColorPassState* state = nullptr
);

}
