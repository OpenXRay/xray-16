#include "stdafx.h"
#include "ClusterDrawConfig.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"

namespace xray::render::fg::passes {

bool ClusterDrawConfig::HasGeometry() const
{
    return geometry.valid && refCount > 0;
}

bool ClusterDrawConfig::IsValid() const
{
    return HasGeometry() && visibleEntries.is_valid() && fades.is_valid() && args.is_valid()
        && staticRefCount > 0;
}

bool ClusterDrawConfig::TerrainValid() const
{
    return HasGeometry() && terrainVisibleEntries.is_valid() && terrainFades.is_valid()
        && terrainArgs.is_valid() && terrainRefCount > 0;
}

bool ClusterDrawConfig::SwValid() const
{
    return HasGeometry() && swEntries.is_valid() && swArgs.is_valid() && UseCompactGeometry();
}

bool ClusterDrawConfig::UseCompactGeometry() const
{
    return geometry.clusterPages.is_valid() && geometry.clusterPayload.is_valid()
        && geometry.clusterVertices.is_valid();
}

ClusterDrawBuffers ClusterDrawConfig::Resolve(const framegraph::FrameGraph& graph, ClusterDrawUse use) const
{
    ClusterDrawBuffers out;
    if (!geometry.valid)
        return out;
    auto resolve = [&](framegraph::VirtualResourceHandle handle) -> nvrhi::IBuffer*
    {
        return handle.is_valid() ? graph.GetPhysicalBuffer(handle) : nullptr;
    };
    out.clusterRefs = resolve(geometry.clusterRefs);
    out.clusterMeta = resolve(geometry.clusterMeta);
    out.instances = resolve(geometry.instances);
    out.clusterPages = resolve(geometry.clusterPages);
    out.clusterPayload = resolve(geometry.clusterPayload);
    out.clusterVertices = resolve(geometry.clusterVertices);
    if (use == ClusterDrawUse::Raster && IsValid())
    {
        out.visibleEntryBuffer = resolve(visibleEntries);
        out.fadeBuffer = resolve(fades);
        out.argsBuffer = resolve(args);
    }
    if (use == ClusterDrawUse::Raster && TerrainValid())
    {
        out.terrainVisibleEntryBuffer = resolve(terrainVisibleEntries);
        out.terrainFadeBuffer = resolve(terrainFades);
        out.terrainArgsBuffer = resolve(terrainArgs);
    }
    if (use == ClusterDrawUse::Software && SwValid())
    {
        out.swEntryBuffer = resolve(swEntries);
        out.swArgsBuffer = resolve(swArgs);
    }
    return out;
}

}
