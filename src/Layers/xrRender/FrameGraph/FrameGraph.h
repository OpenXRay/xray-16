#pragma once

#include "FGTypes.h"
#include "FGResource.h"
#include "FGPass.h"
#include "FGFrameArena.h"
#include "RenderTargetRegistry.h"
#include "FGResourcePool.h"
#include "../RenderContext/RenderContext.h"
#include "../ResourceManager/FGResourceManager.h"
#include "xrCore/Profiler/CPUProfiler.h"

#include <cstdio>
#include <type_traits>

class IRenderBackend;

namespace xray::profiler {
    class GPUProfiler;
}

namespace xray::render::framegraph {

namespace fg = xray::render::fg;

class FrameGraph {
public:
    FrameGraph(fg::RenderDevice* renderDevice);
    ~FrameGraph();

    VirtualResourceHandle CreateTexture(const char* name, const ResourceDesc& desc);
    VirtualResourceHandle CreateBuffer(const char* name, const ResourceDesc& desc);

    VirtualResourceHandle ImportTexture(
        const char* name,
        nvrhi::ITexture* physicalTexture,
        const ResourceDesc& desc
    );

    VirtualResourceHandle ImportBuffer(
        const char* name,
        nvrhi::IBuffer* physicalBuffer,
        const ResourceDesc& desc
    );

    PassHandle AddPass(const char* name);

    void PassRead(PassHandle pass, VirtualResourceHandle resource,
                 ResourceState state = ResourceState::ShaderResource);

    void PassWrite(PassHandle pass, VirtualResourceHandle resource,
                  ResourceState state = ResourceState::RenderTarget);

    void PassReadWrite(PassHandle pass, VirtualResourceHandle resource,
                      ResourceState state = ResourceState::UnorderedAccess);

    void SetPassCallbackPtr(PassHandle pass, IPassCallback* callback);

    template <typename F>
    void SetPassCallback(PassHandle pass, F&& callback)
    {
        SetPassCallbackPtr(pass,
            m_frameArena.Make<PassCallback<std::decay_t<F>>>(std::forward<F>(callback)));
    }

    void SetPassAsyncCompute(PassHandle pass);

    void SetSegmentBeginCallbackPtr(IPassCallback* callback) { m_segmentBeginCallback = callback; }

    template <typename F>
    void SetSegmentBeginCallback(F&& callback)
    {
        SetSegmentBeginCallbackPtr(
            m_frameArena.Make<PassCallback<std::decay_t<F>>>(std::forward<F>(callback)));
    }

    void SetPassHasSideEffects(PassHandle pass);

    void SetPresentTarget(VirtualResourceHandle handle);

    template <typename PassData, typename Setup, typename Execute>
    PassData& addCallbackPass(const char* name, Setup&& setupFunc, Execute&& executeFunc)
    {
        const xray::profiler::ZoneInfo* setupZone = nullptr;
        if (xray::profiler::CPUProfiler::Instance().IsSamplingFrame())
        {
            char setupZoneName[128];
            std::snprintf(setupZoneName, sizeof(setupZoneName), "%s [setup]", name);
            setupZone = xray::profiler::CPUProfiler::Instance().RegisterDynamicZone(setupZoneName);
        }
        xray::profiler::CPUZoneScope _zoneSetup(setupZone);

        PassHandle passHandle = AddPass(name);

        auto* pass = m_frameArena.Make<DataPassCallback<PassData, std::decay_t<Execute>>>(
            std::forward<Execute>(executeFunc));

        setupFunc(*this, passHandle, pass->data);

        SetPassCallbackPtr(passHandle, pass);

        return pass->data;
    }

    void SetRenderContext(fg::RenderContext* context) { m_context = context; }

    void SetGPUProfiler(xray::profiler::GPUProfiler* profiler) { m_gpuProfiler = profiler; }
    xray::profiler::GPUProfiler* GetGPUProfiler() const { return m_gpuProfiler; }

    const FrameArena::Stats& GetFrameArenaStats() const { return m_frameArena.GetStats(); }

    void SetAsyncComputeBackend(IRenderBackend* backend) { m_backend = backend; }

    void Compile();

    void Execute();

    void ResetForNextFrame();

    void Reset();

    nvrhi::ITexture* GetPhysicalTexture(VirtualResourceHandle handle) const;
    nvrhi::IBuffer* GetPhysicalBuffer(VirtualResourceHandle handle) const;

    const ResourceDesc& GetResourceDesc(VirtualResourceHandle handle) const;

    RenderTargetRegistry& GetRTRegistry() { return m_rtRegistry; }
    const RenderTargetRegistry& GetRTRegistry() const { return m_rtRegistry; }

    VirtualResourceHandle GetResourceByName(const char* name) const {
        return m_rtRegistry.GetRT(name);
    }

    void ExportVisualization(const char* htmlPath) const;
    void PrintStatistics() const;
    void PrintExecutionOrder() const;

    bool ValidateGraph() const;

    struct Statistics {
        u32 numPasses = 0;
        u32 numResources = 0;
        u32 numCulledPasses = 0;
        u32 numCulledResources = 0;
        float compileTimeMs = 0.0f;

        u64 totalMemoryAllocated = 0;

        float executeTimeMs = 0.0f;
        float totalGPUTimeMs = 0.0f;

        xr_map<shared_str, float> passTimings;
    };

    const Statistics& GetStatistics() const { return m_stats; }

private:

    fg::RenderDevice* m_renderDevice;
    nvrhi::IDevice* m_device;
    fg::RenderContext* m_context = nullptr;
    xray::profiler::GPUProfiler* m_gpuProfiler = nullptr;
    IRenderBackend* m_backend = nullptr;
    IPassCallback* m_segmentBeginCallback = nullptr;
    resources::FGResourceManager* m_resourceManager;
    xr_unique_ptr<FGResourcePool> m_resourcePool;

    RenderTargetRegistry m_rtRegistry;

    xr_vector<ResourceNode> m_resources;
    xr_vector<PassNode> m_passes;
    xr_vector<PassNode> m_passPool;
    FrameArena m_frameArena;

    struct ReaderLink {
        PassNode* pass;
        u32 next;
    };

    xr_vector<ReaderLink> m_compileReaderLinks;
    xr_vector<PassNode*> m_compilePassWorklist;

    xr_vector<PassNode*> m_sortedPasses;

    static constexpr u32 MAX_SEGMENT_WAITS = 8;

    struct Segment {
        PassQueue queue = PassQueue::Graphics;
        u32 firstPass = INVALID_INDEX;
        u32 lastPass = INVALID_INDEX;
        u32 waits[MAX_SEGMENT_WAITS] = {};
        u32 numWaits = 0;
        u32 token = 0;
    };

    xr_vector<Segment> m_segments;
    bool m_compiled = false;
    VirtualResourceHandle m_presentTarget;
    u32 m_generation = 1;
    const PassNode* m_currentPass = nullptr;

    Statistics m_stats;

    void BuildDependencyGraph();
    void ValidateAsyncPasses();
    void CullUnusedPasses();
    void ResolveUsage();
    void ComputeResourceLifetimes();
    void BuildLifetimeLists();
    void AssignSegments();
    void Devirtualize(ResourceNode& resource);
    void Destroy(ResourceNode& resource);
    void InsertResourceBarriers();

    void ExecutePass(PassNode* pass, nvrhi::ICommandList* cmdList);
    void BeginSegment();
    bool IsAsyncComputeActive() const;

    ResourceNode* GetResourceNode(VirtualResourceHandle handle);
    const ResourceNode* GetResourceNode(VirtualResourceHandle handle) const;

    bool PassDeclaresResource(const PassNode& pass, VirtualResourceHandle handle) const;

    PassNode* GetPassNode(PassHandle handle);
    const PassNode* GetPassNode(PassHandle handle) const;

    static nvrhi::ResourceStates ConvertToNVRHIState(ResourceState state);
};

}
