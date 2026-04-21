#pragma once

#include "FGTypes.h"
#include "FGResource.h"
#include "FGPass.h"
#include "RenderTargetRegistry.h"
#include "FGResourcePool.h"
#include "../RenderContext/RenderContext.h"
#include "../ResourceManager/FGResourceManager.h"

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

    void SetPassCallback(PassHandle pass, PassExecuteCallback callback);

    void SetPassAsyncCompute(PassHandle pass);

    void SetPassHasSideEffects(PassHandle pass);

    template<typename PassData>
    PassData& addCallbackPass(
        const char* name,
        std::function<void(FrameGraph&, PassHandle, PassData&)> setupFunc,
        std::function<void(const PassData&, const FrameGraph&, fg::RenderContext*)> executeFunc)
    {
        PassHandle passHandle = AddPass(name);

        auto passData = std::make_shared<PassData>();

        setupFunc(*this, passHandle, *passData);

        PassExecuteCallback callback = [passData, executeFunc](fg::RenderContext& ctx, const FrameGraph& fg) {
            executeFunc(*passData, fg, &ctx);
        };

        SetPassCallback(passHandle, callback);

        return *passData;
    }

    void SetRenderContext(fg::RenderContext* context) { m_context = context; }

    void SetGPUProfiler(xray::profiler::GPUProfiler* profiler) { m_gpuProfiler = profiler; }

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
