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
}

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
    // Note: We'll need to store the NVRHI texture pointer differently
    // For now, mark as imported
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
    // Note: We'll need to store the NVRHI buffer pointer differently
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

    // Phase 5-7: TODO Week 10
    // AllocateResources();
    // InsertResourceBarriers();
    // OptimizeMemoryAliasing();

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

    Msg("~ [FrameGraph] Executing graph...");

    // TODO: Execute passes in sorted order

    Msg("~ [FrameGraph] Execution complete");
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  QUERY METHODS
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

nvrhi::ITexture* FrameGraph::GetPhysicalTexture(VirtualResourceHandle handle) const {
    const ResourceNode* node = GetResourceNode(handle);
    VERIFY(node != nullptr);
    VERIFY(node->isAllocated && "Resource not allocated - call Compile first");
    // TODO: Return actual NVRHI texture
    return nullptr;
}

nvrhi::IBuffer* FrameGraph::GetPhysicalBuffer(VirtualResourceHandle handle) const {
    const ResourceNode* node = GetResourceNode(handle);
    VERIFY(node != nullptr);
    VERIFY(node->isAllocated && "Resource not allocated - call Compile first");
    // TODO: Return actual NVRHI buffer
    return nullptr;
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
    // TODO: Destroy allocated resources (except imported)

    // Clear state
    m_resources.clear();
    m_passes.clear();
    m_sortedPasses.clear();
    m_compiled = false;

    // Reset statistics
    memset(&m_stats, 0, sizeof(m_stats));
}

// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP
//  UTILITIES & DEBUGGING (STUBS)
// PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP

void FrameGraph::ExportVisualization(const char* htmlPath) const {
    // TODO: Implement graph visualization export
    Msg("~ [FrameGraph] ExportVisualization not yet implemented");
}

void FrameGraph::PrintStatistics() const {
    Msg("=== FrameGraph Statistics ===");
    Msg("Passes: %u total, %u culled", m_stats.numPasses, m_stats.numCulledPasses);
    Msg("Resources: %u total, %u culled", m_stats.numResources, m_stats.numCulledResources);
    Msg("Compile time: %.2f ms", m_stats.compileTimeMs);
    Msg("Execute time: %.2f ms", m_stats.executeTimeMs);
}

void FrameGraph::PrintExecutionOrder() const {
    Msg("=== FrameGraph Execution Order ===");
    for (u32 i = 0; i < m_sortedPasses.size(); i++) {
        const PassNode* pass = m_sortedPasses[i];
        Msg("  [%u] %s", i, pass->name.c_str());
    }
}

bool FrameGraph::ValidateGraph() const {
    // TODO: Implement validation
    return true;
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
    // TODO: Week 10
}

void FrameGraph::InsertResourceBarriers() {
    // TODO: Week 10
}

void FrameGraph::OptimizeMemoryAliasing() {
    // TODO: Week 10
}

} // namespace xray::render::framegraph
