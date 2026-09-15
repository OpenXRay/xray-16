#include "stdafx.h"
#include "GpuParticleManager.h"
#include "GpuParticleTranslate.h"
#include "GpuParticleCollision.h"
#include "ParticleEffectDef.h"
#include "PSLibrary.h"
#include "r_FrameGraphRenderer.h"
#include "FrameGraph/FrameGraph.h"
#include "FrameGraph/ShaderLoader.h"
#include "FrameGraph/BindingSetBuilder.h"
#include "FrameGraph/BindingLayoutBuilder.h"
#include "FrameGraphPasses/PassCommon.h"
#include "xrParticles/noise.h"
#include <atomic>
#include <algorithm>
#include <array>
#include <mutex>
#include <limits>

namespace xray::render::fg {
namespace {
std::atomic<u32> generationCounter{1};
u32 CapacityFor(u64 required) {
    R_ASSERT2(required <= 0x40000000u,"GPU PAPI capacity exceeds addressable pool");
    u32 result = 1;
    while (result < required) result <<= 1;
    return result;
}
}
struct GpuParticleManager::Impl {
    struct Root {
        u32 generation = 0,serial = 0,lifecycleSerial = 0,program = 0,collisionFlags = 0;
        bool live = false,retiring = false,hasSnapshot = false,hud = false,hasParent = false;
        GpuPapiCommand parent{};
        GpuParticleSnapshot snapshot;
        xr_vector<GpuPapiCommand> commands;
    };
    struct Migration { nvrhi::BufferHandle destination,source; bool clear; };
    struct Readback {
        nvrhi::BufferHandle buffer;
        nvrhi::TimerQueryHandle query;
        u32 roots = 0,particleCapacity = 0,emitterCapacity = 0,stateCapacity = 0;
        bool recorded = false;
    };
    struct Kernel {
        framegraph::ShaderLoader::ShaderResult shader;
        nvrhi::BindingLayoutHandle layout;
        nvrhi::ComputePipelineHandle pipeline;
        nvrhi::BindingSetHandle binding;
    };
    mutable std::mutex mutex;
    nvrhi::IDevice* device = nullptr;
    xr_vector<Root> roots;
    xr_vector<const PS::CPEDef*> definitions;
    xr_vector<GpuPapiProgram> programs;
    xr_vector<GpuPapiAction> actions;
    xr_vector<Migration> migrations;
    GpuParticleDrawResources draw;
    nvrhi::BufferHandle actionBuffer,actionState,particleAllocation,stateAllocation,commands,rootCommands,visibleRoots,status,errors,noise,constants,visibilityConstants,cursors;
    Kernel initialize,simulate,compact;
    Readback readbacks[3];
    GpuParticleCollision collision;
    u32 emitterCapacity = 0,stateCapacity = 0;
    bool initialized = false,definitionsDirty = true,noiseUploaded = false;
    Root* Find(GpuParticleHandle handle) {
        if (!handle) return nullptr;
        u32 index = u32(handle) - 1;
        if (index >= roots.size()) return nullptr;
        Root& root = roots[index];
        return root.live && root.generation == u32(handle >> 32) ? &root : nullptr;
    }
    u32 Register(const PS::CPEDef& definition) {
        for (u32 i = 0; i < definitions.size(); ++i) if (definitions[i] == &definition) return i;
        GpuPapiProgram program{};
        xr_vector<GpuPapiAction> translated;
        xr_string error;
        if (!TranslateGpuParticleDefinition(definition,program,translated,error))
            xrDebug::Fatal(DEBUG_INFO,"GPU PAPI definition '%s': %s",definition.Name(),error.c_str());
        R_ASSERT2(actions.size() + translated.size() <= std::numeric_limits<u32>::max(),"GPU PAPI action table overflow");
        program.actionFirst = u32(actions.size());
        program.actionCount = u32(translated.size());
        program.materialID = GPU_PAPI_INVALID;
        u32 index = u32(programs.size());
        programs.push_back(program);
        definitions.push_back(&definition);
        actions.insert(actions.end(),translated.begin(),translated.end());
        definitionsDirty = true;
        return index;
    }
    GpuPapiCommand& Queue(Root& root,u32 type,bool lifecycle = false) {
        R_ASSERT2(root.serial != std::numeric_limits<u32>::max(),"GPU PAPI command serial exhausted");
        GpuPapiCommand command{};
        command.type = type;
        command.emitter = u32(&root - roots.data()) * 2;
        command.generation = root.generation;
        command.serial = ++root.serial;
        if (lifecycle) root.lifecycleSerial = command.serial;
        root.commands.push_back(command);
        return root.commands.back();
    }
    void Buffer(nvrhi::BufferHandle& buffer,u64 count,u32 stride,const char* name,bool uav,bool preserve = false,bool indirect = false) {
        R_ASSERT2(count <= std::numeric_limits<u32>::max() / stride,"GPU PAPI buffer exceeds supported resource size");
        u64 bytes = std::max<u64>(count,1) * stride;
        if (buffer && buffer->getDesc().byteSize >= bytes) return;
        nvrhi::BufferDesc desc;
        desc.byteSize = u64(CapacityFor(std::max<u64>(count,1))) * stride;
        R_ASSERT2(desc.byteSize <= std::numeric_limits<u32>::max(),"GPU PAPI rounded buffer exceeds supported resource size");
        desc.structStride = stride;
        desc.canHaveUAVs = uav;
        desc.canHaveRawViews = true;
        desc.isDrawIndirectArgs = indirect;
        desc.keepInitialState = true;
        desc.initialState = uav ? nvrhi::ResourceStates::UnorderedAccess : nvrhi::ResourceStates::ShaderResource;
        desc.debugName = name;
        auto replacement = device->createBuffer(desc);
        R_ASSERT2(replacement,"GPU PAPI buffer allocation failed");
        if (preserve && initialized) migrations.push_back({replacement,buffer,true});
        buffer = replacement;
        initialize.binding = nullptr;
        compact.binding = nullptr;
    }
    void KernelLoad(Kernel& kernel,const char* name) {
        if (kernel.pipeline) return;
        auto* loader = GEnv.Render->GetShaderLoader();
        R_ASSERT2(loader,"GPU PAPI requires the renderer shader loader");
        kernel.shader = loader->LoadComputeShader(name);
        R_ASSERT3(kernel.shader.handle && kernel.shader.reflection,"GPU PAPI shader compilation failed",name);
        kernel.layout = device->createBindingLayout(framegraph::BindingLayoutBuilder::Build(*kernel.shader.reflection,nvrhi::ShaderType::Compute));
        nvrhi::ComputePipelineDesc desc;
        desc.CS = kernel.shader.handle;
        desc.bindingLayouts = {kernel.layout};
        kernel.pipeline = device->createComputePipeline(desc);
        R_ASSERT3(kernel.pipeline,"GPU PAPI pipeline creation failed",name);
    }
    void Poll() {
        for (auto& readback : readbacks) {
            if (!readback.recorded || !device->pollTimerQuery(readback.query)) continue;
            const auto* data = static_cast<const GpuPapiStatus*>(device->mapBuffer(readback.buffer,nvrhi::CpuAccessMode::Read));
            R_ASSERT2(data,"GPU PAPI completed status readback mapping failed");
            u32 error = 0;
            memcpy(&error,reinterpret_cast<const u8*>(data) + size_t(readback.roots) * sizeof(GpuPapiStatus),sizeof(error));
            for (u32 i = 0; i < readback.roots && i < roots.size(); ++i) {
                Root& root = roots[i];
                const auto& value = data[i];
                if (value.generation != root.generation) continue;
                auto acknowledged = std::upper_bound(root.commands.begin(),root.commands.end(),value.serial,
                    [](u32 serial,const GpuPapiCommand& command) { return serial < command.serial; });
                root.commands.erase(root.commands.begin(),acknowledged);
                if (root.retiring && value.retired && value.serial >= root.lifecycleSerial) root.retiring = false;
                if (!root.live || value.serial < root.lifecycleSerial || (root.hasSnapshot && value.serial < root.snapshot.serial)) continue;
                root.snapshot.count = value.count;
                root.snapshot.playing = value.playing != 0;
                root.snapshot.serial = value.serial;
                root.snapshot.bounds.set(value.boundsMin,value.boundsMax);
                root.hasSnapshot = true;
            }
            device->unmapBuffer(readback.buffer);
            device->resetTimerQuery(readback.query);
            readback.recorded = false;
            if (error & ~7u)
                xrDebug::Fatal(DEBUG_INFO,"GPU PAPI invalid state or unsupported allocation size (mask %u; particles %u, emitters %u, action states %u)",error,readback.particleCapacity,readback.emitterCapacity,readback.stateCapacity);
            if ((error & 1u) && draw.particleCapacity <= readback.particleCapacity)
                draw.particleCapacity = CapacityFor(u64(readback.particleCapacity) * 2);
            if ((error & 2u) && emitterCapacity <= readback.emitterCapacity)
                emitterCapacity = CapacityFor(u64(readback.emitterCapacity) * 2);
            if ((error & 4u) && stateCapacity <= readback.stateCapacity)
                stateCapacity = CapacityFor(u64(readback.stateCapacity) * 2);
        }
    }
    void Ensure(nvrhi::IDevice* requested) {
        R_ASSERT2(!device || device == requested,"GPU PAPI device changed without renderer teardown");
        device = requested;
        Poll();
        emitterCapacity = std::max(emitterCapacity,CapacityFor(std::max<u64>(u64(roots.size()) * 2,4096)));
        draw.particleCapacity = std::max(draw.particleCapacity,65536u);
        stateCapacity = std::max(stateCapacity,16384u);
        Buffer(draw.particles,draw.particleCapacity,sizeof(GpuPapiParticle),"GpuPapiParticles",true,true);
        Buffer(draw.emitters,emitterCapacity,sizeof(GpuPapiEmitter),"GpuPapiEmitters",true,true);
        Buffer(actionState,stateCapacity,sizeof(Fvector4),"GpuPapiActionState",true,true);
        Buffer(particleAllocation,draw.particleCapacity,sizeof(u32),"GpuPapiParticleAllocation",true,true);
        Buffer(stateAllocation,stateCapacity,sizeof(u32),"GpuPapiStateAllocation",true,true);
        Buffer(draw.programs,programs.size(),sizeof(GpuPapiProgram),"GpuPapiPrograms",false);
        Buffer(actionBuffer,actions.size(),sizeof(GpuPapiAction),"GpuPapiActions",false);
        Buffer(draw.indices,draw.particleCapacity,sizeof(u32),"GpuPapiDrawIndices",true);
        Buffer(draw.drawArgs,14,sizeof(u32) * 4,"GpuPapiDrawArgs",true,false,true);
        Buffer(draw.bucketOffsets,14,sizeof(u32),"GpuPapiBucketOffsets",true);
        Buffer(cursors,14,sizeof(u32),"GpuPapiBucketCursors",true);
        Buffer(status,roots.size(),sizeof(GpuPapiStatus),"GpuPapiStatus",true,true);
        Buffer(errors,1,sizeof(u32),"GpuPapiErrors",true);
        Buffer(noise,514,sizeof(Fvector4),"GpuPapiNoise",false);
        if (!constants) {
            nvrhi::BufferDesc desc;
            desc.byteSize = sizeof(GpuPapiConstants);
            desc.isConstantBuffer = true;
            desc.isVolatile = true;
            desc.maxVersions = 32;
            desc.debugName = "GpuPapiParams";
            constants = device->createBuffer(desc);
            R_ASSERT2(constants,"GPU PAPI constant buffer allocation failed");
        }
        if (!visibilityConstants) {
            nvrhi::BufferDesc desc;
            desc.byteSize = sizeof(Fvector4) * 6;
            desc.isConstantBuffer = true;
            desc.isVolatile = true;
            desc.maxVersions = 32;
            desc.debugName = "GpuPapiVisibility";
            visibilityConstants = device->createBuffer(desc);
            R_ASSERT2(visibilityConstants,"GPU PAPI visibility constant buffer allocation failed");
        }
        KernelLoad(initialize,"gpu_particle_init");
        KernelLoad(simulate,"gpu_particle_simulate");
        KernelLoad(compact,"gpu_particle_compact");
    }
    void Dispatch(nvrhi::ICommandList* commandList,Kernel& kernel,GpuPapiConstants params,u32 groups) {
        R_ASSERT2(groups <= 65535u,"GPU PAPI dispatch exceeds supported resource size");
        commandList->writeBuffer(constants,&params,sizeof(params));
        auto binding = kernel.binding;
        if (!binding) {
            framegraph::BindingSetBuilder builder(*kernel.shader.reflection,device,"GpuPapi");
            builder.ConstantBuffer("GpuPapiParams",constants);
            if (&kernel == &compact) {
                builder.ConstantBuffer("GpuPapiVisibility",visibilityConstants)
                    .BufferSRV("g_Emitters",draw.emitters).BufferSRV("g_Programs",draw.programs).BufferSRV("g_VisibleRoots",visibleRoots)
                    .BufferUAV("g_Indices",draw.indices).BufferUAV("g_DrawArgs",draw.drawArgs)
                    .BufferUAV("g_BucketOffsets",draw.bucketOffsets).BufferUAV("g_BucketCursors",cursors);
            } else {
                builder.BufferUAV("g_Emitters",draw.emitters).BufferUAV("g_ParticleAllocation",particleAllocation)
                    .BufferUAV("g_StateAllocation",stateAllocation).BufferUAV("g_Errors",errors);
                if (&kernel == &simulate) {
                    builder.BufferUAV("g_Particles",draw.particles).BufferUAV("g_ActionState",actionState)
                        .BufferSRV("g_Programs",draw.programs).BufferSRV("g_Actions",actionBuffer)
                        .BufferSRV("g_Commands",commands).BufferSRV("g_RootCommands",rootCommands)
                        .BufferSRV("g_PapiNoise",noise).BufferUAV("g_Status",status);
                    collision.Bind(builder);
                }
            }
            binding = device->createBindingSet(builder.Build(),kernel.layout);
            if (&kernel != &simulate) kernel.binding = binding;
        }
        R_ASSERT2(binding,"GPU PAPI shader resource binding failed");
        nvrhi::ComputeState state;
        state.pipeline = kernel.pipeline;
        state.bindings = {binding};
        commandList->setComputeState(state);
        commandList->dispatch(groups,1,1);
    }
    void Execute(nvrhi::ICommandList* commandList,const xr_vector<GpuPapiCommand>& batch,const xr_vector<std::array<u32,2>>& ranges,const xr_vector<u32>& visible,u32 rootCount,u32 collisionFlags) {
        std::lock_guard lock(mutex);
        GpuPapiConstants params{draw.particleCapacity,emitterCapacity,stateCapacity,u32(ranges.size()),u32(batch.size()),u32(programs.size()),0,0};
        for (const auto& migration : migrations) {
            if (migration.clear) commandList->clearBufferUInt(migration.destination,0);
            if (migration.source) commandList->copyBuffer(migration.destination,0,migration.source,0,migration.source->getDesc().byteSize);
        }
        migrations.clear();
        if (!initialized) {
            Dispatch(commandList,initialize,params,(std::max({emitterCapacity,draw.particleCapacity,stateCapacity}) + 255) / 256);
            commandList->clearBufferUInt(status,0);
            initialized = true;
        }
        if (definitionsDirty) {
            if (!programs.empty()) commandList->writeBuffer(draw.programs,programs.data(),programs.size() * sizeof(GpuPapiProgram));
            if (!actions.empty()) commandList->writeBuffer(actionBuffer,actions.data(),actions.size() * sizeof(GpuPapiAction));
            definitionsDirty = false;
        }
        if (!noiseUploaded) {
            int permutation[514];
            float gradients[514 * 3];
            Fvector4 rows[514];
            PAPI::GetNoise3Tables(permutation,gradients);
            for (u32 i = 0; i < 514; ++i) {
                rows[i].set(gradients[i * 3],gradients[i * 3 + 1],gradients[i * 3 + 2],0);
                memcpy(&rows[i].w,&permutation[i],sizeof(u32));
            }
            commandList->writeBuffer(noise,rows,sizeof(rows));
            noiseUploaded = true;
        }
        if (!batch.empty()) commandList->writeBuffer(commands,batch.data(),batch.size() * sizeof(GpuPapiCommand));
        if (!ranges.empty()) commandList->writeBuffer(rootCommands,ranges.data(),ranges.size() * sizeof(ranges[0]));
        if (!visible.empty()) commandList->writeBuffer(visibleRoots,visible.data(),visible.size() * sizeof(u32));
        collision.Update(device,commandList,collisionFlags);
        if (!ranges.empty()) {
            params.phase = 1;
            Dispatch(commandList,initialize,params,1);
            params.phase = 0;
            Dispatch(commandList,simulate,params,(u32(ranges.size()) + 31) / 32);
        }
        params.rootCapacity = u32(visible.size());
        Fvector4 planes[6]{};
        passes::ExtractFrustumPlanes(planes);
        commandList->writeBuffer(visibilityConstants,planes,sizeof(planes));
        commandList->clearBufferUInt(draw.drawArgs,0);
        if (!visible.empty()) Dispatch(commandList,compact,params,(u32(visible.size()) + 63) / 64);
        params.phase = 1;
        Dispatch(commandList,compact,params,1);
        params.phase = 2;
        if (!visible.empty()) Dispatch(commandList,compact,params,(u32(visible.size()) + 63) / 64);
        if (ranges.empty()) return;
        for (auto& readback : readbacks) {
            if (readback.recorded) continue;
            const u64 bytes = u64(rootCount) * sizeof(GpuPapiStatus) + sizeof(u32);
            if (!readback.buffer || readback.buffer->getDesc().byteSize < bytes) {
                nvrhi::BufferDesc desc;
                desc.byteSize = bytes;
                desc.cpuAccess = nvrhi::CpuAccessMode::Read;
                desc.initialState = nvrhi::ResourceStates::CopyDest;
                desc.keepInitialState = true;
                desc.debugName = "GpuPapiStatusReadback";
                readback.buffer = device->createBuffer(desc);
                R_ASSERT2(readback.buffer,"GPU PAPI readback allocation failed");
            }
            if (!readback.query) readback.query = device->createTimerQuery();
            R_ASSERT2(readback.query,"GPU PAPI completion query allocation failed");
            commandList->beginTimerQuery(readback.query);
            readback.roots = rootCount;
            readback.particleCapacity = draw.particleCapacity;
            readback.emitterCapacity = emitterCapacity;
            readback.stateCapacity = stateCapacity;
            if (readback.roots) commandList->copyBuffer(readback.buffer,0,status,0,u64(readback.roots) * sizeof(GpuPapiStatus));
            commandList->copyBuffer(readback.buffer,u64(readback.roots) * sizeof(GpuPapiStatus),errors,0,sizeof(u32));
            commandList->endTimerQuery(readback.query);
            readback.recorded = true;
            break;
        }
    }
};
GpuParticleManager::GpuParticleManager() : m_impl(std::make_shared<Impl>()) {}
GpuParticleManager::~GpuParticleManager() = default;
GpuParticleManager& GetGpuParticleManager() { static GpuParticleManager manager; return manager; }
GpuParticleHandle GpuParticleManager::CreateEmitter(const PS::CPEDef& definition) {
    std::lock_guard lock(m_impl->mutex);
    u32 program = m_impl->Register(definition);
    u32 index = 0;
    while (index < m_impl->roots.size() && (m_impl->roots[index].live || m_impl->roots[index].retiring)) ++index;
    R_ASSERT2(index < 0x20000000u,"GPU PAPI root handle capacity exhausted");
    if (index == m_impl->roots.size()) m_impl->roots.emplace_back();
    auto& root = m_impl->roots[index];
    root = Impl::Root{};
    root.live = true;
    root.program = program;
    root.collisionFlags = m_impl->programs[program].flags & (PS::CPEDef::dfCollision | PS::CPEDef::dfCollisionDyn);
    root.generation = generationCounter.fetch_add(1);
    R_ASSERT2(root.generation != 0,"GPU PAPI handle generation exhausted");
    m_impl->Queue(root,0,true).p0 = program;
    return (u64(root.generation) << 32) | (u64(index) + 1);
}
void GpuParticleManager::DestroyEmitter(GpuParticleHandle handle) {
    std::lock_guard lock(m_impl->mutex);
    if (auto* root = m_impl->Find(handle)) {
        m_impl->Queue(*root,1,true);
        root->live = false;
        root->retiring = true;
        root->hasSnapshot = false;
    }
}
void GpuParticleManager::Play(GpuParticleHandle handle) {
    std::lock_guard lock(m_impl->mutex);
    if (auto* root = m_impl->Find(handle)) m_impl->Queue(*root,2,true);
}
void GpuParticleManager::Stop(GpuParticleHandle handle,bool deferred) {
    std::lock_guard lock(m_impl->mutex);
    if (auto* root = m_impl->Find(handle)) m_impl->Queue(*root,3,true).p0 = deferred;
}
void GpuParticleManager::UpdateParent(GpuParticleHandle handle,const Fmatrix& matrix,const Fvector& velocity,bool localSpace) {
    std::lock_guard lock(m_impl->mutex);
    if (auto* root = m_impl->Find(handle)) {
        GpuPapiCommand command{};
        command.p0 = localSpace;
        command.basisX.set(matrix.i.x,matrix.i.y,matrix.i.z,0);
        command.basisY.set(matrix.j.x,matrix.j.y,matrix.j.z,0);
        command.basisZ.set(matrix.k.x,matrix.k.y,matrix.k.z,0);
        command.origin.set(matrix.c.x,matrix.c.y,matrix.c.z,1);
        command.velocity.set(velocity.x,velocity.y,velocity.z,0);
        if (root->hasParent && root->parent.p0 == command.p0 &&
            memcmp(&root->parent.basisX,&command.basisX,sizeof(Fvector4) * 5) == 0) return;
        root->parent = command;
        root->hasParent = true;
        auto& queued = m_impl->Queue(*root,4);
        command.type = queued.type;
        command.emitter = queued.emitter;
        command.generation = queued.generation;
        command.serial = queued.serial;
        queued = command;
    }
}
void GpuParticleManager::SetHudMode(GpuParticleHandle handle,bool enabled) {
    std::lock_guard lock(m_impl->mutex);
    if (auto* root = m_impl->Find(handle); root && root->hud != enabled) {
        m_impl->Queue(*root,5).p0 = enabled;
        root->hud = enabled;
    }
}
void GpuParticleManager::Tick(GpuParticleHandle handle,u32 milliseconds) {
    std::lock_guard lock(m_impl->mutex);
    if (auto* root = m_impl->Find(handle)) m_impl->Queue(*root,6).p0 = milliseconds;
}
bool GpuParticleManager::GetSnapshot(GpuParticleHandle handle,GpuParticleSnapshot& snapshot) const {
    std::lock_guard lock(m_impl->mutex);
    auto* root = m_impl->Find(handle);
    if (!root || !root->hasSnapshot || root->snapshot.serial < root->lifecycleSerial) return false;
    snapshot = root->snapshot;
    return true;
}
void GpuParticleManager::ConfigureChildren(GpuParticleHandle handle,const char* birth,const char* play,const char* death,u32 groupFlags) {
    std::lock_guard lock(m_impl->mutex);
    auto* root = m_impl->Find(handle);
    if (!root) return;
    auto resolve = [&](const char* name,bool freeChild) {
        if (!name) return GPU_PAPI_INVALID;
        R_ASSERT2(*name,"GPU PAPI enabled child has an empty definition name");
        auto* definition = RImplementation.m_PSLibrary.FindPED(name);
        R_ASSERT3(definition,"GPU PAPI child definition does not exist",name);
        R_ASSERT3(!freeChild || definition->m_Flags.is(PS::CPEDef::dfTimeLimit),"GPU PAPI free child cannot be a looped effect",name);
        return m_impl->Register(*definition);
    };
    u32 birthProgram = resolve((groupFlags & 32u) != 0 ? birth : nullptr,true);
    u32 playProgram = resolve((groupFlags & 2u) != 0 ? play : nullptr,false);
    u32 deathProgram = resolve((groupFlags & 64u) != 0 ? death : nullptr,true);
    for (u32 program : {birthProgram,playProgram,deathProgram})
        if (program != GPU_PAPI_INVALID)
            root->collisionFlags |= m_impl->programs[program].flags & (PS::CPEDef::dfCollision | PS::CPEDef::dfCollisionDyn);
    auto& command = m_impl->Queue(*root,7);
    command.p0 = birthProgram;
    command.p1 = playProgram;
    command.p2 = deathProgram;
    command.p3 = groupFlags;
}
const xr_vector<const PS::CPEDef*>& GpuParticleManager::GetDefinitions() const { return m_impl->definitions; }
void GpuParticleManager::SetProgramMaterial(u32 program,u32 material,u32 blend,u32 variant) {
    std::lock_guard lock(m_impl->mutex);
    R_ASSERT2(program < m_impl->programs.size() && blend < 6 && variant < 3,"GPU PAPI invalid material program");
    auto& value = m_impl->programs[program];
    if (value.materialID == material && value.blendMode == blend && value.shaderVariant == variant) return;
    value.materialID = material;
    value.blendMode = blend;
    value.shaderVariant = variant;
    m_impl->definitionsDirty = true;
}
void GpuParticleManager::SetupSimulationPasses(framegraph::FrameGraph& graph,nvrhi::IDevice* device) {
    std::lock_guard lock(m_impl->mutex);
    if (m_impl->roots.empty()) return;
    m_impl->Ensure(device);
    xr_vector<GpuPapiCommand> batch;
    xr_vector<std::array<u32,2>> ranges;
    xr_vector<u32> visible;
    size_t commandCount = 0,pendingRoots = 0,visibleCount = 0;
    for (const auto& root : m_impl->roots) {
        R_ASSERT2(root.commands.size() <= std::numeric_limits<u32>::max() - commandCount,"GPU PAPI command table overflow");
        commandCount += root.commands.size();
        pendingRoots += !root.commands.empty();
        visibleCount += root.live;
    }
    batch.reserve(commandCount);
    ranges.reserve(pendingRoots);
    visible.reserve(visibleCount);
    u32 collisionFlags = 0;
    for (u32 i = 0; i < m_impl->roots.size(); ++i) {
        const auto& root = m_impl->roots[i];
        if (root.live) visible.push_back(i * 2);
        if (root.commands.empty()) continue;
        ranges.push_back({u32(batch.size()),u32(root.commands.size())});
        batch.insert(batch.end(),root.commands.begin(),root.commands.end());
        for (const auto& command : root.commands)
            if (command.type == 6) { collisionFlags |= root.collisionFlags; break; }
    }
    m_impl->Buffer(m_impl->commands,batch.size(),sizeof(GpuPapiCommand),"GpuPapiCommands",false);
    m_impl->Buffer(m_impl->rootCommands,ranges.size(),sizeof(ranges[0]),"GpuPapiRootCommands",false);
    m_impl->Buffer(m_impl->visibleRoots,visible.size(),sizeof(u32),"GpuPapiVisibleRoots",false);
    auto pass = graph.AddPass("GpuPapiSimulation");
    graph.SetPassHasSideEffects(pass);
    auto import = [&](const char* name,nvrhi::IBuffer* buffer) {
        framegraph::ResourceDesc desc;
        desc.type = framegraph::ResourceDesc::Type::Buffer;
        desc.bufferSize = buffer->getDesc().byteSize;
        desc.isUAV = buffer->getDesc().canHaveUAVs;
        desc.isTransient = false;
        desc.debugName = name;
        auto resource = graph.ImportBuffer(name,buffer,desc);
        graph.PassReadWrite(pass,resource,desc.isUAV ? framegraph::ResourceState::UnorderedAccess : framegraph::ResourceState::ShaderResource);
        return resource;
    };
    auto& draw = m_impl->draw;
    draw.particleResource = import("gpu_papi_particles",draw.particles);
    draw.emitterResource = import("gpu_papi_emitters",draw.emitters);
    draw.programResource = import("gpu_papi_programs",draw.programs);
    draw.indexResource = import("gpu_papi_indices",draw.indices);
    draw.argsResource = import("gpu_papi_args",draw.drawArgs);
    draw.bucketResource = import("gpu_papi_buckets",draw.bucketOffsets);
    import("gpu_papi_actions",m_impl->actionBuffer);
    import("gpu_papi_action_state",m_impl->actionState);
    import("gpu_papi_particle_allocation",m_impl->particleAllocation);
    import("gpu_papi_state_allocation",m_impl->stateAllocation);
    import("gpu_papi_commands",m_impl->commands);
    import("gpu_papi_root_commands",m_impl->rootCommands);
    import("gpu_papi_visible_roots",m_impl->visibleRoots);
    import("gpu_papi_status",m_impl->status);
    import("gpu_papi_errors",m_impl->errors);
    import("gpu_papi_noise",m_impl->noise);
    import("gpu_papi_cursors",m_impl->cursors);
    graph.SetPassCallback(pass,[impl = m_impl,batch = std::move(batch),ranges = std::move(ranges),visible = std::move(visible),rootCount = u32(m_impl->roots.size()),collisionFlags](RenderContext& context,const framegraph::FrameGraph&) {
        impl->Execute(context.GetCommandList(),batch,ranges,visible,rootCount,collisionFlags);
    });
}
GpuParticleDrawResources GpuParticleManager::GetDrawResources() const {
    std::lock_guard lock(m_impl->mutex);
    return m_impl->draw;
}
void GpuParticleManager::LevelUnload() {
    std::lock_guard lock(m_impl->mutex);
    for (auto& root : m_impl->roots) if (root.live) {
        root.commands.erase(std::remove_if(root.commands.begin(),root.commands.end(),[](const GpuPapiCommand& command) { return command.type == 6; }),root.commands.end());
        m_impl->Queue(root,3,true).p0 = false;
        root.hasSnapshot = false;
    }
    m_impl->collision.Reset();
}
void GpuParticleManager::Reset() { m_impl = std::make_shared<Impl>(); }
}
