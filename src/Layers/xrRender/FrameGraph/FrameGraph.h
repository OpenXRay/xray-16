#pragma once

#include "FGTypes.h"
#include "FGResource.h"
#include "FGPass.h"
#include "RenderTargetRegistry.h"
#include "FGResourcePool.h"
#include "../RenderContext/RenderContext.h"
#include "../ResourceManager/ModernResourceManager.h"

namespace xray::render::framegraph {

namespace ng = xray::render::ng;

class FrameGraph {
public:
    FrameGraph(nvrhi::IDevice* device, resources::ModernResourceManager* resourceManager);
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

    void SetRenderContext(ng::RenderContext* context) { m_context = context; }

    void Compile();

    void Execute();

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

    nvrhi::IDevice* m_device;
    ng::RenderContext* m_context = nullptr;
    resources::ModernResourceManager* m_resourceManager;
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

    ResourceNode* GetResourceNode(VirtualResourceHandle handle);
    const ResourceNode* GetResourceNode(VirtualResourceHandle handle) const;

    PassNode* GetPassNode(PassHandle handle);
    const PassNode* GetPassNode(PassHandle handle) const;

    PassNode* FindProducer(VirtualResourceHandle resource);
    bool HasCyclicDependency() const;

    static nvrhi::ResourceStates ConvertToNVRHIState(ResourceState state);
};

}
