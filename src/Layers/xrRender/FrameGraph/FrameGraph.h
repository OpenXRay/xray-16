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

    void SetPassHasSideEffects(PassHandle pass);

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

    const FrameArena::Stats& GetFrameArenaStats() const { return m_frameArena.GetStats(); }

    void SetAsyncCompute(nvrhi::ICommandList* computeCmdList, IRenderBackend* backend) {
        m_computeCommandList = computeCmdList;
        m_asyncComputeBackend = backend;
    }

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
        u64 peakMemoryUsage = 0;
        u32 numAliasedResources = 0;
        u64 memoryReduced = 0;

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
    nvrhi::ICommandList* m_computeCommandList = nullptr;
    IRenderBackend* m_asyncComputeBackend = nullptr;
    resources::FGResourceManager* m_resourceManager;
    xr_unique_ptr<FGResourcePool> m_resourcePool;

    RenderTargetRegistry m_rtRegistry;

    xr_vector<ResourceNode> m_resources;
    xr_vector<PassNode> m_passes;
    xr_vector<PassNode> m_passPool;
    FrameArena m_frameArena;

    xr_vector<u32> m_compileInDegree;
    xr_vector<PassNode*> m_compilePassWorklist;
    xr_vector<ResourceNode*> m_compileTransientResources;

    xr_vector<PassNode*> m_sortedPasses;
    bool m_compiled = false;

    Statistics m_stats;

    void BuildDependencyGraph();
    void TopologicalSort();
    void CullUnusedPasses();
    void ComputeResourceLifetimes();
    void AllocateResources();
    void InsertResourceBarriers();
    void OptimizeMemoryAliasing();

    void ExecutePass(PassNode* pass, nvrhi::ICommandList* cmdList);

    ResourceNode* GetResourceNode(VirtualResourceHandle handle);
    const ResourceNode* GetResourceNode(VirtualResourceHandle handle) const;

    PassNode* GetPassNode(PassHandle handle);
    const PassNode* GetPassNode(PassHandle handle) const;

    PassNode* FindProducer(VirtualResourceHandle resource);
    bool HasCyclicDependency() const;

    static nvrhi::ResourceStates ConvertToNVRHIState(ResourceState state);
};

}
