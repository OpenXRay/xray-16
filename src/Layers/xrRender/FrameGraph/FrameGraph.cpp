// xrRender/FrameGraph/FrameGraph.cpp
#include "stdafx.h"
#include "FrameGraph.h"

namespace xray::render::framegraph {

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  CONSTRUCTOR / DESTRUCTOR
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

FrameGraph::FrameGraph(nvrhi::IDevice* device)
    : m_device(device)
{
    VERIFY(device != nullptr);

    // Reserve space to avoid reallocations
    m_resources.reserve(256);
    m_passes.reserve(128);
    m_sortedPasses.reserve(128);

    Msg("* [FrameGraph] Initialized");
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

    Msg("~ [FrameGraph] Created texture '%s' (%ux%u, %.2f MB)",
        name,
        desc.width,
        desc.height,
        desc.ComputeMemorySize() / (1024.0f * 1024.0f));

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

    Msg("~ [FrameGraph] Created buffer '%s' (%.2f MB)",
        name,
        desc.bufferSize / (1024.0f * 1024.0f));

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

    // Add to registry
    m_resources.push_back(node);

    Msg("~ [FrameGraph] Imported texture '%s'", name);

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

    // Add to registry
    m_resources.push_back(node);

    Msg("~ [FrameGraph] Imported buffer '%s'", name);

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

    Msg("~ [FrameGraph] Added pass '%s'", name);

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

    Msg("~ [FrameGraph] Compiling graph...");

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

    Msg("~ [FrameGraph] Compilation complete (%u passes, %u resources)",
        m_stats.numPasses, m_stats.numResources);
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  EXECUTE PHASE (STUB FOR NOW)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void FrameGraph::Execute() {
    VERIFY(m_compiled && "Must compile before execute");
    VERIFY(m_context != nullptr && "RenderContext required for execution");

    Msg("~ [FrameGraph] Executing %u passes...", m_sortedPasses.size());

    u32 passesExecuted = 0;

    // Execute passes in sorted order
    for (PassNode* pass : m_sortedPasses) {
        // Skip culled passes
        if (pass->culled) {
            continue;
        }

        // Skip passes without callbacks
        if (!pass->executeCallback) {
            Msg("! [FrameGraph] Pass '%s' has no execute callback - skipping",
                pass->name.c_str());
            continue;
        }

        Msg("~ [FrameGraph] Executing pass '%s' [%u]",
            pass->name.c_str(), pass->executionOrder);

        // Apply resource barriers before this pass
        if (!pass->barriersBeforePass.empty()) {
            Msg("~ [FrameGraph]   Applying %u barriers...",
                pass->barriersBeforePass.size());

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
                Msg("~ [FrameGraph]     Barrier: %s -> %s",
                    ResourceStateToString(barrier.stateBefore),
                    ResourceStateToString(barrier.stateAfter));
            }
        }

        // Execute the pass callback
        pass->executeCallback(*m_context, *this);

        passesExecuted++;
    }

    Msg("~ [FrameGraph] Execution complete: %u/%u passes executed",
        passesExecuted, m_sortedPasses.size());
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  QUERY METHODS
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

nvrhi::ITexture* FrameGraph::GetPhysicalTexture(VirtualResourceHandle handle) const {
    const ResourceNode* node = GetResourceNode(handle);
    VERIFY(node != nullptr);
    VERIFY(node->isAllocated && "Resource not allocated - call Compile first");
    return node->nvrhiTexture;
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

void FrameGraph::Reset() {
    // Destroy allocated resources (except imported)
    for (auto& resource : m_resources) {
        // Skip imported resources (we don't own them)
        if (resource.desc.isImported) {
            continue;
        }

        // Release NVRHI resources
        if (resource.nvrhiTexture) {
            Msg("~ [FrameGraph] Releasing texture '%s'",
                resource.desc.debugName.c_str());
            resource.nvrhiTexture = nullptr;  // RefPtr will release
        }

        if (resource.nvrhiBuffer) {
            Msg("~ [FrameGraph] Releasing buffer '%s'",
                resource.desc.debugName.c_str());
            resource.nvrhiBuffer = nullptr;  // RefPtr will release
        }
    }

    // Clear state
    m_resources.clear();
    m_passes.clear();
    m_sortedPasses.clear();
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

    Msg("~ [FrameGraph] Reset complete");
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
            Msg("! [FrameGraph] ERROR: Pass '%s' has no execute callback", pass.name.c_str());
            valid = false;
        }
    }

    // Check 2: All resources used by non-culled passes are allocated
    for (const auto& pass : m_passes) {
        if (pass.culled) continue;

        for (const auto& access : pass.resourceAccesses) {
            const ResourceNode* resource = GetResourceNode(access.resource);
            if (!resource) {
                Msg("! [FrameGraph] ERROR: Pass '%s' uses invalid resource handle %u",
                    pass.name.c_str(), access.resource.index);
                valid = false;
                continue;
            }

            if (!resource->isAllocated && !resource->desc.isImported) {
                Msg("! [FrameGraph] ERROR: Pass '%s' uses unallocated resource '%s'",
                    pass.name.c_str(), resource->desc.debugName.c_str());
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
                    Msg("! [FrameGraph] ERROR: Imported buffer '%s' has null handle",
                        resource.desc.debugName.c_str());
                    valid = false;
                }
            } else {
                if (!resource.nvrhiTexture) {
                    Msg("! [FrameGraph] ERROR: Imported texture '%s' has null handle",
                        resource.desc.debugName.c_str());
                    valid = false;
                }
            }
        }
    }

    if (valid) {
        Msg("~ [FrameGraph] Validation passed");
    } else {
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

    Msg("~ [FrameGraph] Built dependency graph: %u passes, %u edges",
        m_passes.size(),
        [this]() {
            u32 edgeCount = 0;
            for (const auto& pass : m_passes) {
                edgeCount += static_cast<u32>(pass.dependsOn.size());
            }
            return edgeCount;
        }());
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

    Msg("~ [FrameGraph] Topological sort complete: %u passes ordered",
        m_sortedPasses.size());
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
        Msg("~ [FrameGraph] No terminal passes found - keeping all passes");
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
            Msg("~ [FrameGraph] Culling unused pass: %s", pass.name.c_str());
        }
    }

    // Remove culled passes from sorted list
    auto it = std::remove_if(m_sortedPasses.begin(), m_sortedPasses.end(),
        [](const PassNode* pass) { return pass->culled; });
    m_sortedPasses.erase(it, m_sortedPasses.end());

    m_stats.numCulledPasses = culledCount;

    Msg("~ [FrameGraph] Pass culling complete: %u/%u passes culled",
        culledCount, m_passes.size());
}

void FrameGraph::ComputeResourceLifetimes() {
    // Reset lifetimes for all resources
    for (auto& resource : m_resources) {
        resource.firstUsedPass = INVALID_INDEX;
        resource.lastUsedPass = INVALID_INDEX;
        resource.refCount = 0;
    }

    // Iterate through sorted passes (in execution order)
    for (const PassNode* pass : m_sortedPasses) {
        // Skip culled passes
        if (pass->culled) continue;

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

    // Log resource lifetimes
    u32 unusedCount = 0;
    for (const auto& resource : m_resources) {
        if (resource.firstUsedPass == INVALID_INDEX) {
            unusedCount++;
            if (resource.desc.debugName.size() > 0) {
                Msg("~ [FrameGraph] Unused resource: %s", resource.desc.debugName.c_str());
            }
        } else {
            u32 lifetime = resource.GetLifetimeSpan();
            const char* name = (resource.desc.debugName.size() > 0) ? resource.desc.debugName.c_str() : "<unnamed>";
            Msg("~ [FrameGraph] Resource '%s': passes [%u-%u], lifetime=%u, refs=%u",
                name,
                resource.firstUsedPass,
                resource.lastUsedPass,
                lifetime,
                resource.refCount);
        }
    }

    m_stats.numCulledResources = unusedCount;

    Msg("~ [FrameGraph] Resource lifetime computation complete: %u/%u resources unused",
        unusedCount, m_resources.size());
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

        // Create physical resource based on type
        if (resource.desc.type == ResourceDesc::Type::Buffer) {
            // Create buffer
            nvrhi::BufferDesc bufferDesc;
            bufferDesc.byteSize = resource.desc.bufferSize;
            bufferDesc.structStride = resource.desc.structStride;
            bufferDesc.debugName = resource.desc.debugName.c_str();
            bufferDesc.initialState = nvrhi::ResourceStates::Common;
            bufferDesc.keepInitialState = false;

            // Set buffer usage flags
            if (resource.desc.allowUAV) {
                bufferDesc.canHaveUAVs = true;
            }
            if (resource.desc.structStride > 0) {
                bufferDesc.isConstantBuffer = false;
                bufferDesc.structStride = resource.desc.structStride;
            }

            resource.nvrhiBuffer = m_device->createBuffer(bufferDesc);

            if (resource.nvrhiBuffer) {
                resource.isAllocated = true;
                totalMemoryAllocated += resource.memorySize;

                Msg("~ [FrameGraph] Allocated buffer '%s': %.2f MB",
                    resource.desc.debugName.c_str(),
                    resource.memorySize / (1024.0f * 1024.0f));
            } else {
                Msg("! [FrameGraph] Failed to allocate buffer '%s'",
                    resource.desc.debugName.c_str());
            }
        } else {
            // Create texture
            nvrhi::TextureDesc texDesc;
            texDesc.width = resource.desc.width;
            texDesc.height = resource.desc.height;
            texDesc.depth = resource.desc.depth;
            texDesc.arraySize = resource.desc.arraySize;
            texDesc.mipLevels = resource.desc.mipLevels;
            texDesc.sampleCount = resource.desc.sampleCount;
            texDesc.format = resource.desc.format;
            texDesc.debugName = resource.desc.debugName.c_str();
            texDesc.initialState = nvrhi::ResourceStates::Common;
            texDesc.keepInitialState = false;

            // Set texture type
            switch (resource.desc.type) {
                case ResourceDesc::Type::Texture2D:
                    texDesc.dimension = nvrhi::TextureDimension::Texture2D;
                    break;
                case ResourceDesc::Type::Texture3D:
                    texDesc.dimension = nvrhi::TextureDimension::Texture3D;
                    break;
                case ResourceDesc::Type::TextureCube:
                    texDesc.dimension = nvrhi::TextureDimension::TextureCube;
                    break;
                case ResourceDesc::Type::Texture2DArray:
                    texDesc.dimension = nvrhi::TextureDimension::Texture2DArray;
                    break;
                default:
                    texDesc.dimension = nvrhi::TextureDimension::Texture2D;
                    break;
            }

            // Set usage flags
            texDesc.isRenderTarget = resource.desc.isRenderTarget;
            texDesc.isUAV = resource.desc.isUAV || resource.desc.allowUAV;
            texDesc.isShaderResource = true;  // Always allow SRV

            // Depth/stencil handling
            if (resource.desc.isDepthStencil) {
                texDesc.isRenderTarget = true;  // NVRHI uses this flag + format to set BIND_DEPTH_STENCIL
                texDesc.isTypeless = true;  // CRITICAL: Use typeless format for depth+SRV
                texDesc.useClearValue = true;
                texDesc.clearValue = nvrhi::Color(1.0f);  // Default depth clear
            }

            resource.nvrhiTexture = m_device->createTexture(texDesc);

            if (resource.nvrhiTexture) {
                resource.isAllocated = true;
                totalMemoryAllocated += resource.memorySize;

                Msg("~ [FrameGraph] Allocated texture '%s': %ux%ux%u, %.2f MB",
                    resource.desc.debugName.c_str(),
                    resource.desc.width,
                    resource.desc.height,
                    resource.desc.depth,
                    resource.memorySize / (1024.0f * 1024.0f));
            } else {
                Msg("! [FrameGraph] Failed to allocate texture '%s'",
                    resource.desc.debugName.c_str());
            }
        }
    }

    m_stats.totalMemoryAllocated = totalMemoryAllocated;

    Msg("~ [FrameGraph] Resource allocation complete: %.2f MB total",
        totalMemoryAllocated / (1024.0f * 1024.0f));
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

                Msg("~ [FrameGraph] Pass '%s': Barrier %s -> %s for resource '%s'",
                    pass->name.c_str(),
                    ResourceStateToString(currentState),
                    ResourceStateToString(requiredState),
                    resource->desc.debugName.c_str());
            }

            // Update current state after this access
            // Write accesses define the new state
            if (access.IsWrite()) {
                currentStates[access.resource.index] = requiredState;
                resource->currentState = requiredState;
            }
        }
    }

    Msg("~ [FrameGraph] Resource barrier insertion complete: %u barriers inserted",
        totalBarriers);
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
        Msg("~ [FrameGraph] No transient resources to alias");
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

                    Msg("~ [FrameGraph] Aliased '%s' with '%s' (saved %.2f MB)",
                        current->desc.debugName.c_str(),
                        candidate->desc.debugName.c_str(),
                        current->memorySize / (1024.0f * 1024.0f));

                    break;
                }
            }
        }
    }

    m_stats.numAliasedResources = aliasedCount;
    m_stats.memoryReduced = memoryReduced;
    m_stats.peakMemoryUsage = m_stats.totalMemoryAllocated - memoryReduced;

    Msg("~ [FrameGraph] Memory aliasing complete: %u resources aliased, %.2f MB saved",
        aliasedCount,
        memoryReduced / (1024.0f * 1024.0f));
    Msg("~ [FrameGraph] Peak memory usage: %.2f MB (reduced from %.2f MB)",
        m_stats.peakMemoryUsage / (1024.0f * 1024.0f),
        m_stats.totalMemoryAllocated / (1024.0f * 1024.0f));
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
