#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/ShaderVariant/VariantPartitionConfig.h"
#include <nvrhi/nvrhi.h>

namespace xray::render {
    class GeometryCollector;
    class MaterialCache;
    namespace ng {
        class RenderDevice;
    }
}

namespace xray::render::framegraph {
    class FrameGraph;
    struct DefaultOutputLayout;
}

namespace xray::render::RENDER_NAMESPACE::passes {

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

void InitializeForwardPipelines(ng::RenderDevice* device);

void ShutdownForwardPipelines();

framegraph::DefaultOutputLayout setupForwardColorPass(
    framegraph::FrameGraph& fg,
    ng::RenderDevice* device,
    framegraph::VirtualResourceHandle depthInput,
    framegraph::VirtualResourceHandle colorInput,
    framegraph::VirtualResourceHandle normalInput,
    const GeometryCollector* geometry,
    MaterialCache* materialCache,
    u32 width,
    u32 height,
    framegraph::VirtualResourceHandle drawArgsBuffer = framegraph::VirtualResourceHandle(),
    const BindlessForwardConfig& bindlessConfig = BindlessForwardConfig()
);

}
