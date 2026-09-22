#pragma once

#include "r__sector.h"
#include "xrCDB/ISpatial.h"
#include "FrameGraphPasses/PathTracerPassSetup.h"
#include "FrameGraphPasses/ReSTIRGIPassSetup.h"
#include <memory>

class IRenderBackend;

namespace xray::render::fg
{
class RenderViewFrame
{
public:
    Fmatrix view = Fidentity;
    Fmatrix project = Fidentity;
    Fmatrix viewProj = Fidentity;
    Fvector cameraPos = {};
    Fvector cameraDir = {};
    float detailTime = 0.0f;
    u32 frameIndex = 0;
    u32 epoch = 0;
    u32 width = 0;
    u32 height = 0;
    u32 guideIndex = 0;
    u64 lease = 0;
    bool surfacesRecorded = false;
    bool hizRecorded = false;
    passes::PathTracerHistory pathTracer;
    xr_vector<nvrhi::TextureHandle> textures;
    xr_vector<nvrhi::BufferHandle> buffers;
};

class ViewSurfaceHistoryData
{
public:
    framegraph::VirtualResourceHandle depth;
    framegraph::VirtualResourceHandle normal;
    framegraph::VirtualResourceHandle historyDepth;
    framegraph::VirtualResourceHandle historyNormal;
    std::shared_ptr<RenderViewFrame> frame;
};

class RenderView
{
public:
    RenderView();
    RenderView(const RenderView&) = delete;
    RenderView& operator=(const RenderView&) = delete;
    ~RenderView();
    void reset();
    void r_pmask(bool first, bool second, bool wallmarks = false);
    void BeginFrame(IRenderBackend* backend, u32 frameIndex, u32 width, u32 height,
        const Fmatrix& view, const Fmatrix& project, const Fmatrix& viewProj,
        const Fvector& cameraPos, const Fvector& cameraDir, float detailTime);
    void BeginRecording();
    void FinishRecording(const LightingFrameState& lighting, nvrhi::ITexture* hiz, bool hizRecorded);
    void InvalidateHistory();
    void Shutdown();
    void CaptureSurfaceHistory(framegraph::FrameGraph& graph, RenderDevice* device,
        framegraph::VirtualResourceHandle depth, framegraph::VirtualResourceHandle normal);
    framegraph::VirtualResourceHandle ImportPreviousDepth(framegraph::FrameGraph& graph) const;
    framegraph::VirtualResourceHandle ImportPreviousNormals(framegraph::FrameGraph& graph) const;

    u32 phase{};
    u32 portal_traverse_flags{};
    u32 spatial_traverse_flags{};
    u32 spatial_types{ STYPE_RENDERABLE };
    float query_box_side{ EPS_L * 20.0f };
    Fvector view_pos{};
    Fmatrix xform{};
    CFrustum view_frustum{};
    IRender_Sector::sector_id_t sector_id{ IRender_Sector::INVALID_SECTOR_ID };
    bool pmask[2]{ true, true };
    bool pmask_wmark{ false };
    bool use_hom{ false };
    bool precise_portals{ false };
    bool is_main_pass{ false };
    bool mt_calculate{ false };

    Fmatrix prevView = Fidentity;
    Fmatrix prevProject = Fidentity;
    Fmatrix prevViewProj = Fidentity;
    Fvector prevCameraPos = {};
    float prevDetailTime = 0.0f;
    u32 prevFrameWidth = 0;
    u32 prevFrameHeight = 0;
    u32 writeIndex = 0;
    bool hasPrevFrameData = false;
    bool hasPrevHiZ = false;
    passes::ReSTIRGIPassState rtgi;
    passes::PathTracerPassState pathTracer;

private:
    IRenderBackend* m_backend = nullptr;
    u32 m_epoch = 0;
    nvrhi::TextureHandle m_depth[2];
    nvrhi::TextureHandle m_normals[2];
    std::shared_ptr<RenderViewFrame> m_recording;
    xr_vector<std::shared_ptr<RenderViewFrame>> m_frames;
};
}
