// xrRender/FrameGraph/FrameGraph.cpp
#include "stdafx.h"
#include "FrameGraph.h"
#include "../RenderContext/RenderDevice.h"

namespace xray::render::framegraph {

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  CONSTRUCTOR / DESTRUCTOR
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

FrameGraph::FrameGraph(ng::RenderDevice* renderDevice)
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
    VERIFY(!m_compiled && "Cannot create resources after compile");
    VERIFY(desc.type != ResourceDesc::Type::Buffer && "Use CreateBuffer for buffers");

    // Create resource node
    ResourceNode node(desc);
    node.handle.index = static_cast<u32>(m_resources.size());

    // Add to registry
    m_resources.push_back(node);

    return node.handle;
}

VirtualResourceHandle FrameGraph::CreateBuffer(const char* name, const ResourceDesc& desc) {
    VERIFY(!m_compiled && "Cannot create resources after compile");
    VERIFY(desc.type == ResourceDesc::Type::Buffer && "Use CreateTexture for textures");

    // Create resource node
    ResourceNode node(desc);
    node.handle.index = static_cast<u32>(m_resources.size());

    // Add to registry
    m_resources.push_back(node);

    return node.handle;
}

VirtualResourceHandle FrameGraph::ImportTexture(
    const char* name,
    nvrhi::ITexture* physicalTexture,
    const ResourceDesc& desc
) {
    VERIFY(!m_compiled && "Cannot import resources after compile");
    VERIFY(physicalTexture != nullptr);

    // Create imported resource node
    ResourceNode node(desc);
    node.handle.index = static_cast<u32>(m_resources.size());
    // Store the imported NVRHI texture
    node.nvrhiTexture = physicalTexture;
    node.isAllocated = true;
    node.canAlias = false;
    node.isPersistent = true;
    node.desc.isImported = true;  // CRITICAL: Mark as imported so we don't reallocate/clear it

    // Add to registry
    m_resources.push_back(node);

    return node.handle;
}

VirtualResourceHandle FrameGraph::ImportBuffer(
    const char* name,
    nvrhi::IBuffer* physicalBuffer,
    const ResourceDesc& desc
) {
    VERIFY(!m_compiled && "Cannot import resources after compile");
    VERIFY(physicalBuffer != nullptr);

    // Create imported resource node
    ResourceNode node(desc);
    node.handle.index = static_cast<u32>(m_resources.size());
    // Store the imported NVRHI buffer
    node.nvrhiBuffer = physicalBuffer;
    node.isAllocated = true;
    node.canAlias = false;
    node.isPersistent = true;
    node.desc.isImported = true;  // CRITICAL: Mark as imported so we don't reallocate/clear it

    // Add to registry
    m_resources.push_back(node);

    return node.handle;
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  SETUP PHASE - PASS CREATION
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

PassHandle FrameGraph::AddPass(const char* name) {
    VERIFY(!m_compiled && "Cannot add passes after compile");

    // Create pass node
    PassNode pass(name);
    pass.handle.index = static_cast<u32>(m_passes.size());

    // Add to registry
    m_passes.push_back(pass);

    return pass.handle;
}

void FrameGraph::PassRead(PassHandle pass, VirtualResourceHandle resource, ResourceState state) {
    PassNode* passNode = GetPassNode(pass);
    VERIFY(passNode != nullptr);

    passNode->Read(resource, state);
}

void FrameGraph::PassWrite(PassHandle pass, VirtualResourceHandle resource, ResourceState state) {
    PassNode* passNode = GetPassNode(pass);
    VERIFY(passNode != nullptr);

    passNode->Write(resource, state);
}

void FrameGraph::PassReadWrite(PassHandle pass, VirtualResourceHandle resource, ResourceState state) {
    PassNode* passNode = GetPassNode(pass);
    VERIFY(passNode != nullptr);

    passNode->ReadWrite(resource, state);
}

void FrameGraph::SetPassCallback(PassHandle pass, PassExecuteCallback callback) {
    PassNode* passNode = GetPassNode(pass);
    VERIFY(passNode != nullptr);

    passNode->executeCallback = callback;
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  COMPILE PHASE (STUB FOR NOW)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void FrameGraph::Compile() {
    VERIFY(!m_compiled && "Already compiled");

    // Phase 1: Build dependency graph from resource accesses
    BuildDependencyGraph();

    // Phase 2: Sort passes into execution order
    TopologicalSort();

    // Phase 3: Remove unused passes
    CullUnusedPasses();

    // Phase 4: Compute resource lifetimes for aliasing
    ComputeResourceLifetimes();

    // Phase 5: Allocate physical GPU resources
    AllocateResources();

    // Phase 6: Insert resource barriers for state transitions
    InsertResourceBarriers();

    // Phase 7: Optimize memory usage through aliasing
    OptimizeMemoryAliasing();

    m_compiled = true;
    m_stats.numPasses = static_cast<u32>(m_passes.size());
    m_stats.numResources = static_cast<u32>(m_resources.size());
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  EXECUTE PHASE (STUB FOR NOW)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void FrameGraph::Execute() {
    VERIFY(m_compiled && "Must compile before execute");
    VERIFY(m_context != nullptr && "RenderContext required for execution");
    VERIFY(m_renderDevice != nullptr && "RenderDevice required for execution");

    // Get command list from context (which now uses the backend's command list)
    // The backend opens the command list in BeginFrame() and closes/executes it in EndFrame()
    // We just record commands to it here.
    nvrhi::ICommandList* cmdList = m_context->GetCommandList();
    VERIFY(cmdList != nullptr);

    u32 passesExecuted = 0;
    u32 passesCulled = 0;
    u32 passesNoCallback = 0;

    // Execute passes in sorted order
    for (PassNode* pass : m_sortedPasses) {
        // Skip culled passes
        if (pass->culled) {
            passesCulled++;
            continue;
        }

        // Skip passes without callbacks
        if (!pass->executeCallback) {
            Msg("! [FrameGraph::Execute] Pass '%s' has no callback!", pass->name.c_str());
            passesNoCallback++;
            continue;
        }

        // Apply resource barriers before this pass
        if (!pass->barriersBeforePass.empty()) {
            for (const auto& barrier : pass->barriersBeforePass) {
                ResourceNode* resource = GetResourceNode(barrier.resource);
                if (!resource || !resource->nvrhiTexture) {
                    continue;
                }

                // Convert FrameGraph states to NVRHI states
                nvrhi::ResourceStates nvrhiBefore = ConvertToNVRHIState(barrier.stateBefore);
                nvrhi::ResourceStates nvrhiAfter = ConvertToNVRHIState(barrier.stateAfter);

                // Note: Actual barrier insertion would require command list access
                // For now, we just log the barrier
                // TODO: Insert actual NVRHI barriers when RenderContext supports it
            }
        }

        // Execute the pass callback with automatic markers
        cmdList->beginMarker(pass->name.c_str());
        pass->executeCallback(*m_context, *this);
        cmdList->endMarker();

        passesExecuted++;
    }

    // ═══════════════════════════════════════════════════════
    //  TRANSITION IMPORTED BACKBUFFER TO PRESENT STATE
    // ═══════════════════════════════════════════════════════
    // D3D12 requires backbuffer to be in Present state before Present() is called.
    // Find imported resources and transition them to Present state.
    for (auto& resource : m_resources) {
        if (resource.desc.isImported && resource.nvrhiTexture) {
            // Imported render target (backbuffer) - transition to Present
            if (resource.desc.isRenderTarget) {
                cmdList->setTextureState(
                    resource.nvrhiTexture.Get(),
                    nvrhi::AllSubresources,
                    nvrhi::ResourceStates::Present
                );
            }
        }
    }
    // Commit all pending barriers
    cmdList->commitBarriers();

    // NOTE: We do NOT close or execute the command list here.
    // The backend handles that in EndFrame() after all rendering is complete.
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  QUERY METHODS
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

nvrhi::ITexture* FrameGraph::GetPhysicalTexture(VirtualResourceHandle handle) const {
    const ResourceNode* node = GetResourceNode(handle);
    if (!node || !node->isAllocated) {
        return nullptr;
    }

    nvrhi::ITexture* tex = node->nvrhiTexture.Get();

    if (!tex) {
        return nullptr;
    }

    return tex;
}

nvrhi::IBuffer* FrameGraph::GetPhysicalBuffer(VirtualResourceHandle handle) const {
    const ResourceNode* node = GetResourceNode(handle);
    VERIFY(node != nullptr);
    VERIFY(node->isAllocated && "Resource not allocated - call Compile first");
    return node->nvrhiBuffer;
}

const ResourceDesc& FrameGraph::GetResourceDesc(VirtualResourceHandle handle) const {
    const ResourceNode* node = GetResourceNode(handle);
    VERIFY(node != nullptr);
    return node->desc;
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  RESET
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

// ════════════════════════════════════════════════════════════
//  RESET FOR NEXT FRAME (KEEPS STRUCTURE)
// ════════════════════════════════════════════════════════════

void FrameGraph::ResetForNextFrame() {
    // Lambda-based FrameGraph architecture (Frostbite-style):
    // - Graph structure rebuilt every frame
    // - GPU resources reused from pool
    //

    // FREE TRANSIENT RESOURCES FIRST (before clearing the vector!)
    // This returns them to the pool for reuse next frame
    if (m_resourcePool) {
        for (auto& resource : m_resources) {
            // Skip imported resources (we don't own them)
            if (resource.desc.isImported) {
                continue;
            }

            // Free transient textures back to pool for aliasing
            if (resource.resourceTexture.IsValid()) {
                m_resourcePool->FreeTexture(resource.resourceTexture);
            }

            // Free buffers
            if (resource.resourceBuffer.IsValid()) {
                m_resourcePool->FreeBuffer(resource.resourceBuffer);
            }
        }
    }

    // Clear graph structure:
    m_passes.clear();
    m_resources.clear();
    m_sortedPasses.clear();
    m_compiled = false;

    // Clear render target registry
    m_rtRegistry.Clear();

    // Reset per-frame statistics
    m_stats = Statistics();
}

// ════════════════════════════════════════════════════════════
//  FULL RESET (CLEARS EVERYTHING INCLUDING STRUCTURE)
// ════════════════════════════════════════════════════════════

void FrameGraph::Reset() {
    // Release ResourceManager handles first (before resetting pool)
    if (m_resourceManager) {
        resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
        resources::BufferManager* bufManager = m_resourceManager->GetBufferManager();

        for (auto& resource : m_resources) {
            // Skip imported resources (we don't own them)
            if (resource.desc.isImported) {
                continue;
            }

            // Release ResourceManager textures
            if (resource.resourceTexture.IsValid()) {
                texManager->Release(resource.resourceTexture);
                resource.resourceTexture = resources::TextureHandle();
            }

            // Release ResourceManager buffers
            if (resource.resourceBuffer.IsValid()) {
                bufManager->Release(resource.resourceBuffer);
                resource.resourceBuffer = resources::BufferHandle();
            }
        }
    }

    // Reset resource pool (frees transient resources)
    if (m_resourcePool) {
        m_resourcePool->Reset();
    }

    // Clear direct NVRHI handles (already released via ResourceManager)
    for (auto& resource : m_resources) {
        // Skip imported resources (we don't own them)
        if (resource.desc.isImported) {
            continue;
        }

        // Clear NVRHI handles (RefPtr will release if not managed by ResourceManager)
        if (resource.nvrhiTexture) {
            resource.nvrhiTexture = nullptr;
        }

        if (resource.nvrhiBuffer) {
            resource.nvrhiBuffer = nullptr;
        }
    }

    // Clear state
    m_resources.clear();
    m_passes.clear();
    m_sortedPasses.clear();
    m_rtRegistry.Clear();  // Clear RT registry
    m_compiled = false;

    // Reset statistics (don't use memset - contains non-trivial types!)
    m_stats.numPasses = 0;
    m_stats.numResources = 0;
    m_stats.numCulledPasses = 0;
    m_stats.numCulledResources = 0;
    m_stats.compileTimeMs = 0.0f;
    m_stats.totalMemoryAllocated = 0;
    m_stats.peakMemoryUsage = 0;
    m_stats.numAliasedResources = 0;
    m_stats.memoryReduced = 0;
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
        Msg("  Peak usage: %.2f MB", m_stats.peakMemoryUsage / (1024.0f * 1024.0f));

        if (m_stats.numAliasedResources > 0) {
            float savingsPercent = 100.0f * m_stats.memoryReduced / (float)m_stats.totalMemoryAllocated;
            Msg("  Saved via aliasing: %.2f MB (%.1f%%)",
                m_stats.memoryReduced / (1024.0f * 1024.0f),
                savingsPercent);
            Msg("  Aliased resources: %u", m_stats.numAliasedResources);
        }
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

        Msg("[%2u] %-30s (depth %u, %u deps)",
            i,
            pass->name.c_str(),
            pass->depth,
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

    // Check 3: No cyclic dependencies (should be caught by topological sort)
    // Already handled by TopologicalSort

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
    if (!handle.is_valid() || handle.index >= m_resources.size()) {
        return nullptr;
    }
    return &m_resources[handle.index];
}

const ResourceNode* FrameGraph::GetResourceNode(VirtualResourceHandle handle) const {
    if (!handle.is_valid() || handle.index >= m_resources.size()) {
        return nullptr;
    }
    return &m_resources[handle.index];
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

PassNode* FrameGraph::FindProducer(VirtualResourceHandle resource) {
    // TODO: Implement producer finding
    return nullptr;
}

bool FrameGraph::HasCyclicDependency() const {
    // TODO: Implement cycle detection
    return false;
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  COMPILATION PHASES (STUBS - TO BE IMPLEMENTED)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void FrameGraph::BuildDependencyGraph() {
    // Clear existing dependencies
    for (auto& pass : m_passes) {
        pass.dependsOn.clear();
        pass.dependents.clear();
    }

    // Track the last pass that wrote to each resource
    xr_map<u32, PassNode*> lastWriter;  // resource index -> pass that wrote it

    // For each pass in order
    for (auto& pass : m_passes) {
        // Check all resources this pass reads
        xr_vector<VirtualResourceHandle> readResources;
        pass.GetReadResources(readResources);

        for (const auto& resource : readResources) {
            // Find the producer (last pass that wrote this resource)
            auto it = lastWriter.find(resource.index);
            if (it != lastWriter.end()) {
                PassNode* producer = it->second;

                // Add dependency: this pass depends on the producer
                if (!pass.DependsOn(producer)) {
                    pass.dependsOn.push_back(producer);
                    producer->dependents.push_back(&pass);
                }
            }
        }

        // Track resources this pass writes (for future consumers)
        xr_vector<VirtualResourceHandle> writeResources;
        pass.GetWriteResources(writeResources);

        for (const auto& resource : writeResources) {
            lastWriter[resource.index] = &pass;
        }
    }

}

void FrameGraph::TopologicalSort() {
    m_sortedPasses.clear();
    m_sortedPasses.reserve(m_passes.size());

    // Kahn's algorithm for topological sorting
    // Calculate in-degree (number of dependencies) for each pass
    xr_map<PassNode*, u32> inDegree;
    for (auto& pass : m_passes) {
        inDegree[&pass] = static_cast<u32>(pass.dependsOn.size());
    }

    // Queue of passes with no dependencies (in-degree = 0)
    xr_vector<PassNode*> queue;
    for (auto& pass : m_passes) {
        if (inDegree[&pass] == 0) {
            queue.push_back(&pass);
        }
    }

    // Process passes in dependency order
    u32 executionOrder = 0;
    while (!queue.empty()) {
        // Pop pass from queue
        PassNode* current = queue.back();
        queue.pop_back();

        // Add to sorted list
        current->executionOrder = executionOrder++;
        m_sortedPasses.push_back(current);

        // Decrease in-degree for all dependents
        for (PassNode* dependent : current->dependents) {
            inDegree[dependent]--;

            // If dependent now has no dependencies, add to queue
            if (inDegree[dependent] == 0) {
                queue.push_back(dependent);
            }
        }
    }

    // Check for cycles (if we didn't process all passes)
    if (m_sortedPasses.size() != m_passes.size()) {
        Msg("! [FrameGraph] Cycle detected in dependency graph!");
        Msg("! [FrameGraph] Processed %u/%u passes",
            m_sortedPasses.size(), m_passes.size());

        // Find passes that weren't processed (part of cycle)
        for (auto& pass : m_passes) {
            if (pass.executionOrder == INVALID_INDEX) {
                Msg("! [FrameGraph]   Pass in cycle: %s", pass.name.c_str());
            }
        }

        VERIFY2(false, "FrameGraph has cyclic dependencies");
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
    xr_vector<PassNode*> terminalPasses;

    for (auto& pass : m_passes) {
        // Check if this pass writes to any imported/persistent resources
        for (const auto& access : pass.resourceAccesses) {
            if (access.IsWrite()) {
                const ResourceNode* resource = GetResourceNode(access.resource);
                if (resource && (resource->desc.isImported || resource->isPersistent)) {
                    terminalPasses.push_back(&pass);
                    break;
                }
            }
        }
    }

    // If no terminal passes found, keep all passes (conservative)
    if (terminalPasses.empty()) {
        for (auto& pass : m_passes) {
            pass.culled = false;
        }
        return;
    }

    // Mark terminal passes and all their dependencies as used
    // (Work backwards from terminal passes)
    xr_vector<PassNode*> queue = terminalPasses;

    while (!queue.empty()) {
        PassNode* current = queue.back();
        queue.pop_back();

        // Mark as used
        if (current->culled) {
            current->culled = false;

            // Add all dependencies to queue
            for (PassNode* dependency : current->dependsOn) {
                if (dependency->culled) {
                    queue.push_back(dependency);
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

void FrameGraph::AllocateResources() {
    u64 totalMemoryAllocated = 0;

    for (auto& resource : m_resources) {
        // Skip unused resources
        if (resource.firstUsedPass == INVALID_INDEX) {
            continue;
        }

        // Skip imported resources (already have physical resources)
        if (resource.desc.isImported) {
            resource.isAllocated = true;
            continue;
        }

        // Determine if resource should use aliasing
        // ONLY imported resources (owned externally) should skip aliasing
        // All transient resources (created each frame) use the aliasing pool
        bool skipAliasing = resource.desc.isImported;  // Only imported resources skip the pool

        // Create physical resource based on type
        if (resource.desc.type == ResourceDesc::Type::Buffer) {
            // Allocate buffer via ResourceManager if available
            if (m_resourcePool) {
                // Convert to ResourceManager BufferDesc
                resources::BufferDesc rmBufferDesc;
                rmBufferDesc.type = resources::BufferType::Structured;
                rmBufferDesc.usage = resources::BufferUsage::Static;
                rmBufferDesc.size = resource.desc.bufferSize;
                rmBufferDesc.stride = resource.desc.structStride;
                rmBufferDesc.gpuWrite = resource.desc.allowUAV;
                rmBufferDesc.cpuAccess = false;
                rmBufferDesc.debugName = resource.desc.debugName;

                // Allocate via FGResourcePool
                resources::BufferHandle rmHandle = m_resourcePool->AllocateBuffer(rmBufferDesc);

                if (rmHandle.IsValid()) {
                    // Store ResourceManager handle for lifecycle management
                    resource.resourceBuffer = rmHandle;

                    // Get the NVRHI buffer for immediate use
                    resources::BufferManager* bufManager = m_resourceManager->GetBufferManager();
                    resource.nvrhiBuffer = bufManager->GetNVRHIBuffer(rmHandle);
                    resource.isAllocated = (resource.nvrhiBuffer != nullptr);
                    totalMemoryAllocated += resource.memorySize;

                    // Early continue - we're done
                    continue;
                } else {
                    Msg("! [FrameGraph] Failed to allocate buffer '%s' via ResourceManager, falling back to NVRHI",
                        resource.desc.debugName.c_str());
                }
            }

            // Fallback: Create NVRHI buffer directly
            nvrhi::BufferDesc nvrhiDesc;
            nvrhiDesc.byteSize = resource.desc.bufferSize;
            nvrhiDesc.structStride = resource.desc.structStride;
            nvrhiDesc.debugName = resource.desc.debugName.c_str();
            nvrhiDesc.initialState = nvrhi::ResourceStates::Common;
            nvrhiDesc.keepInitialState = true;  // D3D12 requires state tracking
            nvrhiDesc.canHaveUAVs = resource.desc.allowUAV;

            resource.nvrhiBuffer = m_device->createBuffer(nvrhiDesc);

            if (resource.nvrhiBuffer) {
                resource.isAllocated = true;
                totalMemoryAllocated += resource.memorySize;
            } else {
                Msg("! [FrameGraph] Failed to create NVRHI buffer '%s'",
                    resource.desc.debugName.c_str());
            }
        } else {
            // Allocate texture via ResourceManager if available
            if (m_resourcePool) {
                // Convert to ResourceManager TextureDesc
                resources::TextureDesc rmTexDesc;
                rmTexDesc.width = resource.desc.width;
                rmTexDesc.height = resource.desc.height;
                rmTexDesc.depth = resource.desc.depth;
                rmTexDesc.arraySize = resource.desc.arraySize;
                rmTexDesc.mipLevels = resource.desc.mipLevels;
                rmTexDesc.format = resource.desc.format;
                rmTexDesc.debugName = resource.desc.debugName;

                // Set texture type
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
                        rmTexDesc.type = resources::TextureDesc::Texture2D;
                        break;
                }

                // Set usage flags
                rmTexDesc.isRenderTarget = resource.desc.isRenderTarget;
                rmTexDesc.isDepthStencil = resource.desc.isDepthStencil;
                rmTexDesc.isUAV = resource.desc.isUAV || resource.desc.allowUAV;

                // Allocate via FGResourcePool (with or without aliasing)
                resources::TextureHandle rmHandle;
                if (skipAliasing) {
                    rmHandle = m_resourcePool->AllocatePersistentTexture(rmTexDesc);
                } else {
                    rmHandle = m_resourcePool->AllocateTexture(rmTexDesc);
                }

                if (rmHandle.IsValid()) {
                    // Store ResourceManager handle for lifecycle management
                    resource.resourceTexture = rmHandle;

                    // Get the NVRHI texture for immediate use
                    resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
                    resource.nvrhiTexture = texManager->GetNVRHITexture(rmHandle);
                    resource.isAllocated = (resource.nvrhiTexture != nullptr);
                    totalMemoryAllocated += resource.memorySize;
                    continue;
                }
            }

            // Fallback: Create NVRHI texture directly (if no ResourceManager or allocation failed)
            nvrhi::TextureDesc nvrhiDesc;
                nvrhiDesc.width = resource.desc.width;
                nvrhiDesc.height = resource.desc.height;
                nvrhiDesc.depth = resource.desc.depth;
                nvrhiDesc.arraySize = resource.desc.arraySize;
                nvrhiDesc.mipLevels = resource.desc.mipLevels;
                nvrhiDesc.sampleCount = resource.desc.sampleCount;
                nvrhiDesc.format = resource.desc.format;
                nvrhiDesc.debugName = resource.desc.debugName.c_str();
                nvrhiDesc.initialState = nvrhi::ResourceStates::Common;
                nvrhiDesc.keepInitialState = true;  // D3D12 requires state tracking

                // Set texture dimension
                switch (resource.desc.type) {
                    case ResourceDesc::Type::Texture2D:
                        nvrhiDesc.dimension = nvrhi::TextureDimension::Texture2D;
                        break;
                    case ResourceDesc::Type::Texture3D:
                        nvrhiDesc.dimension = nvrhi::TextureDimension::Texture3D;
                        break;
                    case ResourceDesc::Type::TextureCube:
                        nvrhiDesc.dimension = nvrhi::TextureDimension::TextureCube;
                        break;
                    case ResourceDesc::Type::Texture2DArray:
                        nvrhiDesc.dimension = nvrhi::TextureDimension::Texture2DArray;
                        break;
                    default:
                        nvrhiDesc.dimension = nvrhi::TextureDimension::Texture2D;
                        break;
                }

                // Set usage flags
                nvrhiDesc.isRenderTarget = resource.desc.isRenderTarget;
                nvrhiDesc.isUAV = resource.desc.isUAV || resource.desc.allowUAV;
                nvrhiDesc.isShaderResource = true;  // Always allow SRV

                // Set optimized clear value for render targets (D3D12 performance optimization)
                if (resource.desc.isRenderTarget && !resource.desc.isDepthStencil) {
                    nvrhiDesc.useClearValue = true;
                    nvrhiDesc.clearValue = nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f);  // Default RT clear: black
                }

                // Depth/stencil handling
                if (resource.desc.isDepthStencil) {
                    nvrhiDesc.isRenderTarget = true;  // NVRHI uses this flag + format to set BIND_DEPTH_STENCIL
                    nvrhiDesc.isTypeless = true;  // CRITICAL: Use typeless format for depth+SRV
                    nvrhiDesc.useClearValue = true;
                    nvrhiDesc.clearValue = nvrhi::Color(1.0f);  // Default depth clear
                }

            resource.nvrhiTexture = m_device->createTexture(nvrhiDesc);

            if (resource.nvrhiTexture) {
                resource.isAllocated = true;
                totalMemoryAllocated += resource.memorySize;
            }
        }
    }

    m_stats.totalMemoryAllocated = totalMemoryAllocated;
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

void FrameGraph::OptimizeMemoryAliasing() {
    // Collect all transient resources (candidates for aliasing)
    xr_vector<ResourceNode*> transientResources;
    for (auto& resource : m_resources) {
        if (resource.canAlias && resource.firstUsedPass != INVALID_INDEX && !resource.desc.isImported) {
            transientResources.push_back(&resource);
        }
    }

    if (transientResources.empty()) {
        return;
    }

    // Sort by memory size (largest first) for better packing
    std::sort(transientResources.begin(), transientResources.end(),
        [](const ResourceNode* a, const ResourceNode* b) {
            return a->memorySize > b->memorySize;
        });

    u32 aliasedCount = 0;
    u64 memoryReduced = 0;

    // Try to alias each resource with a previous one
    for (size_t i = 0; i < transientResources.size(); i++) {
        ResourceNode* current = transientResources[i];

        // Skip if already aliased
        if (current->aliasedWith != INVALID_INDEX) {
            continue;
        }

        // Look for a resource to alias with
        for (size_t j = 0; j < i; j++) {
            ResourceNode* candidate = transientResources[j];

            // Check if lifetimes don't overlap
            if (!current->OverlapsWith(*candidate)) {
                // Check if they have compatible properties
                bool compatible = true;

                // Must be same type (texture vs buffer)
                if (current->desc.type != candidate->desc.type) {
                    compatible = false;
                }

                // Must have same format for textures
                if (current->desc.type != ResourceDesc::Type::Buffer &&
                    current->desc.format != candidate->desc.format) {
                    compatible = false;
                }

                // Candidate must be large enough
                if (candidate->memorySize < current->memorySize) {
                    compatible = false;
                }

                if (compatible) {
                    // Alias this resource with the candidate
                    current->aliasedWith = candidate->handle.index;
                    aliasedCount++;
                    memoryReduced += current->memorySize;
                    break;
                }
            }
        }
    }

    m_stats.numAliasedResources = aliasedCount;
    m_stats.memoryReduced = memoryReduced;
    m_stats.peakMemoryUsage = m_stats.totalMemoryAllocated - memoryReduced;
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
