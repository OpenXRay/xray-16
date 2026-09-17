// xrRender/FrameGraph/FrameGraph.h
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
    void SetPassCallbackPtr(PassHandle pass, IPassCallback* callback);

    template <typename F>
    void SetPassCallback(PassHandle pass, F&& callback)
    {
        SetPassCallbackPtr(pass,
            m_frameArena.Make<PassCallback<std::decay_t<F>>>(std::forward<F>(callback)));
    }

    // Mark pass as async compute (runs on compute queue)
    void SetPassAsyncCompute(PassHandle pass);

    void SetSegmentBeginCallbackPtr(IPassCallback* callback) { m_segmentBeginCallback = callback; }

    template <typename F>
    void SetSegmentBeginCallback(F&& callback)
    {
        SetSegmentBeginCallbackPtr(
            m_frameArena.Make<PassCallback<std::decay_t<F>>>(std::forward<F>(callback)));
    }

    // Mark pass as having side effects (writes to external resources, prevents culling)
    void SetPassHasSideEffects(PassHandle pass);

    void SetPresentTarget(VirtualResourceHandle handle);

    // Template method for lambda-based passes (Frostbite pattern)
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

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  COMPILE PHASE (ONCE PER FRAME)
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    // Set RenderContext for execution (must be called before Execute)
    void SetRenderContext(fg::RenderContext* context) { m_context = context; }

    // Set GPUProfiler for per-pass timing (optional, can be nullptr)
    void SetGPUProfiler(xray::profiler::GPUProfiler* profiler) { m_gpuProfiler = profiler; }
    xray::profiler::GPUProfiler* GetGPUProfiler() const { return m_gpuProfiler; }

    const FrameArena::Stats& GetFrameArenaStats() const { return m_frameArena.GetStats(); }

    void SetAsyncComputeBackend(IRenderBackend* backend) { m_backend = backend; }

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
    IRenderBackend* m_backend = nullptr;
    IPassCallback* m_segmentBeginCallback = nullptr;
    resources::FGResourceManager* m_resourceManager;
    xr_unique_ptr<FGResourcePool> m_resourcePool;

    // Render target registry
    RenderTargetRegistry m_rtRegistry;

    // Graph data
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

    // Compilation results
    xr_vector<PassNode*> m_sortedPasses;  // Execution order

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

    // Statistics
    Statistics m_stats;

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  COMPILATION PHASES
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    void BuildDependencyGraph();
    void KeepDepthReadersOnGraphicsQueue();
    void ValidateAsyncPasses();
    void CullUnusedPasses();
    void ResolveUsage();
    void ComputeResourceLifetimes();
    void BuildLifetimeLists();
    void AssignSegments();
    void Devirtualize(ResourceNode& resource);
    void Destroy(ResourceNode& resource);
    void InsertResourceBarriers();

    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
    //  HELPER METHODS
    // PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

    void ExecutePass(PassNode* pass, nvrhi::ICommandList* cmdList);
    void BeginSegment();
    bool IsAsyncComputeActive() const;

    ResourceNode* GetResourceNode(VirtualResourceHandle handle);
    const ResourceNode* GetResourceNode(VirtualResourceHandle handle) const;

    bool PassDeclaresResource(const PassNode& pass, VirtualResourceHandle handle) const;

    PassNode* GetPassNode(PassHandle handle);
    const PassNode* GetPassNode(PassHandle handle) const;

    // Convert FrameGraph state to NVRHI state
    static nvrhi::ResourceStates ConvertToNVRHIState(ResourceState state);
};

} // namespace xray::render::framegraph
