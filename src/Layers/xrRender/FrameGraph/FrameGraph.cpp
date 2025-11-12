#include "stdafx.h"
#include "FrameGraph.h"
#include "../RenderContext/RenderDevice.h"

namespace xray::render::framegraph {

FrameGraph::FrameGraph(ng::RenderDevice* renderDevice)
    : m_renderDevice(renderDevice)
    , m_device(renderDevice->GetNVRHIDevice())
    , m_resourceManager(renderDevice->GetFGResourceManager())
{
    VERIFY(renderDevice != nullptr);
    VERIFY(m_device != nullptr);

    if (m_resourceManager) {
        m_resourcePool = xr_make_unique<FGResourcePool>(m_resourceManager);
        Msg("* [FrameGraph] Initialized with ResourceManager");
    } else {
        Msg("* [FrameGraph] Initialized without ResourceManager (direct NVRHI mode)");
    }

    m_resources.reserve(256);
    m_passes.reserve(128);
    m_sortedPasses.reserve(128);
}

FrameGraph::~FrameGraph() {
    Reset();

    Msg("* [FrameGraph] Destroyed");
};

VirtualResourceHandle FrameGraph::CreateTexture(const char* name, const ResourceDesc& desc) {
    VERIFY(!m_compiled && "Cannot create resources after compile");
    VERIFY(desc.type != ResourceDesc::Type::Buffer && "Use CreateBuffer for buffers");

    ResourceNode node(desc);
    node.handle.index = static_cast<u32>(m_resources.size());

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

    ResourceNode node(desc);
    node.handle.index = static_cast<u32>(m_resources.size());

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

    ResourceNode node(desc);
    node.handle.index = static_cast<u32>(m_resources.size());
    node.nvrhiTexture = physicalTexture;
    node.isAllocated = true;
    node.canAlias = false;
    node.isPersistent = true;

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

    ResourceNode node(desc);
    node.handle.index = static_cast<u32>(m_resources.size());
    node.nvrhiBuffer = physicalBuffer;
    node.isAllocated = true;
    node.canAlias = false;
    node.isPersistent = true;

    m_resources.push_back(node);

    Msg("~ [FrameGraph] Imported buffer '%s'", name);

    return node.handle;
}

PassHandle FrameGraph::AddPass(const char* name) {
    VERIFY(!m_compiled && "Cannot add passes after compile");

    PassNode pass(name);
    pass.handle.index = static_cast<u32>(m_passes.size());

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

void FrameGraph::Compile() {
    VERIFY(!m_compiled && "Already compiled");

    Msg("~ [FrameGraph] Compiling graph...");

    BuildDependencyGraph();

    TopologicalSort();

    CullUnusedPasses();

    ComputeResourceLifetimes();

    AllocateResources();

    InsertResourceBarriers();

    OptimizeMemoryAliasing();

    m_compiled = true;
    m_stats.numPasses = static_cast<u32>(m_passes.size());
    m_stats.numResources = static_cast<u32>(m_resources.size());

    Msg("~ [FrameGraph] Compilation complete (%u passes, %u resources)",
        m_stats.numPasses, m_stats.numResources);
}

void FrameGraph::Execute() {
    VERIFY(m_compiled && "Must compile before execute");
    VERIFY(m_context != nullptr && "RenderContext required for execution");
    VERIFY(m_renderDevice != nullptr && "RenderDevice required for execution");

    nvrhi::ICommandList* cmdList = m_context->GetCommandList();
    VERIFY(cmdList != nullptr);
    cmdList->open();

    u32 passesExecuted = 0;

    for (PassNode* pass : m_sortedPasses) {
        if (pass->culled) {
            continue;
        }

        if (!pass->executeCallback) {
            Msg("! [FrameGraph] Pass '%s' has no execute callback - skipping",
                pass->name.c_str());
            continue;
        }

        if (!pass->barriersBeforePass.empty()) {
            for (const auto& barrier : pass->barriersBeforePass) {
                ResourceNode* resource = GetResourceNode(barrier.resource);
                if (!resource || !resource->nvrhiTexture) {
                    continue;
                }

                nvrhi::ResourceStates nvrhiBefore = ConvertToNVRHIState(barrier.stateBefore);
                nvrhi::ResourceStates nvrhiAfter = ConvertToNVRHIState(barrier.stateAfter);

            }
        }

        pass->executeCallback(*m_context, *this);

        passesExecuted++;
    }

    cmdList->close();
    m_renderDevice->ExecuteContext(m_context);
}

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

void FrameGraph::ResetForNextFrame() {

    m_stats.compileTimeMs = 0.0f;
    m_stats.executeTimeMs = 0.0f;
    m_stats.totalGPUTimeMs = 0.0f;
    m_stats.passTimings.clear();

}

void FrameGraph::Reset() {
    if (m_resourceManager) {
        resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
        resources::BufferManager* bufManager = m_resourceManager->GetBufferManager();

        for (auto& resource : m_resources) {
            if (resource.desc.isImported) {
                continue;
            }

            if (resource.resourceTexture.IsValid()) {
                texManager->Release(resource.resourceTexture);
                resource.resourceTexture = resources::TextureHandle();
            }

            if (resource.resourceBuffer.IsValid()) {
                bufManager->Release(resource.resourceBuffer);
                resource.resourceBuffer = resources::BufferHandle();
            }
        }
    }

    if (m_resourcePool) {
        m_resourcePool->Reset();
    }

    for (auto& resource : m_resources) {
        if (resource.desc.isImported) {
            continue;
        }

        if (resource.nvrhiTexture) {
            resource.nvrhiTexture = nullptr;
        }

        if (resource.nvrhiBuffer) {
            resource.nvrhiBuffer = nullptr;
        }
    }

    m_resources.clear();
    m_passes.clear();
    m_sortedPasses.clear();
    m_rtRegistry.Clear();
    m_compiled = false;

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
    m_stats.passTimings.clear();

    Msg("~ [FrameGraph] Reset complete");
}

void FrameGraph::ExportVisualization(const char* htmlPath) const {
    Msg("~ [FrameGraph] ExportVisualization not yet implemented");
}

void FrameGraph::PrintStatistics() const {
    Msg("========================================");
    Msg("  FrameGraph Statistics");
    Msg("========================================");

    Msg("Passes:");
    Msg("  Total: %u", m_stats.numPasses);
    Msg("  Executed: %u", m_stats.numPasses - m_stats.numCulledPasses);
    Msg("  Culled: %u", m_stats.numCulledPasses);

    Msg("Resources:");
    Msg("  Total: %u", m_stats.numResources);
    Msg("  Allocated: %u", m_stats.numResources - m_stats.numCulledResources);
    Msg("  Culled: %u", m_stats.numCulledResources);

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

    Msg("Timing:");
    Msg("  Compile: %.2f ms", m_stats.compileTimeMs);
    Msg("  Execute (CPU): %.2f ms", m_stats.executeTimeMs);
    if (m_stats.totalGPUTimeMs > 0.0f) {
        Msg("  Execute (GPU): %.2f ms", m_stats.totalGPUTimeMs);
    }

    if (!m_stats.passTimings.empty()) {
        Msg("Pass Timings:");

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

    for (const auto& pass : m_passes) {
        if (pass.culled) continue;

        if (!pass.executeCallback) {
            Msg("! [FrameGraph] ERROR: Pass '%s' has no execute callback", pass.name.c_str());
            valid = false;
        }
    }

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
    return nullptr;
}

bool FrameGraph::HasCyclicDependency() const {
    return false;
}

void FrameGraph::BuildDependencyGraph() {
    for (auto& pass : m_passes) {
        pass.dependsOn.clear();
        pass.dependents.clear();
    }

    xr_map<u32, PassNode*> lastWriter;

    for (auto& pass : m_passes) {
        xr_vector<VirtualResourceHandle> readResources;
        pass.GetReadResources(readResources);

        for (const auto& resource : readResources) {
            auto it = lastWriter.find(resource.index);
            if (it != lastWriter.end()) {
                PassNode* producer = it->second;

                if (!pass.DependsOn(producer)) {
                    pass.dependsOn.push_back(producer);
                    producer->dependents.push_back(&pass);
                }
            }
        }

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

    xr_map<PassNode*, u32> inDegree;
    for (auto& pass : m_passes) {
        inDegree[&pass] = static_cast<u32>(pass.dependsOn.size());
    }

    xr_vector<PassNode*> queue;
    for (auto& pass : m_passes) {
        if (inDegree[&pass] == 0) {
            queue.push_back(&pass);
        }
    }

    u32 executionOrder = 0;
    while (!queue.empty()) {
        PassNode* current = queue.back();
        queue.pop_back();

        current->executionOrder = executionOrder++;
        m_sortedPasses.push_back(current);

        for (PassNode* dependent : current->dependents) {
            inDegree[dependent]--;

            if (inDegree[dependent] == 0) {
                queue.push_back(dependent);
            }
        }
    }

    if (m_sortedPasses.size() != m_passes.size()) {
        Msg("! [FrameGraph] Cycle detected in dependency graph!");
        Msg("! [FrameGraph] Processed %u/%u passes",
            m_sortedPasses.size(), m_passes.size());

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
    for (auto& pass : m_passes) {
        pass.culled = true;
    }

    xr_vector<PassNode*> terminalPasses;

    for (auto& pass : m_passes) {
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

    if (terminalPasses.empty()) {
        Msg("~ [FrameGraph] No terminal passes found - keeping all passes");
        for (auto& pass : m_passes) {
            pass.culled = false;
        }
        return;
    }

    xr_vector<PassNode*> queue = terminalPasses;

    while (!queue.empty()) {
        PassNode* current = queue.back();
        queue.pop_back();

        if (current->culled) {
            current->culled = false;

            for (PassNode* dependency : current->dependsOn) {
                if (dependency->culled) {
                    queue.push_back(dependency);
                }
            }
        }
    }

    u32 culledCount = 0;
    for (const auto& pass : m_passes) {
        if (pass.culled) {
            culledCount++;
            Msg("~ [FrameGraph] Culling unused pass: %s", pass.name.c_str());
        }
    }

    auto it = std::remove_if(m_sortedPasses.begin(), m_sortedPasses.end(),
        [](const PassNode* pass) { return pass->culled; });
    m_sortedPasses.erase(it, m_sortedPasses.end());

    m_stats.numCulledPasses = culledCount;

    Msg("~ [FrameGraph] Pass culling complete: %u/%u passes culled",
        culledCount, m_passes.size());
}

void FrameGraph::ComputeResourceLifetimes() {
    for (auto& resource : m_resources) {
        resource.firstUsedPass = INVALID_INDEX;
        resource.lastUsedPass = INVALID_INDEX;
        resource.refCount = 0;
    }

    for (const PassNode* pass : m_sortedPasses) {
        if (pass->culled) continue;

        u32 passIndex = pass->executionOrder;

        for (const auto& access : pass->resourceAccesses) {
            if (!access.resource.is_valid()) continue;

            ResourceNode* resource = GetResourceNode(access.resource);
            if (!resource) continue;

            if (resource->firstUsedPass == INVALID_INDEX) {
                resource->firstUsedPass = passIndex;
            }

            resource->lastUsedPass = passIndex;

            resource->refCount++;
        }
    }

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

    if (m_resourcePool) {
        Msg("~ [FrameGraph] Allocating resources via FGResourcePool...");
    } else {
        Msg("~ [FrameGraph] Allocating resources directly via NVRHI...");
    }

    for (auto& resource : m_resources) {
        if (resource.firstUsedPass == INVALID_INDEX) {
            continue;
        }

        if (resource.desc.isImported) {
            resource.isAllocated = true;
            continue;
        }

        bool usePersistent = resource.isPersistent || resource.desc.isRenderTarget || resource.desc.isDepthStencil;

        if (resource.desc.type == ResourceDesc::Type::Buffer) {
            if (m_resourcePool) {
                resources::BufferDesc rmBufferDesc;
                rmBufferDesc.type = resources::BufferType::Structured;
                rmBufferDesc.usage = resources::BufferUsage::Static;
                rmBufferDesc.size = resource.desc.bufferSize;
                rmBufferDesc.stride = resource.desc.structStride;
                rmBufferDesc.gpuWrite = resource.desc.allowUAV;
                rmBufferDesc.cpuAccess = false;
                rmBufferDesc.debugName = resource.desc.debugName;

                resources::BufferHandle rmHandle = m_resourcePool->AllocateBuffer(rmBufferDesc);

                if (rmHandle.IsValid()) {
                    resource.resourceBuffer = rmHandle;

                    resources::BufferManager* bufManager = m_resourceManager->GetBufferManager();
                    resource.nvrhiBuffer = bufManager->GetNVRHIBuffer(rmHandle);
                    resource.isAllocated = (resource.nvrhiBuffer != nullptr);
                    totalMemoryAllocated += resource.memorySize;

                    Msg("~ [FrameGraph] Allocated buffer '%s' via ResourceManager: %.2f MB",
                        resource.desc.debugName.c_str(),
                        resource.memorySize / (1024.0f * 1024.0f));

                    continue;
                } else {
                    Msg("! [FrameGraph] Failed to allocate buffer '%s' via ResourceManager, falling back to NVRHI",
                        resource.desc.debugName.c_str());
                }
            }

            nvrhi::BufferDesc nvrhiDesc;
            nvrhiDesc.byteSize = resource.desc.bufferSize;
            nvrhiDesc.structStride = resource.desc.structStride;
            nvrhiDesc.debugName = resource.desc.debugName.c_str();
            nvrhiDesc.initialState = nvrhi::ResourceStates::Common;
            nvrhiDesc.keepInitialState = false;
            nvrhiDesc.canHaveUAVs = resource.desc.allowUAV;

            resource.nvrhiBuffer = m_device->createBuffer(nvrhiDesc);

            if (resource.nvrhiBuffer) {
                resource.isAllocated = true;
                totalMemoryAllocated += resource.memorySize;

                Msg("~ [FrameGraph] Allocated buffer '%s' directly: %.2f MB",
                    resource.desc.debugName.c_str(),
                    resource.memorySize / (1024.0f * 1024.0f));
            } else {
                Msg("! [FrameGraph] Failed to create NVRHI buffer '%s'",
                    resource.desc.debugName.c_str());
            }
        } else {
            if (m_resourcePool) {
                resources::TextureDesc rmTexDesc;
                rmTexDesc.width = resource.desc.width;
                rmTexDesc.height = resource.desc.height;
                rmTexDesc.depth = resource.desc.depth;
                rmTexDesc.arraySize = resource.desc.arraySize;
                rmTexDesc.mipLevels = resource.desc.mipLevels;
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
                        rmTexDesc.type = resources::TextureDesc::Texture2D;
                        break;
                }

                rmTexDesc.isRenderTarget = resource.desc.isRenderTarget;
                rmTexDesc.isDepthStencil = resource.desc.isDepthStencil;
                rmTexDesc.isUAV = resource.desc.isUAV || resource.desc.allowUAV;

                resources::TextureHandle rmHandle;
                if (usePersistent) {
                    rmHandle = m_resourcePool->AllocatePersistentTexture(rmTexDesc);
                } else {
                    rmHandle = m_resourcePool->AllocateTexture(rmTexDesc);
                }

                if (rmHandle.IsValid()) {
                    resource.resourceTexture = rmHandle;

                    resources::TextureManager* texManager = m_resourceManager->GetTextureManager();
                    resource.nvrhiTexture = texManager->GetNVRHITexture(rmHandle);
                    resource.isAllocated = (resource.nvrhiTexture != nullptr);
                    totalMemoryAllocated += resource.memorySize;

                    Msg("~ [FrameGraph] Allocated texture '%s' via ResourceManager%s: %ux%ux%u, %.2f MB",
                        resource.desc.debugName.c_str(),
                        !usePersistent ? " (transient)" : "",
                        resource.desc.width,
                        resource.desc.height,
                        resource.desc.depth,
                        resource.memorySize / (1024.0f * 1024.0f));

                    continue;
                } else {
                    Msg("! [FrameGraph] Failed to allocate texture '%s' via ResourceManager, falling back to NVRHI",
                        resource.desc.debugName.c_str());
                }
            }

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
                nvrhiDesc.keepInitialState = false;

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

                nvrhiDesc.isRenderTarget = resource.desc.isRenderTarget;
                nvrhiDesc.isUAV = resource.desc.isUAV || resource.desc.allowUAV;
                nvrhiDesc.isShaderResource = true;

                if (resource.desc.isDepthStencil) {
                    nvrhiDesc.isRenderTarget = true;
                    nvrhiDesc.isTypeless = true;
                    nvrhiDesc.useClearValue = true;
                    nvrhiDesc.clearValue = nvrhi::Color(1.0f);
                }

            resource.nvrhiTexture = m_device->createTexture(nvrhiDesc);

            if (resource.nvrhiTexture) {
                resource.isAllocated = true;
                totalMemoryAllocated += resource.memorySize;

                Msg("~ [FrameGraph] Allocated texture '%s'%s%s: %ux%ux%u, %.2f MB",
                    resource.desc.debugName.c_str(),
                    m_resourcePool ? " via pool" : "",
                    (m_resourcePool && !usePersistent) ? " (transient)" : "",
                    resource.desc.width,
                    resource.desc.height,
                    resource.desc.depth,
                    resource.memorySize / (1024.0f * 1024.0f));
            } else {
                Msg("! [FrameGraph] Failed to create NVRHI texture '%s'",
                    resource.desc.debugName.c_str());
            }
        }
    }

    m_stats.totalMemoryAllocated = totalMemoryAllocated;

    Msg("~ [FrameGraph] Resource allocation complete: %.2f MB total",
        totalMemoryAllocated / (1024.0f * 1024.0f));

    if (m_resourcePool) {
        m_resourcePool->PrintStatistics();
    }
}

void FrameGraph::InsertResourceBarriers() {
    xr_map<u32, ResourceState> currentStates;

    for (auto& resource : m_resources) {
        currentStates[resource.handle.index] = ResourceState::Undefined;
        resource.currentState = ResourceState::Undefined;
    }

    u32 totalBarriers = 0;

    for (PassNode* pass : m_sortedPasses) {
        if (pass->culled) continue;

        pass->barriersBeforePass.clear();

        for (const auto& access : pass->resourceAccesses) {
            if (!access.resource.is_valid()) continue;

            ResourceNode* resource = GetResourceNode(access.resource);
            if (!resource) continue;

            ResourceState currentState = currentStates[access.resource.index];
            ResourceState requiredState = access.state;

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

    std::sort(transientResources.begin(), transientResources.end(),
        [](const ResourceNode* a, const ResourceNode* b) {
            return a->memorySize > b->memorySize;
        });

    u32 aliasedCount = 0;
    u64 memoryReduced = 0;

    for (size_t i = 0; i < transientResources.size(); i++) {
        ResourceNode* current = transientResources[i];

        if (current->aliasedWith != INVALID_INDEX) {
            continue;
        }

        for (size_t j = 0; j < i; j++) {
            ResourceNode* candidate = transientResources[j];

            if (!current->OverlapsWith(*candidate)) {
                bool compatible = true;

                if (current->desc.type != candidate->desc.type) {
                    compatible = false;
                }

                if (current->desc.type != ResourceDesc::Type::Buffer &&
                    current->desc.format != candidate->desc.format) {
                    compatible = false;
                }

                if (candidate->memorySize < current->memorySize) {
                    compatible = false;
                }

                if (compatible) {
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

}
