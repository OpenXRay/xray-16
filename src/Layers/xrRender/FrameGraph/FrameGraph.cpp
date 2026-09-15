// xrRender/FrameGraph/FrameGraph.cpp
#include "stdafx.h"
#include "FrameGraph.h"
#include "../RenderContext/RenderDevice.h"
#include "../Profiler/GPUProfiler.h"
#include "xrEngine/IRenderBackend.h"

namespace xray::render::framegraph {

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  CONSTRUCTOR / DESTRUCTOR
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

FrameGraph::FrameGraph(fg::RenderDevice* renderDevice)
    : m_renderDevice(renderDevice)
    , m_device(renderDevice->GetNVRHIDevice())
    , m_resourceManager(renderDevice->GetFGResourceManager())
{
    VERIFY(renderDevice != nullptr);
    VERIFY(m_device != nullptr);

    // Create resource pool for aliasing (if ResourceManager available)
    if (m_resourceManager) {
        m_resourcePool = xr_make_unique<FGResourcePool>(m_resourceManager);
    }

    // Reserve space to avoid reallocations
    m_resources.reserve(256);
    m_passes.reserve(128);
    m_sortedPasses.reserve(128);
    m_compileReaderLinks.reserve(512);
}

FrameGraph::~FrameGraph() {
    // Clean up resources
    Reset();

    Msg("* [FrameGraph] Destroyed");
};

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  SETUP PHASE - RESOURCE CREATION
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

VirtualResourceHandle FrameGraph::CreateTexture(const char* name, const ResourceDesc& desc) {
    R_ASSERT2(!m_compiled, "Cannot create resources after compile");
    R_ASSERT2(desc.type != ResourceDesc::Type::Buffer, "Use CreateBuffer for buffers");

    ResourceNode& node = m_resources.emplace_back(desc);
    node.handle.index = static_cast<u32>(m_resources.size() - 1);
    node.handle.generation = m_generation;

    return node.handle;
}

VirtualResourceHandle FrameGraph::CreateBuffer(const char* name, const ResourceDesc& desc) {
    R_ASSERT2(!m_compiled, "Cannot create resources after compile");
    R_ASSERT2(desc.type == ResourceDesc::Type::Buffer, "Use CreateTexture for textures");

    ResourceNode& node = m_resources.emplace_back(desc);
    node.handle.index = static_cast<u32>(m_resources.size() - 1);
    node.handle.generation = m_generation;

    return node.handle;
}

VirtualResourceHandle FrameGraph::ImportTexture(
    const char* name,
    nvrhi::ITexture* physicalTexture,
    const ResourceDesc& desc
) {
    R_ASSERT2(!m_compiled, "Cannot import resources after compile");
    R_ASSERT(physicalTexture != nullptr);
    R_ASSERT2(desc.type != ResourceDesc::Type::Buffer, "Use ImportBuffer for buffers");

    ResourceNode& node = m_resources.emplace_back(desc);
    node.handle.index = static_cast<u32>(m_resources.size() - 1);
    node.handle.generation = m_generation;
    node.nvrhiTexture = physicalTexture;
    node.isAllocated = true;
    node.isPersistent = true;
    node.desc.isImported = true;

    return node.handle;
}

VirtualResourceHandle FrameGraph::ImportBuffer(
    const char* name,
    nvrhi::IBuffer* physicalBuffer,
    const ResourceDesc& desc
) {
    R_ASSERT2(!m_compiled, "Cannot import resources after compile");
    R_ASSERT(physicalBuffer != nullptr);
    R_ASSERT2(desc.type == ResourceDesc::Type::Buffer, "Use ImportTexture for textures");

    ResourceNode& node = m_resources.emplace_back(desc);
    node.handle.index = static_cast<u32>(m_resources.size() - 1);
    node.handle.generation = m_generation;
    node.nvrhiBuffer = physicalBuffer;
    node.isAllocated = true;
    node.isPersistent = true;
    node.desc.isImported = true;

    return node.handle;
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  SETUP PHASE - PASS CREATION
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

PassHandle FrameGraph::AddPass(const char* name) {
    R_ASSERT2(!m_compiled, "Cannot add passes after compile");

    PassHandle handle;
    handle.index = static_cast<u32>(m_passes.size());

    if (!m_passPool.empty()) {
        m_passes.push_back(std::move(m_passPool.back()));
        m_passPool.pop_back();
        m_passes.back().Recycle(name);
    } else {
        m_passes.emplace_back(name);
    }
    m_passes.back().handle = handle;

    return handle;
}

void FrameGraph::PassRead(PassHandle pass, VirtualResourceHandle resource, ResourceState state) {
    R_ASSERT2(!m_compiled, "Cannot declare resources after compile");
    PassNode* passNode = GetPassNode(pass);
    R_ASSERT(passNode != nullptr);
    R_ASSERT3(GetResourceNode(resource) != nullptr, "framegraph pass declares an invalid resource", passNode->name.c_str());

    passNode->Read(resource, state);
}

void FrameGraph::PassWrite(PassHandle pass, VirtualResourceHandle resource, ResourceState state) {
    R_ASSERT2(!m_compiled, "Cannot declare resources after compile");
    PassNode* passNode = GetPassNode(pass);
    R_ASSERT(passNode != nullptr);
    R_ASSERT3(GetResourceNode(resource) != nullptr, "framegraph pass declares an invalid resource", passNode->name.c_str());

    passNode->Write(resource, state);
}

void FrameGraph::PassReadWrite(PassHandle pass, VirtualResourceHandle resource, ResourceState state) {
    R_ASSERT2(!m_compiled, "Cannot declare resources after compile");
    PassNode* passNode = GetPassNode(pass);
    R_ASSERT(passNode != nullptr);
    R_ASSERT3(GetResourceNode(resource) != nullptr, "framegraph pass declares an invalid resource", passNode->name.c_str());

    passNode->ReadWrite(resource, state);
}

void FrameGraph::SetPassCallbackPtr(PassHandle pass, IPassCallback* callback) {
    PassNode* passNode = GetPassNode(pass);
    VERIFY(passNode != nullptr);

    passNode->executeCallback = callback;
}

void FrameGraph::SetPassAsyncCompute(PassHandle pass) {
    PassNode* passNode = GetPassNode(pass);
    VERIFY(passNode != nullptr);

    passNode->isAsync = true;
    passNode->isCompute = true;
    passNode->isGraphics = false;
}

void FrameGraph::SetPassHasSideEffects(PassHandle pass) {
    PassNode* passNode = GetPassNode(pass);
    VERIFY(passNode != nullptr);

    passNode->hasSideEffects = true;
}

void FrameGraph::SetPresentTarget(VirtualResourceHandle handle) {
    R_ASSERT2(!m_compiled, "Cannot set present target after compile");

    const ResourceNode* node = GetResourceNode(handle);
    R_ASSERT(node != nullptr);
    R_ASSERT(node->desc.isImported && node->nvrhiTexture);

    m_presentTarget = handle;
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  COMPILE PHASE (STUB FOR NOW)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void FrameGraph::Compile() {
    ZoneScoped;

    R_ASSERT2(!m_compiled, "Already compiled");

    // Phase 1: Build dependency graph from resource accesses
    {
        ZoneScopedN("Compile::BuildDependencyGraph");
        BuildDependencyGraph();
    }

    ValidateAsyncPasses();

    // Phase 2: Establish execution order (declaration order)
    {
        ZoneScopedN("Compile::OrderPasses");
        m_sortedPasses.clear();
        m_sortedPasses.reserve(m_passes.size());
        for (auto& pass : m_passes) {
            pass.executionOrder = pass.handle.index;
            m_sortedPasses.push_back(&pass);
        }
    }

    // Phase 3: Remove unused passes
    {
        ZoneScopedN("Compile::CullUnusedPasses");
        CullUnusedPasses();
    }

    {
        ZoneScopedN("Compile::ResolveUsage");
        ResolveUsage();
    }

    {
        ZoneScopedN("Compile::ComputeResourceLifetimes");
        ComputeResourceLifetimes();
    }

    {
        ZoneScopedN("Compile::BuildLifetimeLists");
        BuildLifetimeLists();
    }

    m_compiled = true;
    m_stats.numPasses = static_cast<u32>(m_passes.size());
    m_stats.numResources = static_cast<u32>(m_resources.size());
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  EXECUTE PHASE (STUB FOR NOW)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void FrameGraph::ExecutePass(PassNode* pass, nvrhi::ICommandList* cmdList) {
    cmdList->beginMarker(pass->name.c_str());

    xray::profiler::CPUZoneScope _zonePass(
        xray::profiler::CPUProfiler::Instance().RegisterDynamicZone(pass->name.c_str()));

    if (m_gpuProfiler)
        m_gpuProfiler->BeginPass(cmdList, pass->name.c_str(), pass->isAsync);

    m_currentPass = pass;
    (*pass->executeCallback)(*m_context, *this);
    m_currentPass = nullptr;

    if (m_gpuProfiler)
        m_gpuProfiler->EndPass(cmdList, pass->name.c_str());

    cmdList->endMarker();
}

void FrameGraph::Execute() {
    ZoneScoped;

    R_ASSERT2(m_compiled, "Must compile before execute");
    R_ASSERT2(m_context != nullptr, "RenderContext required for execution");
    R_ASSERT2(m_renderDevice != nullptr, "RenderDevice required for execution");

    nvrhi::ICommandList* graphicsCmdList = m_context->GetCommandList();
    R_ASSERT(graphicsCmdList != nullptr);

    bool hasAsyncCompute = m_computeCommandList != nullptr && m_asyncComputeBackend != nullptr;
    R_ASSERT2(!hasAsyncCompute || m_computeCommandList != graphicsCmdList,
        "async compute must record into a separate command list");

    // Count async passes to check if we have any work for the compute queue
    u32 asyncPassCount = 0;
    if (hasAsyncCompute) {
        for (PassNode* pass : m_sortedPasses) {
            if (!pass->culled && pass->executeCallback && pass->isAsync)
                asyncPassCount++;
        }
        if (asyncPassCount == 0)
            hasAsyncCompute = false;
    }

    // ═══════════════════════════════════════════════════════
    //  PHASE A: Record async compute passes
    // ═══════════════════════════════════════════════════════
    if (hasAsyncCompute) {
        m_computeCommandList->open();
        m_context->SetOverrideCommandList(m_computeCommandList);

        for (PassNode* pass : m_sortedPasses) {
            if (pass->culled || !pass->executeCallback || !pass->isAsync)
                continue;
            R_ASSERT(pass->devirtualize.empty() && pass->destroy.empty());
            ExecutePass(pass, m_computeCommandList);
        }

        m_context->SetOverrideCommandList(nullptr);
        m_computeCommandList->close();
        m_asyncComputeBackend->QueueComputeCommandList(m_computeCommandList);
    }

    bool syncInserted = false;

    for (PassNode* pass : m_sortedPasses) {
        if (pass->culled)
            continue;

        if (hasAsyncCompute && pass->isAsync)
            continue;

        if (!syncInserted && hasAsyncCompute) {
            bool dependsOnAsync = false;
            for (const auto& dep : pass->dependsOn) {
                if (dep.pass->isAsync) {
                    dependsOnAsync = true;
                    break;
                }
            }
            if (dependsOnAsync) {
                m_asyncComputeBackend->QueueWaitForCompute();
                syncInserted = true;
            }
        }

        for (ResourceNode* resource : pass->devirtualize)
            Devirtualize(*resource);

        if (pass->executeCallback)
            ExecutePass(pass, graphicsCmdList);

        for (ResourceNode* resource : pass->destroy)
            Destroy(*resource);
    }

    if (m_presentTarget.is_valid()) {
        ResourceNode* target = GetResourceNode(m_presentTarget);
        R_ASSERT(target != nullptr && target->nvrhiTexture);

        graphicsCmdList->setTextureState(
            target->nvrhiTexture.Get(),
            nvrhi::AllSubresources,
            nvrhi::ResourceStates::Present
        );
    }

    graphicsCmdList->commitBarriers();
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  QUERY METHODS
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

nvrhi::ITexture* FrameGraph::GetPhysicalTexture(VirtualResourceHandle handle) const {
    const ResourceNode* node = GetResourceNode(handle);
    if (!node)
        return nullptr;

    R_ASSERT3(node->isAllocated && node->nvrhiTexture,
        "framegraph texture accessed outside its lifetime", node->desc.debugName.c_str());

#ifdef DEBUG
    if (m_currentPass) {
        VERIFY4(PassDeclaresResource(*m_currentPass, handle),
            "framegraph pass fetched a texture it never declared",
            m_currentPass->name.c_str(),
            node->desc.debugName.c_str());
    }
#endif

    return node->nvrhiTexture;
}

nvrhi::IBuffer* FrameGraph::GetPhysicalBuffer(VirtualResourceHandle handle) const {
    const ResourceNode* node = GetResourceNode(handle);
    if (!node)
        return nullptr;

    R_ASSERT3(node->isAllocated && node->nvrhiBuffer,
        "framegraph buffer accessed outside its lifetime", node->desc.debugName.c_str());

#ifdef DEBUG
    if (m_currentPass) {
        VERIFY4(PassDeclaresResource(*m_currentPass, handle),
            "framegraph pass fetched a buffer it never declared",
            m_currentPass->name.c_str(),
            node->desc.debugName.c_str());
    }
#endif

    return node->nvrhiBuffer;
}

const ResourceDesc& FrameGraph::GetResourceDesc(VirtualResourceHandle handle) const {
    const ResourceNode* node = GetResourceNode(handle);
    R_ASSERT(node != nullptr);
    return node->desc;
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  RESET
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

// ════════════════════════════════════════════════════════════
//  RESET FOR NEXT FRAME (KEEPS STRUCTURE)
// ════════════════════════════════════════════════════════════

void FrameGraph::ResetForNextFrame() {
    ZoneScopedN("FG::ResetForNextFrame");

    for (auto& resource : m_resources) {
        if (!resource.desc.isImported)
            Destroy(resource);
    }
    if (m_resourcePool)
        m_resourcePool->Tick();

    m_compileReaderLinks.clear();
    m_compilePassWorklist.clear();

    // Clear graph structure:
    for (auto& pass : m_passes)
        m_passPool.push_back(std::move(pass));
    m_passes.clear();
    m_resources.clear();
    m_sortedPasses.clear();
    m_compiled = false;
    m_presentTarget = VirtualResourceHandle();
    m_currentPass = nullptr;
    if (++m_generation == 0)
        m_generation = 1;

    // Clear render target registry
    m_rtRegistry.Clear();

    // Reset per-frame statistics
    m_stats = Statistics();

    m_frameArena.Reset();
}

// ════════════════════════════════════════════════════════════
//  FULL RESET (CLEARS EVERYTHING INCLUDING STRUCTURE)
// ════════════════════════════════════════════════════════════

void FrameGraph::Reset() {
    for (auto& resource : m_resources) {
        if (!resource.desc.isImported)
            Destroy(resource);
    }
    if (m_resourcePool)
        m_resourcePool->Reset();

    m_compileReaderLinks.clear();
    m_compilePassWorklist.clear();

    // Clear state
    m_resources.clear();
    m_passes.clear();
    m_passPool.clear();
    m_sortedPasses.clear();
    m_rtRegistry.Clear();  // Clear RT registry
    m_compiled = false;
    m_presentTarget = VirtualResourceHandle();
    m_currentPass = nullptr;
    if (++m_generation == 0)
        m_generation = 1;
    m_frameArena.Reset();

    // Reset statistics (don't use memset - contains non-trivial types!)
    m_stats.numPasses = 0;
    m_stats.numResources = 0;
    m_stats.numCulledPasses = 0;
    m_stats.numCulledResources = 0;
    m_stats.compileTimeMs = 0.0f;
    m_stats.totalMemoryAllocated = 0;
    m_stats.executeTimeMs = 0.0f;
    m_stats.totalGPUTimeMs = 0.0f;
    m_stats.passTimings.clear();  // Properly clear the map
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  UTILITIES & DEBUGGING (STUBS)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void FrameGraph::ExportVisualization(const char* htmlPath) const {
    // TODO: Implement graph visualization export
    Msg("~ [FrameGraph] ExportVisualization not yet implemented");
}

void FrameGraph::PrintStatistics() const {
    Msg("========================================");
    Msg("  FrameGraph Statistics");
    Msg("========================================");

    // Passes
    Msg("Passes:");
    Msg("  Total: %u", m_stats.numPasses);
    Msg("  Executed: %u", m_stats.numPasses - m_stats.numCulledPasses);
    Msg("  Culled: %u", m_stats.numCulledPasses);

    // Resources
    Msg("Resources:");
    Msg("  Total: %u", m_stats.numResources);
    Msg("  Allocated: %u", m_stats.numResources - m_stats.numCulledResources);
    Msg("  Culled: %u", m_stats.numCulledResources);

    // Memory
    if (m_stats.totalMemoryAllocated > 0) {
        Msg("Memory:");
        Msg("  Total allocated: %.2f MB", m_stats.totalMemoryAllocated / (1024.0f * 1024.0f));
    }

    // Timing
    Msg("Timing:");
    Msg("  Compile: %.2f ms", m_stats.compileTimeMs);
    Msg("  Execute (CPU): %.2f ms", m_stats.executeTimeMs);
    if (m_stats.totalGPUTimeMs > 0.0f) {
        Msg("  Execute (GPU): %.2f ms", m_stats.totalGPUTimeMs);
    }

    // Per-pass timings
    if (!m_stats.passTimings.empty()) {
        Msg("Pass Timings:");

        // Sort by time (descending)
        xr_vector<std::pair<shared_str, float>> sorted;
        for (const auto& pair : m_stats.passTimings) {
            sorted.push_back(pair);
        }
        std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        for (const auto& pair : sorted) {
            float percent = (m_stats.totalGPUTimeMs > 0.0f)
                ? (100.0f * pair.second / m_stats.totalGPUTimeMs)
                : 0.0f;
            Msg("  %-30s: %6.2f ms (%4.1f%%)",
                pair.first.c_str(),
                pair.second,
                percent);
        }
    }

    Msg("========================================");
}

void FrameGraph::PrintExecutionOrder() const {
    Msg("========================================");
    Msg("  FrameGraph Execution Order");
    Msg("========================================");

    for (u32 i = 0; i < m_sortedPasses.size(); i++) {
        const PassNode* pass = m_sortedPasses[i];

        Msg("[%2u] %-30s (%u deps)",
            i,
            pass->name.c_str(),
            static_cast<u32>(pass->dependsOn.size()));

        // Show resource accesses
        if (!pass->resourceAccesses.empty()) {
            for (const auto& access : pass->resourceAccesses) {
                const ResourceNode* resource = GetResourceNode(access.resource);
                const char* accessType = "?";

                if (access.IsWrite()) {
                    accessType = access.IsRead() ? "R/W" : "W";
                } else {
                    accessType = "R";
                }

                Msg("     [%s] %s (%s)",
                    accessType,
                    resource->desc.debugName.c_str(),
                    ResourceStateToString(access.state));
            }
        }
    }

    Msg("========================================");
}

bool FrameGraph::ValidateGraph() const {
    bool valid = true;

    // Check 1: All passes have callbacks
    for (const auto& pass : m_passes) {
        if (pass.culled) continue;

        if (!pass.executeCallback) {
            valid = false;
        }
    }

    // Check 2: All resources used by non-culled passes are allocated
    for (const auto& pass : m_passes) {
        if (pass.culled) continue;

        for (const auto& access : pass.resourceAccesses) {
            const ResourceNode* resource = GetResourceNode(access.resource);
            if (!resource) {
                valid = false;
                continue;
            }

            if (!resource->isAllocated && !resource->desc.isImported) {
                valid = false;
            }
        }
    }

    // Check 3: Execution order is declaration order, so cycles are impossible

    // Check 4: Imported resources have valid handles
    for (const auto& resource : m_resources) {
        if (resource.desc.isImported) {
            if (resource.desc.type == ResourceDesc::Type::Buffer) {
                if (!resource.nvrhiBuffer) {
                    valid = false;
                }
            } else {
                if (!resource.nvrhiTexture) {
                    valid = false;
                }
            }
        }
    }

    if (!valid) {
        Msg("! [FrameGraph] Validation FAILED");
    }

    return valid;
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  HELPER METHODS
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

ResourceNode* FrameGraph::GetResourceNode(VirtualResourceHandle handle) {
    if (!handle.is_valid())
        return nullptr;
    R_ASSERT2(handle.generation == m_generation, "stale framegraph resource handle");
    R_ASSERT2(handle.index < m_resources.size(), "framegraph resource handle is out of bounds");
    return &m_resources[handle.index];
}

const ResourceNode* FrameGraph::GetResourceNode(VirtualResourceHandle handle) const {
    if (!handle.is_valid())
        return nullptr;
    R_ASSERT2(handle.generation == m_generation, "stale framegraph resource handle");
    R_ASSERT2(handle.index < m_resources.size(), "framegraph resource handle is out of bounds");
    return &m_resources[handle.index];
}

bool FrameGraph::PassDeclaresResource(const PassNode& pass, VirtualResourceHandle handle) const {
    for (const auto& access : pass.resourceAccesses) {
        if (access.resource.index == handle.index)
            return true;
    }
    return false;
}

PassNode* FrameGraph::GetPassNode(PassHandle handle) {
    if (!handle.is_valid() || handle.index >= m_passes.size()) {
        return nullptr;
    }
    return &m_passes[handle.index];
}

const PassNode* FrameGraph::GetPassNode(PassHandle handle) const {
    if (!handle.is_valid() || handle.index >= m_passes.size()) {
        return nullptr;
    }
    return &m_passes[handle.index];
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  COMPILATION PHASES (STUBS - TO BE IMPLEMENTED)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void FrameGraph::BuildDependencyGraph() {
    for (auto& pass : m_passes) {
        pass.dependsOn.clear();
    }

    for (auto& resource : m_resources) {
        resource.lastWriter = nullptr;
        resource.readersHead = INVALID_INDEX;
    }

    auto& readerLinks = m_compileReaderLinks;
    readerLinks.clear();

    auto addEdge = [](PassNode& pass, PassNode* producer, EdgeKind kind) {
        if (producer == &pass)
            return;
        if (PassDependency* existing = pass.FindDependency(producer)) {
            if (kind == EdgeKind::ReadAfterWrite)
                existing->kind = EdgeKind::ReadAfterWrite;
            return;
        }
        pass.dependsOn.push_back(PassDependency{producer, kind});
    };

    for (auto& pass : m_passes) {
        for (const auto& access : pass.resourceAccesses) {
            if (!access.IsRead()) continue;

            ResourceNode* resource = GetResourceNode(access.resource);
            VERIFY(resource != nullptr);
            if (resource->lastWriter)
                addEdge(pass, resource->lastWriter, EdgeKind::ReadAfterWrite);
        }

        for (const auto& access : pass.resourceAccesses) {
            if (!access.IsWrite()) continue;

            ResourceNode* resource = GetResourceNode(access.resource);
            VERIFY(resource != nullptr);
            if (resource->lastWriter)
                addEdge(pass, resource->lastWriter, EdgeKind::WriteAfterWrite);

            for (u32 link = resource->readersHead; link != INVALID_INDEX; link = readerLinks[link].next)
                addEdge(pass, readerLinks[link].pass, EdgeKind::WriteAfterRead);
            resource->readersHead = INVALID_INDEX;
        }

        for (const auto& access : pass.resourceAccesses) {
            ResourceNode* resource = GetResourceNode(access.resource);
            VERIFY(resource != nullptr);
            if (access.IsRead()) {
                readerLinks.push_back(ReaderLink{&pass, resource->readersHead});
                resource->readersHead = static_cast<u32>(readerLinks.size() - 1);
            }
            if (access.IsWrite())
                resource->lastWriter = &pass;
        }
    }
}

void FrameGraph::ValidateAsyncPasses() {
    for (const auto& pass : m_passes) {
        if (!pass.isAsync)
            continue;

        for (const auto& access : pass.resourceAccesses) {
            const ResourceNode* resource = GetResourceNode(access.resource);
            R_ASSERT4(resource != nullptr && resource->desc.isImported,
                "async framegraph pass touches a resource that is not imported",
                pass.name.c_str(),
                resource ? resource->desc.debugName.c_str() : "<invalid handle>");
        }

        for (const auto& dep : pass.dependsOn) {
            R_ASSERT4(dep.pass->isAsync,
                "async framegraph pass depends on a graphics pass",
                pass.name.c_str(),
                dep.pass->name.c_str());
        }
    }
}

void FrameGraph::CullUnusedPasses() {
    // Mark all passes as potentially culled
    for (auto& pass : m_passes) {
        pass.culled = true;
    }

    // Find terminal passes (passes we MUST execute)
    // These are passes that write to:
    // 1. Imported resources (like backbuffer)
    // 2. Persistent resources (non-transient)
    auto& queue = m_compilePassWorklist;
    queue.clear();

    for (auto& pass : m_passes) {
        if (pass.hasSideEffects) {
            queue.push_back(&pass);
            continue;
        }
        for (const auto& access : pass.resourceAccesses) {
            if (access.IsWrite()) {
                const ResourceNode* resource = GetResourceNode(access.resource);
                if (resource && (resource->desc.isImported || resource->isPersistent)) {
                    queue.push_back(&pass);
                    break;
                }
            }
        }
    }

    // If no terminal passes found, keep all passes (conservative)
    if (queue.empty()) {
        for (auto& pass : m_passes) {
            pass.culled = false;
        }
        return;
    }

    // Mark terminal passes and all their dependencies as used
    // (Work backwards from terminal passes)
    while (!queue.empty()) {
        PassNode* current = queue.back();
        queue.pop_back();

        // Mark as used
        if (current->culled) {
            current->culled = false;

            // Only data-flow edges keep a producer alive
            for (const auto& dependency : current->dependsOn) {
                if (dependency.kind == EdgeKind::ReadAfterWrite && dependency.pass->culled) {
                    queue.push_back(dependency.pass);
                }
            }
        }
    }

    // Count culled passes
    u32 culledCount = 0;
    for (const auto& pass : m_passes) {
        if (pass.culled) {
            culledCount++;
        }
    }

    // Remove culled passes from sorted list
    auto it = std::remove_if(m_sortedPasses.begin(), m_sortedPasses.end(),
        [](const PassNode* pass) { return pass->culled; });
    m_sortedPasses.erase(it, m_sortedPasses.end());

    m_stats.numCulledPasses = culledCount;
}

void FrameGraph::ResolveUsage() {
    for (const PassNode* pass : m_sortedPasses) {
        if (pass->culled) continue;

        for (const auto& access : pass->resourceAccesses) {
            ResourceNode* resource = GetResourceNode(access.resource);
            R_ASSERT(resource != nullptr);

            const bool isBuffer = resource->desc.type == ResourceDesc::Type::Buffer;

            if (resource->desc.isImported) {
                const auto* textureDesc = resource->nvrhiTexture ? &resource->nvrhiTexture->getDesc() : nullptr;
                const auto* bufferDesc = resource->nvrhiBuffer ? &resource->nvrhiBuffer->getDesc() : nullptr;
                bool satisfied = true;
                switch (access.state) {
                    case ResourceState::UnorderedAccess:
                        satisfied = isBuffer
                            ? bufferDesc && bufferDesc->canHaveUAVs
                            : textureDesc && textureDesc->isUAV;
                        break;
                    case ResourceState::RenderTarget:
                        satisfied = !isBuffer && textureDesc && textureDesc->isRenderTarget
                            && !nvrhi::getFormatInfo(textureDesc->format).hasDepth
                            && !nvrhi::getFormatInfo(textureDesc->format).hasStencil;
                        break;
                    case ResourceState::DepthStencilWrite:
                    case ResourceState::DepthStencilRead:
                        satisfied = !isBuffer && textureDesc && textureDesc->isRenderTarget
                            && (nvrhi::getFormatInfo(textureDesc->format).hasDepth
                                || nvrhi::getFormatInfo(textureDesc->format).hasStencil);
                        break;
                    case ResourceState::IndirectArgument:
                        satisfied = isBuffer && bufferDesc && bufferDesc->isDrawIndirectArgs;
                        break;
                    default:
                        break;
                }
                R_ASSERT4(satisfied,
                    "framegraph pass requests a state the imported resource was not created for",
                    pass->name.c_str(),
                    resource->desc.debugName.c_str());
                continue;
            }

            switch (access.state) {
                case ResourceState::UnorderedAccess:
                    if (isBuffer)
                        resource->desc.allowUAV = true;
                    else
                        resource->desc.isUAV = true;
                    break;
                case ResourceState::RenderTarget:
                    resource->desc.isRenderTarget = true;
                    break;
                case ResourceState::DepthStencilWrite:
                case ResourceState::DepthStencilRead:
                case ResourceState::DepthStencilReadShaderResource:
                    resource->desc.isDepthStencil = true;
                    break;
                case ResourceState::IndirectArgument:
                    resource->desc.isIndirectArgs = true;
                    break;
                default:
                    break;
            }
        }
    }
}

void FrameGraph::ComputeResourceLifetimes() {
    // Reset lifetimes for all resources
    for (auto& resource : m_resources) {
        resource.firstUsedPass = INVALID_INDEX;
        resource.lastUsedPass = INVALID_INDEX;
        resource.refCount = 0;
    }

    u32 nonCulledPasses = 0;
    // Iterate through sorted passes (in execution order)
    for (const PassNode* pass : m_sortedPasses) {
        // Skip culled passes
        if (pass->culled) continue;
        nonCulledPasses++;

        u32 passIndex = pass->executionOrder;

        // Check all resources accessed by this pass
        for (const auto& access : pass->resourceAccesses) {
            if (!access.resource.is_valid()) continue;

            ResourceNode* resource = GetResourceNode(access.resource);
            if (!resource) continue;

            // Update first usage
            if (resource->firstUsedPass == INVALID_INDEX) {
                resource->firstUsedPass = passIndex;
            }

            // Update last usage
            resource->lastUsedPass = passIndex;

            // Increment reference count
            resource->refCount++;
        }
    }

    // Count unused resources
    u32 unusedCount = 0;
    for (const auto& resource : m_resources) {
        if (resource.firstUsedPass == INVALID_INDEX) {
            unusedCount++;
        }
    }

    m_stats.numCulledResources = unusedCount;
}

void FrameGraph::BuildLifetimeLists() {
    for (auto& pass : m_passes) {
        pass.devirtualize.clear();
        pass.destroy.clear();
    }

    for (auto& resource : m_resources) {
        if (resource.desc.isImported || resource.firstUsedPass == INVALID_INDEX)
            continue;

        R_ASSERT(resource.firstUsedPass < m_passes.size());
        R_ASSERT(resource.lastUsedPass < m_passes.size());
        PassNode& first = m_passes[resource.firstUsedPass];
        PassNode& last = m_passes[resource.lastUsedPass];
        R_ASSERT(!first.culled && !last.culled);
        R_ASSERT(!first.isAsync && !last.isAsync);
        first.devirtualize.push_back(&resource);
        last.destroy.push_back(&resource);
    }
}

void FrameGraph::Destroy(ResourceNode& resource) {
    R_ASSERT(!resource.desc.isImported);
    if (resource.resourceTexture.IsValid()) {
        m_resourcePool->FreeTexture(resource.resourceTexture);
        resource.resourceTexture = {};
    }
    if (resource.resourceBuffer.IsValid()) {
        m_resourcePool->FreeBuffer(resource.resourceBuffer);
        resource.resourceBuffer = {};
    }
    resource.nvrhiTexture = nullptr;
    resource.nvrhiBuffer = nullptr;
    resource.isAllocated = false;
}

void FrameGraph::Devirtualize(ResourceNode& resource) {
    R_ASSERT(!resource.desc.isImported && !resource.isAllocated);

    if (resource.desc.type == ResourceDesc::Type::Buffer) {
        if (m_resourcePool) {
            resources::BufferDesc rmBufferDesc;
            rmBufferDesc.type = resources::BufferType::Structured;
            rmBufferDesc.usage = resources::BufferUsage::Static;
            rmBufferDesc.size = resource.desc.bufferSize;
            rmBufferDesc.stride = resource.desc.structStride;
            rmBufferDesc.gpuWrite = resource.desc.allowUAV;
            rmBufferDesc.indirectArgs = resource.desc.isIndirectArgs;
            rmBufferDesc.cpuAccess = false;
            rmBufferDesc.debugName = resource.desc.debugName;

            resource.resourceBuffer = m_resourcePool->AllocateBuffer(rmBufferDesc);
            if (resource.resourceBuffer.IsValid())
                resource.nvrhiBuffer = m_resourceManager->GetBufferManager()->GetNVRHIBuffer(resource.resourceBuffer);
        }

        if (!resource.resourceBuffer.IsValid()) {
            nvrhi::BufferDesc nvrhiDesc;
            nvrhiDesc.byteSize = resource.desc.bufferSize;
            nvrhiDesc.structStride = resource.desc.structStride;
            nvrhiDesc.debugName = resource.desc.debugName.c_str();
            nvrhiDesc.initialState = nvrhi::ResourceStates::Common;
            nvrhiDesc.keepInitialState = true;
            nvrhiDesc.canHaveUAVs = resource.desc.allowUAV;
            nvrhiDesc.isDrawIndirectArgs = resource.desc.isIndirectArgs;
            resource.nvrhiBuffer = m_device->createBuffer(nvrhiDesc);
        }
    } else {
        if (m_resourcePool) {
            resources::TextureDesc rmTexDesc;
            rmTexDesc.width = resource.desc.width;
            rmTexDesc.height = resource.desc.height;
            rmTexDesc.depth = resource.desc.depth;
            rmTexDesc.arraySize = resource.desc.arraySize;
            rmTexDesc.mipLevels = resource.desc.mipLevels;
            rmTexDesc.sampleCount = resource.desc.sampleCount;
            rmTexDesc.format = resource.desc.format;
            rmTexDesc.debugName = resource.desc.debugName;

            switch (resource.desc.type) {
                case ResourceDesc::Type::Texture2D:
                    rmTexDesc.type = resources::TextureDesc::Texture2D;
                    break;
                case ResourceDesc::Type::Texture3D:
                    rmTexDesc.type = resources::TextureDesc::Texture3D;
                    break;
                case ResourceDesc::Type::TextureCube:
                    rmTexDesc.type = resources::TextureDesc::TextureCube;
                    break;
                case ResourceDesc::Type::Texture2DArray:
                    rmTexDesc.type = resources::TextureDesc::Texture2DArray;
                    break;
                default:
                    NODEFAULT;
            }

            rmTexDesc.isRenderTarget = resource.desc.isRenderTarget;
            rmTexDesc.isDepthStencil = resource.desc.isDepthStencil;
            rmTexDesc.isUAV = resource.desc.isUAV || resource.desc.allowUAV;
            resource.resourceTexture = m_resourcePool->AllocateTexture(rmTexDesc);
            if (resource.resourceTexture.IsValid())
                resource.nvrhiTexture = m_resourceManager->GetTextureManager()->GetNVRHITexture(resource.resourceTexture);
        }

        if (!resource.resourceTexture.IsValid()) {
            nvrhi::TextureDesc nvrhiDesc;
            nvrhiDesc.width = resource.desc.width;
            nvrhiDesc.height = resource.desc.height;
            nvrhiDesc.depth = resource.desc.depth;
            nvrhiDesc.arraySize = resource.desc.arraySize;
            nvrhiDesc.mipLevels = resource.desc.mipLevels;
            nvrhiDesc.sampleCount = resource.desc.sampleCount;
            nvrhiDesc.format = resource.desc.format;
            nvrhiDesc.debugName = resource.desc.debugName.c_str();
            nvrhiDesc.keepInitialState = true;

            switch (resource.desc.type) {
                case ResourceDesc::Type::Texture2D:
                    nvrhiDesc.dimension = resource.desc.sampleCount > 1
                        ? nvrhi::TextureDimension::Texture2DMS : nvrhi::TextureDimension::Texture2D;
                    break;
                case ResourceDesc::Type::Texture3D:
                    nvrhiDesc.dimension = nvrhi::TextureDimension::Texture3D;
                    break;
                case ResourceDesc::Type::TextureCube:
                    nvrhiDesc.dimension = nvrhi::TextureDimension::TextureCube;
                    break;
                case ResourceDesc::Type::Texture2DArray:
                    nvrhiDesc.dimension = resource.desc.sampleCount > 1
                        ? nvrhi::TextureDimension::Texture2DMSArray : nvrhi::TextureDimension::Texture2DArray;
                    break;
                default:
                    NODEFAULT;
            }

            nvrhiDesc.isRenderTarget = resource.desc.isRenderTarget;
            nvrhiDesc.isUAV = resource.desc.isUAV || resource.desc.allowUAV;
            nvrhiDesc.isShaderResource = true;
            if (resource.desc.isRenderTarget && !resource.desc.isDepthStencil) {
                nvrhiDesc.initialState = nvrhi::ResourceStates::RenderTarget;
                nvrhiDesc.useClearValue = true;
                nvrhiDesc.clearValue = nvrhi::Color(0.0f);
            } else if (resource.desc.isDepthStencil) {
                nvrhiDesc.initialState = nvrhi::ResourceStates::DepthWrite;
                nvrhiDesc.isRenderTarget = true;
                nvrhiDesc.isTypeless = true;
                nvrhiDesc.useClearValue = true;
                nvrhiDesc.clearValue = nvrhi::Color(0.0f);
            } else {
                nvrhiDesc.initialState = nvrhi::ResourceStates::ShaderResource;
            }
            resource.nvrhiTexture = m_device->createTexture(nvrhiDesc);
        }
    }

    resource.isAllocated = resource.nvrhiTexture || resource.nvrhiBuffer;
    R_ASSERT3(resource.isAllocated, "Failed to allocate framegraph resource", resource.desc.debugName.c_str());
    m_stats.totalMemoryAllocated += resource.memorySize;
}

void FrameGraph::InsertResourceBarriers() {
    // Track current state of each resource
    xr_map<u32, ResourceState> currentStates;

    // Initialize all resources to Undefined state
    for (auto& resource : m_resources) {
        currentStates[resource.handle.index] = ResourceState::Undefined;
        resource.currentState = ResourceState::Undefined;
    }

    u32 totalBarriers = 0;

    // Iterate through sorted passes
    for (PassNode* pass : m_sortedPasses) {
        if (pass->culled) continue;

        pass->barriersBeforePass.clear();

        // Check each resource access in this pass
        for (const auto& access : pass->resourceAccesses) {
            if (!access.resource.is_valid()) continue;

            ResourceNode* resource = GetResourceNode(access.resource);
            if (!resource) continue;

            // Get current and required states
            ResourceState currentState = currentStates[access.resource.index];
            ResourceState requiredState = access.state;

            // Insert barrier if state transition needed
            if (currentState != requiredState && currentState != ResourceState::Undefined) {
                pass->barriersBeforePass.push_back(
                    ResourceBarrier(access.resource, currentState, requiredState)
                );
                totalBarriers++;
            }

            // Update current state after this access
            // Write accesses define the new state
            if (access.IsWrite()) {
                currentStates[access.resource.index] = requiredState;
                resource->currentState = requiredState;
            }
        }
    }
}

// ══════════════════════════════════════════════════════════
//  STATE CONVERSION
// ══════════════════════════════════════════════════════════

nvrhi::ResourceStates FrameGraph::ConvertToNVRHIState(ResourceState state) {
    switch (state) {
        case ResourceState::Undefined:
            return nvrhi::ResourceStates::Common;
        case ResourceState::RenderTarget:
            return nvrhi::ResourceStates::RenderTarget;
        case ResourceState::DepthStencilWrite:
            return nvrhi::ResourceStates::DepthWrite;
        case ResourceState::DepthStencilRead:
            return nvrhi::ResourceStates::DepthRead;
        case ResourceState::DepthStencilReadShaderResource:
            return nvrhi::ResourceStates::DepthRead | nvrhi::ResourceStates::ShaderResource;
        case ResourceState::ShaderResource:
            return nvrhi::ResourceStates::ShaderResource;
        case ResourceState::UnorderedAccess:
            return nvrhi::ResourceStates::UnorderedAccess;
        case ResourceState::IndirectArgument:
            return nvrhi::ResourceStates::IndirectArgument;
        case ResourceState::CopySource:
            return nvrhi::ResourceStates::CopySource;
        case ResourceState::CopyDest:
            return nvrhi::ResourceStates::CopyDest;
        case ResourceState::Present:
            return nvrhi::ResourceStates::Present;
        case ResourceState::Common:
            return nvrhi::ResourceStates::Common;
        default:
            return nvrhi::ResourceStates::Common;
    }
}

} // namespace xray::render::framegraph
