#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::framegraph {
    class FrameGraph;
}

namespace xray::render::fg::passes {

class ClusterDrawBuffers
{
public:
    nvrhi::IBuffer* clusterRefs = nullptr;
    nvrhi::IBuffer* clusterMeta = nullptr;
    nvrhi::IBuffer* instances = nullptr;
    nvrhi::IBuffer* clusterPages = nullptr;
    nvrhi::IBuffer* clusterPayload = nullptr;
    nvrhi::IBuffer* clusterVertices = nullptr;
    nvrhi::IBuffer* visibleEntryBuffer = nullptr;
    nvrhi::IBuffer* fadeBuffer = nullptr;
    nvrhi::IBuffer* argsBuffer = nullptr;
    nvrhi::IBuffer* terrainVisibleEntryBuffer = nullptr;
    nvrhi::IBuffer* terrainFadeBuffer = nullptr;
    nvrhi::IBuffer* terrainArgsBuffer = nullptr;
    nvrhi::IBuffer* swEntryBuffer = nullptr;
    nvrhi::IBuffer* swArgsBuffer = nullptr;
};

enum class ClusterDrawUse
{
    Geometry,
    Raster,
    Software
};

class ClusterDrawConfig
{
public:
    GeometryFrameResources geometry;
    framegraph::VirtualResourceHandle visibleEntries;
    framegraph::VirtualResourceHandle fades;
    framegraph::VirtualResourceHandle args;
    framegraph::VirtualResourceHandle terrainVisibleEntries;
    framegraph::VirtualResourceHandle terrainFades;
    framegraph::VirtualResourceHandle terrainArgs;
    framegraph::VirtualResourceHandle swEntries;
    framegraph::VirtualResourceHandle swArgs;

    u32 refCount = 0;
    u32 staticRefCount = 0;
    u32 terrainRefCount = 0;

    bool HasGeometry() const;
    bool IsValid() const;
    bool TerrainValid() const;
    bool SwValid() const;
    bool UseCompactGeometry() const;

    ClusterDrawBuffers Resolve(const framegraph::FrameGraph& graph, ClusterDrawUse use) const;
};

}
