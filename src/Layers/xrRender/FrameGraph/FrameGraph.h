// xrRender/FrameGraph/FrameGraph.h
#pragma once

#include "FGTypes.h"
#include "FGResource.h"
#include "FGPass.h"
#include "RenderTargetRegistry.h"
#include "FGResourcePool.h"
#include "../RenderContext/RenderContext.h"
#include "../ResourceManager/FGResourceManager.h"
#include "xrCore/Profiler/CPUProfiler.h"

#include <cstdio>

class IRenderBackend;

namespace xray::profiler {
    class GPUProfiler;
}

namespace xray::render::framegraph {

// Forward declaration
namespace fg = xray::render::fg;

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  FRAMEGRAPH (MAIN CLASS)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

class FrameGraph {
public:
    FrameGraph(fg::RenderDevice* renderDevice);
    ~FrameGraph();

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  SETUP PHASE (CALLED EVERY FRAME)
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    // Create virtual resources
    VirtualResourceHandle CreateTexture(const char* name, const ResourceDesc& desc);
    VirtualResourceHandle CreateBuffer(const char* name, const ResourceDesc& desc);

    // Import external resources (e.g., backbuffer)
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

    // Create passes
    PassHandle AddPass(const char* name);

    // Declare resource access (alternative to fluent API)
    void PassRead(PassHandle pass, VirtualResourceHandle resource,
                 ResourceState state = ResourceState::ShaderResource);

    void PassWrite(PassHandle pass, VirtualResourceHandle resource,
                  ResourceState state = ResourceState::RenderTarget);

    void PassReadWrite(PassHandle pass, VirtualResourceHandle resource,
                      ResourceState state = ResourceState::UnorderedAccess);

    // Set pass execution callback
    void SetPassCallback(PassHandle pass, PassExecuteCallback callback);

    // Mark pass as async compute (runs on compute queue)
    void SetPassAsyncCompute(PassHandle pass);

    // Mark pass as having side effects (writes to external resources, prevents culling)
    void SetPassHasSideEffects(PassHandle pass);

    // Template method for lambda-based passes (Frostbite pattern)
    template<typename PassData>
    PassData& addCallbackPass(
        const char* name,
        std::function<void(FrameGraph&, PassHandle, PassData&)> setupFunc,
        std::function<void(const PassData&, const FrameGraph&, fg::RenderContext*)> executeFunc)
    {
        // Per-pass setup attribution: covers AddPass + PassData + setupFunc +
        // callback plumbing, as a "<pass> [setup]" zone under FG::SetupPasses
        const xray::profiler::ZoneInfo* setupZone = nullptr;
        if (xray::profiler::CPUProfiler::Instance().IsEnabled())
        {
            char setupZoneName[128];
            std::snprintf(setupZoneName, sizeof(setupZoneName), "%s [setup]", name);
            setupZone = xray::profiler::CPUProfiler::Instance().RegisterDynamicZone(setupZoneName);
        }
        xray::profiler::CPUZoneScope _zoneSetup(setupZone);

        PassHandle passHandle = AddPass(name);

        auto passData = std::make_shared<PassData>();

        setupFunc(*this, passHandle, *passData);

        PassExecuteCallback callback = [passData, executeFunc](fg::RenderContext& ctx, const FrameGraph& fg) {
            executeFunc(*passData, fg, &ctx);
        };

        SetPassCallback(passHandle, callback);

        return *passData;
    }

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  COMPILE PHASE (ONCE PER FRAME)
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    // Set RenderContext for execution (must be called before Execute)
    void SetRenderContext(fg::RenderContext* context) { m_context = context; }

    // Set GPUProfiler for per-pass timing (optional, can be nullptr)
    void SetGPUProfiler(xray::profiler::GPUProfiler* profiler) { m_gpuProfiler = profiler; }

    void SetAsyncCompute(nvrhi::ICommandList* computeCmdList, IRenderBackend* backend) {
        m_computeCommandList = computeCmdList;
        m_asyncComputeBackend = backend;
    }

    void Compile();

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  EXECUTE PHASE (AFTER COMPILE)
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    void Execute();

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  RESET (FOR NEXT FRAME)
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    // Reset per-frame execution state (keeps structure: RTs, passes, registry)
    void ResetForNextFrame();

    // Full reset - clears everything including structure
    void Reset();

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  QUERY (DURING EXECUTE)
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    // Get physical resource from virtual handle (for use in callbacks)
    nvrhi::ITexture* GetPhysicalTexture(VirtualResourceHandle handle) const;
    nvrhi::IBuffer* GetPhysicalBuffer(VirtualResourceHandle handle) const;

    // Get resource description
    const ResourceDesc& GetResourceDesc(VirtualResourceHandle handle) const;

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  RENDER TARGET REGISTRY
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    // Access RT registry
    RenderTargetRegistry& GetRTRegistry() { return m_rtRegistry; }
    const RenderTargetRegistry& GetRTRegistry() const { return m_rtRegistry; }

    // Convenience: lookup RT by name (forwards to registry)
    VirtualResourceHandle GetResourceByName(const char* name) const {
        return m_rtRegistry.GetRT(name);
    }

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  UTILITIES & DEBUGGING
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    void ExportVisualization(const char* htmlPath) const;
    void PrintStatistics() const;
    void PrintExecutionOrder() const;

    // Validation
    bool ValidateGraph() const;

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  STATISTICS
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    struct Statistics {
        // Compile stats
        u32 numPasses = 0;
        u32 numResources = 0;
        u32 numCulledPasses = 0;
        u32 numCulledResources = 0;
        float compileTimeMs = 0.0f;

        // Memory stats
        u64 totalMemoryAllocated = 0;
        u64 peakMemoryUsage = 0;
        u32 numAliasedResources = 0;
        u64 memoryReduced = 0;  // Saved by aliasing

        // Execute stats
        float executeTimeMs = 0.0f;
        float totalGPUTimeMs = 0.0f;

        // Per-pass timings (filled during execute)
        xr_map<shared_str, float> passTimings;
    };

    const Statistics& GetStatistics() const { return m_stats; }

private:
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  INTERNAL STATE
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    fg::RenderDevice* m_renderDevice;
    nvrhi::IDevice* m_device;
    fg::RenderContext* m_context = nullptr;
    xray::profiler::GPUProfiler* m_gpuProfiler = nullptr;
    nvrhi::ICommandList* m_computeCommandList = nullptr;
    IRenderBackend* m_asyncComputeBackend = nullptr;
    resources::FGResourceManager* m_resourceManager;
    xr_unique_ptr<FGResourcePool> m_resourcePool;

    // Render target registry
    RenderTargetRegistry m_rtRegistry;

    // Graph data
    xr_vector<ResourceNode> m_resources;
    xr_vector<PassNode> m_passes;

    // Compilation results
    xr_vector<PassNode*> m_sortedPasses;  // Execution order
    bool m_compiled = false;

    // Statistics
    Statistics m_stats;

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  COMPILATION PHASES
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    void BuildDependencyGraph();
    void TopologicalSort();
    void CullUnusedPasses();
    void ComputeResourceLifetimes();
    void AllocateResources();
    void InsertResourceBarriers();
    void OptimizeMemoryAliasing();

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  HELPER METHODS
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    void ExecutePass(PassNode* pass, nvrhi::ICommandList* cmdList);

    ResourceNode* GetResourceNode(VirtualResourceHandle handle);
    const ResourceNode* GetResourceNode(VirtualResourceHandle handle) const;

    PassNode* GetPassNode(PassHandle handle);
    const PassNode* GetPassNode(PassHandle handle) const;

    PassNode* FindProducer(VirtualResourceHandle resource);
    bool HasCyclicDependency() const;

    // Convert FrameGraph state to NVRHI state
    static nvrhi::ResourceStates ConvertToNVRHIState(ResourceState state);
};

} // namespace xray::render::framegraph
