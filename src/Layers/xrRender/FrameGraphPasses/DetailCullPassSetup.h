#pragma once

#include "Layers/xrRender/FrameGraph/FGTypes.h"
#include "Layers/xrRender/FrameGraph/FGResource.h"
#include "Layers/xrRender/FGDetailManager.h"

namespace xray::profiler
{
class GPUProfiler;
}

namespace xray::render::framegraph
{
class FrameGraph;
class RenderPassBuilder;
}

namespace xray::render::fg::passes
{

struct DetailPassState
{
    bool detailDataUploaded = false;
};

class DetailPassResources
{
public:
    std::shared_ptr<FGDetailManager::VisibilityFrame> frame;
    xr_vector<framegraph::VirtualResourceHandle> sourceChunks;
    framegraph::VirtualResourceHandle visible[FGDetailManager::VIS_KIND_COUNT];
    framegraph::VirtualResourceHandle drawArgs[FGDetailManager::VIS_KIND_COUNT];
    framegraph::VirtualResourceHandle prepared[FGDetailManager::LOD_COUNT];
    framegraph::VirtualResourceHandle indices[FGDetailManager::LOD_COUNT];
    framegraph::VirtualResourceHandle models;
    framegraph::VirtualResourceHandle pulled;
    framegraph::VirtualResourceHandle packets;
    framegraph::VirtualResourceHandle swDispatch;
    framegraph::VirtualResourceHandle perlin;
    framegraph::VirtualResourceHandle interaction[2];
    framegraph::VirtualResourceHandle tints;

    bool HasSource() const;
    void Read(framegraph::RenderPassBuilder& builder, bool drawIndirect, bool swIndirect) const;
};

DetailPassResources setupDetailCullPass(
    framegraph::FrameGraph& fg,
    fg::RenderDevice* device,
    fg::FGDetailManager* detailManager,
    framegraph::VirtualResourceHandle prevHiZ,
    nvrhi::ITexture* fallbackHiZ,
    u32 hiZWidth,
    u32 hiZHeight,
    u32 hiZMipLevels,
    const Fmatrix& prevViewProj,
    xray::profiler::GPUProfiler* gpuProfiler,
    DetailPassState* detailState);

}
