#ifndef GPU_PARTICLE_RUNTIME_H
#define GPU_PARTICLE_RUNTIME_H
#include "gpu_particle_types.h"
cbuffer GpuPapiParams { GpuPapiConstants g_Papi; }
globallycoherent RWStructuredBuffer<GpuPapiParticle> g_Particles;
globallycoherent RWStructuredBuffer<GpuPapiEmitter> g_Emitters;
globallycoherent RWStructuredBuffer<float4> g_ActionState;
StructuredBuffer<GpuPapiProgram> g_Programs;
StructuredBuffer<GpuPapiAction> g_Actions;
StructuredBuffer<GpuPapiCommand> g_Commands;
StructuredBuffer<uint2> g_RootCommands;
StructuredBuffer<float4> g_PapiNoise;
globallycoherent RWStructuredBuffer<uint> g_ParticleAllocation;
globallycoherent RWStructuredBuffer<uint> g_StateAllocation;
RWStructuredBuffer<uint> g_Errors;
RWStructuredBuffer<GpuPapiStatus> g_Status;
float4 GpuPapiInitialActionState(uint actionDataIndex);
uint GpuRuntimeRandom(inout uint state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}
uint GpuParticleIndex(GpuPapiEmitter emitter,uint ordinal) { return emitter.particleFirst + ordinal; }
void GpuAllocationError(uint bit) { InterlockedOr(g_Errors[0],bit); }
struct GpuPapiReservation {
    uint particleFirst,particleCount,particleUsed;
    uint stateFirst,stateCount,stateUsed;
    uint childFirst;
};
static GpuPapiReservation g_Reservation;
groupshared GpuPapiEmitter g_GroupEmitter;
groupshared float4 g_GroupActionState;
groupshared float g_GroupKillOldTime;
groupshared uint g_GroupControl;
groupshared uint g_GroupDead[64];
groupshared float3 g_GroupBoundsMin[64];
groupshared float3 g_GroupBoundsMax[64];
void GpuSyncEmitter(inout GpuPapiEmitter emitter,uint lane) {
    if (lane == 0) g_GroupEmitter = emitter;
    AllMemoryBarrierWithGroupSync();
    emitter = g_GroupEmitter;
    GroupMemoryBarrierWithGroupSync();
}
bool GpuCheckedAdd(inout uint total,uint count) {
    if (count > 0x40000000u - total) { GpuAllocationError(16u); return false; }
    total += count;
    return true;
}
bool GpuCheckedProduct(uint count,uint stride,out uint product) {
    product = 0;
    if (stride != 0 && count > 0x40000000u / stride) { GpuAllocationError(16u); return false; }
    product = count * stride;
    return true;
}
uint GpuAllocateRange(uint count,bool stateRange) {
    if (count == 0) return 0;
    uint limit = stateRange ? g_Papi.actionStateCapacity : g_Papi.particleCapacity;
    if (count > limit) { GpuAllocationError(stateRange ? 4u : 1u); return GPU_PAPI_INVALID; }
    for (uint first = 0; first <= limit - count;) {
        uint acquired = 0;
        uint occupiedEnd = first + 1;
        for (; acquired < count; ++acquired) {
            uint old;
            if (stateRange) InterlockedCompareExchange(g_StateAllocation[first + acquired],0,first + count,old);
            else InterlockedCompareExchange(g_ParticleAllocation[first + acquired],0,first + count,old);
            if (old != 0) { occupiedEnd = old; break; }
        }
        if (acquired == count) return first;
        for (uint j = 0; j < acquired; ++j) {
            if (stateRange) g_StateAllocation[first + j] = 0;
            else g_ParticleAllocation[first + j] = 0;
        }
        first = max(first + acquired + 1,occupiedEnd);
    }
    GpuAllocationError(stateRange ? 4u : 1u);
    return GPU_PAPI_INVALID;
}
void GpuReleaseReservation() {
    DeviceMemoryBarrier();
    if (g_Reservation.particleFirst != GPU_PAPI_INVALID)
        for (uint i = g_Reservation.particleUsed; i < g_Reservation.particleCount; ++i)
            g_ParticleAllocation[g_Reservation.particleFirst + i] = 0;
    if (g_Reservation.stateFirst != GPU_PAPI_INVALID)
        for (uint j = g_Reservation.stateUsed; j < g_Reservation.stateCount; ++j)
            g_StateAllocation[g_Reservation.stateFirst + j] = 0;
    uint child = g_Reservation.childFirst;
    while (child != GPU_PAPI_INVALID) {
        uint next = g_Emitters[child].pad1;
        InterlockedExchange(g_Emitters[child].active,0);
        child = next;
    }
    g_Reservation.childFirst = GPU_PAPI_INVALID;
}
bool GpuReserveResources(uint particles,uint states,uint children) {
    g_Reservation = (GpuPapiReservation)0;
    g_Reservation.particleFirst = GPU_PAPI_INVALID;
    g_Reservation.stateFirst = GPU_PAPI_INVALID;
    g_Reservation.childFirst = GPU_PAPI_INVALID;
    g_Reservation.particleCount = particles;
    g_Reservation.stateCount = states;
    g_Reservation.particleFirst = GpuAllocateRange(particles,false);
    g_Reservation.stateFirst = GpuAllocateRange(states,true);
    uint acquired = 0;
    for (uint index = 1; acquired < children && index < g_Papi.emitterCapacity; index += 2) {
        uint previous;
        InterlockedCompareExchange(g_Emitters[index].active,0,2,previous);
        if (previous != 0) continue;
        DeviceMemoryBarrier();
        g_Emitters[index].pad1 = g_Reservation.childFirst;
        g_Reservation.childFirst = index;
        ++acquired;
    }
    if (acquired != children) GpuAllocationError(2u);
    if (g_Reservation.particleFirst == GPU_PAPI_INVALID || g_Reservation.stateFirst == GPU_PAPI_INVALID || acquired != children) {
        GpuReleaseReservation();
        return false;
    }
    return true;
}
bool GpuReserveChildProgram(uint program,uint count,inout uint particles,inout uint states,inout uint children) {
    if (program == GPU_PAPI_INVALID || count == 0) return true;
    if (program >= g_Papi.programCount) { GpuAllocationError(8u); return false; }
    GpuPapiProgram definition = g_Programs[program];
    uint particleCount,stateCount;
    if (!GpuCheckedProduct(count,definition.maxParticles,particleCount) ||
        !GpuCheckedProduct(count,definition.stateCount,stateCount)) return false;
    return GpuCheckedAdd(particles,particleCount) && GpuCheckedAdd(states,stateCount) && GpuCheckedAdd(children,count);
}
uint GpuTickSteps(GpuPapiEmitter emitter,uint milliseconds) {
    return min(milliseconds / 33u + (emitter.memDT + milliseconds % 33u) / 33u,3u);
}
bool GpuReserveTick(GpuPapiEmitter emitter,uint milliseconds) {
    uint births = 0,deaths = 0,particles = 0,states = 0,children = 0;
    if (emitter.childBirth == GPU_PAPI_INVALID && emitter.childPlay == GPU_PAPI_INVALID && emitter.childDead == GPU_PAPI_INVALID)
        return GpuReserveResources(0,0,0);
    uint steps = GpuTickSteps(emitter,milliseconds);
    if ((emitter.flags & GPU_PAPI_PLAYING) != 0 && steps != 0) {
        GpuPapiProgram program = g_Programs[emitter.program];
        bool deletion = (program.flags & (65536u | 131072u)) == (65536u | 131072u);
        for (uint i = 0; i < program.actionCount; ++i) {
            uint actionIndex = program.actionFirst + i;
            uint actionType = g_Actions[actionIndex].type;
            deletion = deletion || actionType == 10u || actionType == 19u || actionType == 20u;
            if (actionType == 21u && (emitter.flags & GPU_PAPI_STOPPING) == 0) {
                float rate = g_Actions[actionIndex].p0.y;
                if (!isfinite(rate)) { GpuAllocationError(16u); return false; }
                float maximum = ceil(max(rate,0.0) * 0.033);
                uint count = maximum >= float(emitter.capacity) ? emitter.capacity : uint(maximum);
                if (!GpuCheckedAdd(births,count)) return false;
            }
        }
        uint totalBirths;
        if (!GpuCheckedProduct(births,steps,totalBirths)) return false;
        births = totalBirths;
        if (deletion) {
            deaths = emitter.count;
            if (!GpuCheckedAdd(deaths,births)) return false;
        }
    }
    if (!GpuReserveChildProgram(emitter.childBirth,births,particles,states,children) ||
        !GpuReserveChildProgram(emitter.childPlay,births,particles,states,children) ||
        !GpuReserveChildProgram(emitter.childDead,deaths,particles,states,children)) return false;
    return GpuReserveResources(particles,states,children);
}
void GpuReleaseEmitter(inout GpuPapiEmitter emitter) {
    DeviceMemoryBarrier();
    if (emitter.particleFirst != GPU_PAPI_INVALID)
        for (uint i = 0; i < emitter.capacity; ++i) g_ParticleAllocation[emitter.particleFirst + i] = 0;
    if (emitter.actionStateFirst != GPU_PAPI_INVALID)
        for (uint j = 0; j < g_Programs[emitter.program].stateCount; ++j) g_StateAllocation[emitter.actionStateFirst + j] = 0;
    emitter.count = 0;
    emitter.capacity = 0;
    emitter.particleFirst = GPU_PAPI_INVALID;
    emitter.actionStateFirst = GPU_PAPI_INVALID;
    emitter.active = 0;
    emitter.flags = 0;
    emitter.retired = 1;
}
void GpuPublishRetired(uint index,inout GpuPapiEmitter emitter) {
    emitter.active = 2;
    g_Emitters[index] = emitter;
    DeviceMemoryBarrier();
    InterlockedExchange(g_Emitters[index].active,0);
    emitter.active = 0;
}
void GpuInitializeEmitter(out GpuPapiEmitter emitter,uint program,uint index,uint generation,uint root,uint particleFirst,uint stateFirst) {
    emitter = (GpuPapiEmitter)0;
    emitter.program = program;
    emitter.generation = generation;
    emitter.root = root;
    emitter.parent = GPU_PAPI_INVALID;
    emitter.parentParticle = GPU_PAPI_INVALID;
    emitter.childBirth = GPU_PAPI_INVALID;
    emitter.childPlay = GPU_PAPI_INVALID;
    emitter.childDead = GPU_PAPI_INVALID;
    emitter.pad0 = GPU_PAPI_INVALID;
    emitter.pad1 = GPU_PAPI_INVALID;
    emitter.commandFirst = index;
    emitter.particleFirst = GPU_PAPI_INVALID;
    emitter.actionStateFirst = GPU_PAPI_INVALID;
    emitter.basisX = float4(1,0,0,0);
    emitter.basisY = float4(0,1,0,0);
    emitter.basisZ = float4(0,0,1,0);
    emitter.origin = float4(0,0,0,1);
    emitter.actionBasisX = emitter.basisX;
    emitter.actionBasisY = emitter.basisY;
    emitter.actionBasisZ = emitter.basisZ;
    emitter.actionOrigin = emitter.origin;
    emitter.rng = (index + 1) * 747796405u + generation * 2891336453u;
    if (emitter.rng == 0) emitter.rng = 1;
    GpuPapiProgram definition = g_Programs[program];
    emitter.remainingLimit = definition.timeLimit;
    emitter.particleFirst = particleFirst;
    emitter.capacity = definition.maxParticles;
    emitter.actionStateFirst = stateFirst;
    for (uint i = 0; i < definition.actionCount; ++i) {
        uint actionDataIndex = definition.actionFirst + i;
        uint stateIndex = g_Actions[actionDataIndex].stateIndex;
        if (stateIndex != GPU_PAPI_INVALID)
            g_ActionState[emitter.actionStateFirst + stateIndex] = GpuPapiInitialActionState(actionDataIndex);
    }
    emitter.active = 1;
}
void GpuPlayEmitter(inout GpuPapiEmitter emitter) {
    emitter.flags = (emitter.flags | GPU_PAPI_PLAYING) & ~GPU_PAPI_STOPPING;
    GpuPapiProgram program = g_Programs[emitter.program];
    for (uint i = 0; i < program.actionCount; ++i) {
        uint actionType = g_Actions[program.actionFirst + i].type;
        if (actionType == 5u || actionType == 30u)
            g_ActionState[emitter.actionStateFirst + g_Actions[program.actionFirst + i].stateIndex] = 0;
    }
}
void GpuChildTransform(inout GpuPapiEmitter child,GpuPapiEmitter parent,GpuPapiParticle particle,bool initial) {
    float3 position = particle.position;
    float3 velocity = (particle.position - particle.positionB) / 0.033;
    child.basisX = float4(1,0,0,0);
    child.basisY = float4(0,1,0,0);
    child.basisZ = float4(0,0,1,0);
    if (initial && (parent.flags & GPU_PAPI_LOCAL) != 0) {
        child.basisX = parent.basisX;
        child.basisY = parent.basisY;
        child.basisZ = parent.basisZ;
        position = parent.origin.xyz + parent.basisX.xyz * position.x + parent.basisY.xyz * position.y + parent.basisZ.xyz * position.z;
        velocity = parent.basisX.xyz * velocity.x + parent.basisY.xyz * velocity.y + parent.basisZ.xyz * velocity.z;
    }
    child.origin = float4(position,1);
    child.actionBasisX = child.basisX;
    child.actionBasisY = child.basisY;
    child.actionBasisZ = child.basisZ;
    child.actionOrigin = child.origin;
    child.flags |= GPU_PAPI_PARENT_SET;
    child.parentVelocity = velocity;
}
uint GpuSpawnChild(inout GpuPapiEmitter parent,uint program,GpuPapiParticle particle,bool attached) {
    if (program == GPU_PAPI_INVALID) return GPU_PAPI_INVALID;
    uint index = g_Reservation.childFirst;
    GpuPapiProgram definition = g_Programs[program];
    if (index == GPU_PAPI_INVALID || definition.maxParticles > g_Reservation.particleCount - g_Reservation.particleUsed ||
        definition.stateCount > g_Reservation.stateCount - g_Reservation.stateUsed) {
        GpuAllocationError(8u);
        return GPU_PAPI_INVALID;
    }
    g_Reservation.childFirst = g_Emitters[index].pad1;
    uint particleFirst = g_Reservation.particleFirst + g_Reservation.particleUsed;
    uint stateFirst = g_Reservation.stateFirst + g_Reservation.stateUsed;
    g_Reservation.particleUsed += definition.maxParticles;
    g_Reservation.stateUsed += definition.stateCount;
    for (uint i = 0; i < definition.maxParticles; ++i) g_ParticleAllocation[particleFirst + i] = particleFirst + definition.maxParticles;
    for (uint j = 0; j < definition.stateCount; ++j) g_StateAllocation[stateFirst + j] = stateFirst + definition.stateCount;
    GpuPapiEmitter child;
    uint generation = g_Emitters[index].generation + 1;
    GpuInitializeEmitter(child,program,index,generation,parent.root,particleFirst,stateFirst);
    child.parent = parent.root;
    child.parentParticle = particle.pad;
    child.flags = GPU_PAPI_PLAYING | (parent.flags & GPU_PAPI_HUD);
    if (attached) child.flags |= GPU_PAPI_ATTACHED;
    if (attached && (parent.childFlags & 16u) != 0) child.flags |= GPU_PAPI_REWIND;
    GpuPlayEmitter(child);
    GpuChildTransform(child,parent,particle,true);
    child.pad0 = parent.pad0;
    parent.pad0 = index;
    g_Emitters[index] = child;
    return index;
}
bool GpuAddParticle(inout GpuPapiEmitter emitter,GpuPapiParticle particle) {
    if (emitter.count >= emitter.capacity) return false;
    GpuPapiProgram program = g_Programs[emitter.program];
    particle.emitter = emitter.commandFirst;
    particle.program = emitter.program;
    particle.generation = emitter.generation;
    particle.pad = emitter.particleFirst + emitter.count;
    particle.child = GPU_PAPI_INVALID;
    if ((program.flags & 4096u) != 0 && program.frameCount > 0)
        particle.frame = (GpuRuntimeRandom(emitter.rng) % program.frameCount * 255u) & 65535u;
    if ((program.flags & 10240u) == 10240u && (GpuRuntimeRandom(emitter.rng) & 1u) != 0)
        particle.flags |= 1u;
    GpuSpawnChild(emitter,emitter.childBirth,particle,false);
    particle.child = GpuSpawnChild(emitter,emitter.childPlay,particle,true);
    g_Particles[particle.pad] = particle;
    ++emitter.count;
    return true;
}
void GpuRemoveParticle(inout GpuPapiEmitter emitter,uint ordinal) {
    uint index = GpuParticleIndex(emitter,ordinal);
    GpuPapiParticle particle = g_Particles[index];
    if (particle.child != GPU_PAPI_INVALID) {
        GpuPapiEmitter child = g_Emitters[particle.child];
        child.flags = (child.flags | GPU_PAPI_STOPPING) & ~(GPU_PAPI_ATTACHED | GPU_PAPI_REWIND);
        child.parentParticle = GPU_PAPI_INVALID;
        g_Emitters[particle.child] = child;
    }
    GpuSpawnChild(emitter,emitter.childDead,particle,false);
    --emitter.count;
    if (ordinal != emitter.count) {
        GpuPapiParticle last = g_Particles[GpuParticleIndex(emitter,emitter.count)];
        last.pad = index;
        g_Particles[index] = last;
        if (last.child != GPU_PAPI_INVALID) g_Emitters[last.child].parentParticle = index;
    }
}
#include "gpu_particle_actions.h"
#include "gpu_particle_collision.h"
void GpuAnimateParticle(inout GpuPapiParticle particle,GpuPapiProgram program) {
    float frame = float(particle.frame) / 255.0 + ((particle.flags & 1u) != 0 ? -1.0 : 1.0) * program.frameSpeed * 0.033;
    if (frame > float(program.frameCount)) frame -= float(program.frameCount);
    if (frame < 0) frame += float(program.frameCount);
    particle.frame = uint(int(floor(frame * 255.0))) & 65535u;
}
void GpuAdvanceEmitter(inout GpuPapiEmitter emitter,uint milliseconds,uint lane) {
    if ((emitter.flags & GPU_PAPI_PLAYING) == 0 || emitter.active != 1) return;
    uint steps = GpuTickSteps(emitter,milliseconds);
    if (lane == 0) emitter.memDT = (emitter.memDT + milliseconds % 33u) % 33u;
    GpuSyncEmitter(emitter,lane);
    GpuPapiProgram program = g_Programs[emitter.program];
    for (uint step = 0; step < steps; ++step) {
        if (lane == 0) {
            g_GroupControl = 0;
            if ((program.flags & 16384u) != 0 && (emitter.flags & GPU_PAPI_STOPPING) == 0) {
                emitter.remainingLimit -= 0.033;
                if (emitter.remainingLimit < 0) {
                    emitter.remainingLimit = program.timeLimit;
                    emitter.flags |= GPU_PAPI_STOPPING;
                    g_GroupControl = 1;
                }
            }
        }
        GpuSyncEmitter(emitter,lane);
        uint control = g_GroupControl;
        GroupMemoryBarrierWithGroupSync();
        if (control != 0) break;
        float killOldTime = 1.0;
        for (uint actionIndex = 0; actionIndex < program.actionCount;) {
            uint actionDataIndex = program.actionFirst + actionIndex;
            uint type = g_Actions[actionDataIndex].type;
            if (GpuPapiLocalAction(type)) {
                uint segmentCount = 1;
                while (segmentCount < 16u && actionIndex + segmentCount < program.actionCount) {
                    if (!GpuPapiLocalAction(g_Actions[actionDataIndex + segmentCount].type)) break;
                    ++segmentCount;
                }
                GpuPapiExecuteLocalSegment(emitter,actionDataIndex,segmentCount,0.033,killOldTime,lane);
                actionIndex += segmentCount;
                continue;
            }
            if (type == 5u || type == 18u || type == 30u) {
                if (lane == 0)
                    g_GroupActionState = g_ActionState[emitter.actionStateFirst + g_Actions[actionDataIndex].stateIndex];
                GroupMemoryBarrierWithGroupSync();
            }
            if (GpuPapiSerialAction(type)) {
                if (lane == 0) {
                    GpuPapiExecuteAction(emitter,actionDataIndex,0.033,killOldTime,0,1);
                    g_GroupEmitter = emitter;
                    g_GroupKillOldTime = killOldTime;
                }
                AllMemoryBarrierWithGroupSync();
                emitter = g_GroupEmitter;
                killOldTime = g_GroupKillOldTime;
                GroupMemoryBarrierWithGroupSync();
            }
            else {
                GpuPapiExecuteAction(emitter,actionDataIndex,0.033,killOldTime,lane,64);
                AllMemoryBarrierWithGroupSync();
            }
            ++actionIndex;
        }
        bool animated = (program.flags & 3072u) == 3072u;
        bool collision = (program.flags & 65536u) != 0;
        if (animated && !collision) {
            for (uint ordinal = lane; ordinal < emitter.count; ordinal += 64u) {
                uint index = GpuParticleIndex(emitter,ordinal);
                GpuPapiParticle particle = g_Particles[index];
                GpuAnimateParticle(particle,program);
                g_Particles[index].frame = particle.frame;
            }
            AllMemoryBarrierWithGroupSync();
        }
        uint remaining = collision ? emitter.count : 0;
        while (remaining > 0) {
            uint batch = min(remaining,64u);
            if (lane < batch) {
                uint index = GpuParticleIndex(emitter,remaining - 1u - lane);
                GpuPapiParticle particle = g_Particles[index];
                if (animated) GpuAnimateParticle(particle,program);
                bool dead = false;
                GpuParticleCollision(particle.position,particle.positionB,particle.velocity,0.033,program.flags,program.collisionFriction,program.collisionResilience,program.collisionCutoff,dead);
                g_Particles[index] = particle;
                g_GroupDead[lane] = dead ? 1u : 0u;
            }
            AllMemoryBarrierWithGroupSync();
            if (lane == 0)
                for (uint offset = 0; offset < batch; ++offset)
                    if (g_GroupDead[offset] != 0) GpuRemoveParticle(emitter,remaining - 1u - offset);
            GpuSyncEmitter(emitter,lane);
            remaining -= batch;
        }
        if ((emitter.flags & GPU_PAPI_STOPPING) != 0 && emitter.count == 0) {
            if (lane == 0) emitter.flags &= ~(GPU_PAPI_PLAYING | GPU_PAPI_STOPPING);
            GpuSyncEmitter(emitter,lane);
            break;
        }
    }
}
void GpuAdvanceRoot(inout GpuPapiEmitter root,uint milliseconds,uint lane) {
    GpuAdvanceEmitter(root,milliseconds,lane);
    for (uint phase = 0; phase < 2; ++phase) {
        uint index = root.pad0;
        uint previous = GPU_PAPI_INVALID;
        while (index != GPU_PAPI_INVALID) {
            GpuPapiEmitter child = (GpuPapiEmitter)0;
            if (lane == 0) child = g_Emitters[index];
            GpuSyncEmitter(child,lane);
            uint next = child.pad0;
            bool attached = (child.flags & GPU_PAPI_ATTACHED) != 0;
            if (attached == (phase == 0)) {
                if (lane == 0 && attached && (root.flags & GPU_PAPI_PLAYING) != 0 && child.parentParticle != GPU_PAPI_INVALID)
                    GpuChildTransform(child,root,g_Particles[child.parentParticle],false);
                GpuSyncEmitter(child,lane);
                GpuAdvanceEmitter(child,milliseconds,lane);
                bool retire = (child.flags & GPU_PAPI_PLAYING) == 0 && !attached;
                if (lane == 0) {
                    if ((child.flags & GPU_PAPI_PLAYING) == 0) {
                        if (attached && (child.flags & GPU_PAPI_REWIND) != 0) GpuPlayEmitter(child);
                        else if (retire) {
                            GpuReleaseEmitter(child);
                            if (previous == GPU_PAPI_INVALID) root.pad0 = next;
                            else g_Emitters[previous].pad0 = next;
                            GpuPublishRetired(index,child);
                        }
                    }
                    if (!retire) g_Emitters[index] = child;
                }
                GpuSyncEmitter(root,lane);
                if (retire) {
                    index = next;
                    continue;
                }
            }
            previous = index;
            index = next;
        }
    }
}
void GpuStopRoot(inout GpuPapiEmitter root,bool deferred) {
    if (deferred) root.flags |= GPU_PAPI_STOPPING;
    else { root.flags &= ~(GPU_PAPI_PLAYING | GPU_PAPI_STOPPING); root.count = 0; }
    uint index = root.pad0;
    while (index != GPU_PAPI_INVALID) {
        GpuPapiEmitter child = g_Emitters[index];
        uint next = child.pad0;
        if (deferred) child.flags |= GPU_PAPI_STOPPING;
        else GpuReleaseEmitter(child);
        if (deferred) g_Emitters[index] = child;
        else GpuPublishRetired(index,child);
        index = next;
    }
    if (!deferred) root.pad0 = GPU_PAPI_INVALID;
}
void GpuBounds(GpuPapiEmitter emitter,inout GpuPapiStatus status,bool updateBounds,uint lane) {
    status.playing |= (emitter.flags & GPU_PAPI_PLAYING) != 0 ? 1u : 0u;
    status.count += emitter.count;
    if (!updateBounds || emitter.count == 0) return;
    bool local = (emitter.flags & GPU_PAPI_LOCAL) != 0;
    float radiusScale = local ? max(length(emitter.basisX.xyz),max(length(emitter.basisY.xyz),length(emitter.basisZ.xyz))) : 1.0;
    for (uint i = lane; i < emitter.count; i += 64u) {
        GpuPapiParticle particle = g_Particles[GpuParticleIndex(emitter,i)];
        float3 position = particle.position;
        float radius = max(abs(particle.size.x),max(abs(particle.size.y),abs(particle.size.z))) * radiusScale;
        if (local) {
            position = emitter.origin.xyz + emitter.basisX.xyz * position.x + emitter.basisY.xyz * position.y + emitter.basisZ.xyz * position.z;
        }
        status.boundsMin = min(status.boundsMin,position - radius);
        status.boundsMax = max(status.boundsMax,position + radius);
    }
}
void GpuReduceBounds(inout GpuPapiStatus status,uint lane) {
    g_GroupBoundsMin[lane] = status.boundsMin;
    g_GroupBoundsMax[lane] = status.boundsMax;
    GroupMemoryBarrierWithGroupSync();
    for (uint stride = 32u; stride > 0; stride >>= 1u) {
        if (lane < stride) {
            g_GroupBoundsMin[lane] = min(g_GroupBoundsMin[lane],g_GroupBoundsMin[lane + stride]);
            g_GroupBoundsMax[lane] = max(g_GroupBoundsMax[lane],g_GroupBoundsMax[lane + stride]);
        }
        GroupMemoryBarrierWithGroupSync();
    }
    status.boundsMin = g_GroupBoundsMin[0];
    status.boundsMax = g_GroupBoundsMax[0];
    GroupMemoryBarrierWithGroupSync();
}
#endif
