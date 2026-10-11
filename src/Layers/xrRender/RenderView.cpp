#include "stdafx.h"
#include "RenderView.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/RenderPassBuilder.h"
#include "RenderContext/RenderContext.h"
#include "xrEngine/IRenderBackend.h"

namespace xray::render::fg
{
RenderView::RenderView() = default;

RenderView::~RenderView()
{
    Shutdown();
}

void RenderView::reset()
{
    query_box_side = EPS_L * 20.0f;
    use_hom = false;
    precise_portals = false;
    is_main_pass = false;
    spatial_traverse_flags = 0;
    portal_traverse_flags = 0;
    spatial_types = STYPE_RENDERABLE;
}

void RenderView::r_pmask(bool first, bool second, bool wallmarks)
{
    pmask[0] = first;
    pmask[1] = second;
    pmask_wmark = wallmarks;
}

void RenderView::InvalidateHistory()
{
    ++m_epoch;
    hasPrevFrameData = false;
    hasPrevHiZ = false;
    rtgi.historyValid = false;
    pathTracer.history.valid = false;
    pathTracer.history.samples = 0;
}

void RenderView::BeginFrame(IRenderBackend* backend, u32 frameIndex, u32 width, u32 height,
    const Fmatrix& view, const Fmatrix& project, const Fmatrix& viewProj,
    const Fvector& cameraPos, const Fvector& cameraDir, float detailTime)
{
    R_ASSERT(!m_backend || m_backend == backend);
    m_backend = backend;
    if (m_recording && m_recording->lease)
        m_frames.push_back(std::move(m_recording));
    hasPrevFrameData = false;
    hasPrevHiZ = false;
    rtgi.historyValid = false;
    pathTracer.history.valid = false;
    pathTracer.history.samples = 0;
    std::shared_ptr<RenderViewFrame> previous;
    bool failed = false;
    for (auto it = m_frames.begin(); it != m_frames.end();)
    {
        const auto& frame = *it;
        const auto status = backend->PollSubmissionLease(frame->lease);
        failed |= status == IRenderBackend::SubmissionLeaseState::Failed ||
            status == IRenderBackend::SubmissionLeaseState::Unknown;
        if (frame->epoch == m_epoch && frame->frameIndex + 1 == frameIndex &&
            backend->IsSubmissionLeaseSubmitted(frame->lease))
            previous = frame;
        if (status == IRenderBackend::SubmissionLeaseState::Complete ||
            status == IRenderBackend::SubmissionLeaseState::Failed ||
            status == IRenderBackend::SubmissionLeaseState::Unknown)
        {
            if (frame->pathTracer.capturedSnapshot && frame->pathTracer.snapshot)
            {
                if (status == IRenderBackend::SubmissionLeaseState::Complete)
                    frame->pathTracer.snapshot->submitted = true;
                else if (pathTracer.snapshot == frame->pathTracer.snapshot)
                    passes::DiscardPathTracerSnapshot(pathTracer);
            }
            backend->ReleaseSubmissionLease(frame->lease);
            it = m_frames.erase(it);
        }
        else
            ++it;
    }
    if (failed)
        InvalidateHistory();
    if (previous && !failed && previous->width == width && previous->height == height)
    {
        pathTracer.history = previous->pathTracer;
        const bool cameraCut = cameraPos.distance_to(previous->cameraPos) > 5.0f ||
            cameraDir.dotproduct(previous->cameraDir) < 0.5f;
        if (!cameraCut)
        {
            prevView = previous->view;
            prevProject = previous->project;
            prevViewProj = previous->viewProj;
            prevCameraPos = previous->cameraPos;
            prevDetailTime = previous->detailTime;
            hasPrevFrameData = previous->surfacesRecorded;
            hasPrevHiZ = previous->hizRecorded;
            rtgi.historyValid = previous->rtgiHistoryRecorded;
        }
    }
    m_recording = std::make_shared<RenderViewFrame>();
    m_recording->view = view;
    m_recording->project = project;
    m_recording->viewProj = viewProj;
    m_recording->cameraPos = cameraPos;
    m_recording->cameraDir = cameraDir;
    m_recording->detailTime = detailTime;
    m_recording->frameIndex = frameIndex;
    m_recording->epoch = m_epoch;
    m_recording->width = width;
    m_recording->height = height;
    pathTracer.pending.valid = false;
}

void RenderView::BeginRecording()
{
    if (m_backend && m_backend->SupportsSubmissionLeases() && m_recording)
        m_recording->lease = m_backend->OpenSubmissionLease();
}

void RenderView::FinishRecording(const LightingFrameState& lighting, nvrhi::ITexture* hiz, bool hizRecorded)
{
    if (!m_recording || !m_recording->lease)
    {
        InvalidateHistory();
        return;
    }
    auto& frame = *m_recording;
    frame.hizRecorded = !lighting.frameFailed && hiz && hizRecorded;
    frame.rtgiHistoryRecorded = !lighting.frameFailed && lighting.effective == LightingMode::RTGI && lighting.recorded &&
        rtgi.reconstruction.recorded;
    const bool ptRecorded = !lighting.frameFailed && lighting.effective == LightingMode::ReferencePT && lighting.recorded;
    if (ptRecorded || pathTracer.pending.capturedSnapshot || (lighting.frameFailed && pathTracer.pending.snapshot))
    {
        frame.pathTracer = pathTracer.pending;
        if (!ptRecorded)
        {
            frame.pathTracer.valid = false;
            frame.pathTracer.samples = 0;
        }
    }
    if (lighting.frameFailed)
    {
        frame.surfacesRecorded = false;
        frame.pathTracer.valid = false;
        frame.pathTracer.samples = 0;
        frame.pathTracer.capturedSnapshot = false;
        pathTracer.pending.valid = false;
        pathTracer.pending.samples = 0;
        passes::DiscardPathTracerSnapshot(pathTracer);
        pathTracer.snapshotStats.frozen = false;
        pathTracer.snapshotStats.capturePending = false;
        pathTracer.snapshotStats.valid = false;
        pathTracer.snapshotStats.sceneValid = false;
        pathTracer.snapshotStats.fallback = pathTracer.snapshotStats.freezeRequested;
        pathTracer.snapshotStats.cdfActive = false;
        InvalidateHistory();
    }
    frame.textures.push_back(hiz);
    frame.textures.push_back(pathTracer.accumulation);
    frame.textures.push_back(rtgi.rawDiffuse);
    frame.textures.push_back(rtgi.rawSpecular);
    frame.textures.push_back(rtgi.emission);
    frame.textures.push_back(rtgi.normalRoughness);
    frame.textures.push_back(rtgi.albedoMetallic);
    frame.textures.push_back(rtgi.pathData);
    frame.textures.push_back(rtgi.surfaceData);
    frame.textures.push_back(rtgi.motion);
    m_frames.push_back(std::move(m_recording));
}

void RenderView::Shutdown()
{
    if (m_backend)
    {
        if (!m_frames.empty() || (m_recording && m_recording->lease))
            m_backend->WaitForIdle();
        for (const auto& frame : m_frames)
            m_backend->ReleaseSubmissionLease(frame->lease);
        if (m_recording && m_recording->lease)
            m_backend->ReleaseSubmissionLease(m_recording->lease);
    }
    m_frames.clear();
    m_recording.reset();
    m_backend = nullptr;
    passes::ShutdownReSTIRGI(rtgi);
    pathTracer = {};
    InvalidateHistory();
}

void RenderView::CaptureSurfaceHistory(framegraph::FrameGraph& graph,
    framegraph::VirtualResourceHandle depth, framegraph::VirtualResourceHandle normal,
    framegraph::VirtualResourceHandle historyDepth, framegraph::VirtualResourceHandle historyNormal)
{
    using namespace framegraph;
    if (!m_recording || !depth.is_valid() || !normal.is_valid() || !historyDepth.is_valid() || !historyNormal.is_valid())
        return;
    graph.addCallbackPass<ViewSurfaceHistoryData>("Opaque Surface History",
        [&](FrameGraph& builder, PassHandle pass, ViewSurfaceHistoryData& data)
        {
            RenderPassBuilder pb(builder, pass);
            data.depth = pb.read(depth, ResourceState::CopySource);
            data.normal = pb.read(normal, ResourceState::CopySource);
            data.historyDepth = pb.write(historyDepth, ResourceState::CopyDest);
            data.historyNormal = pb.write(historyNormal, ResourceState::CopyDest);
            data.frame = m_recording;
            pb.sideEffects();
        },
        [](const ViewSurfaceHistoryData& data, const FrameGraph& fg, RenderContext* ctx)
        {
            auto* depthTexture = fg.GetPhysicalTexture(data.depth);
            auto* normalTexture = fg.GetPhysicalTexture(data.normal);
            auto* historyDepthTexture = fg.GetPhysicalTexture(data.historyDepth);
            auto* historyNormalTexture = fg.GetPhysicalTexture(data.historyNormal);
            auto* cmd = ctx->GetCommandList();
            if (!cmd || !depthTexture || !normalTexture || !historyDepthTexture || !historyNormalTexture)
                return;
            cmd->copyTexture(historyDepthTexture, nvrhi::TextureSlice(), depthTexture, nvrhi::TextureSlice());
            cmd->copyTexture(historyNormalTexture, nvrhi::TextureSlice(), normalTexture, nvrhi::TextureSlice());
            data.frame->surfacesRecorded = true;
        });
}
}
