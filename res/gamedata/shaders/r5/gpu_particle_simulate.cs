#include "gpu_particle_runtime.h"
uint GpuPrepareCommand(inout GpuPapiEmitter emitter,GpuPapiCommand command,uint index) {
    if (command.emitter != index) { GpuAllocationError(8u); return 2; }
    if (command.generation < emitter.generation) return 0;
    if (command.generation == emitter.generation && command.serial <= emitter.completedSerial) return 0;
    if (command.type == 0) {
        if (emitter.active != 0 || command.p0 >= g_Papi.programCount) { GpuAllocationError(8u); return 2; }
        GpuPapiProgram program = g_Programs[command.p0];
        if (!GpuReserveResources(program.maxParticles,program.actionCount,0)) return 2;
        GpuInitializeEmitter(emitter,command.p0,index,command.generation,index,g_Reservation.particleFirst,g_Reservation.stateFirst);
        g_Reservation.particleUsed = program.maxParticles;
        g_Reservation.stateUsed = program.actionCount;
        GpuReleaseReservation();
    }
    if (emitter.generation != command.generation || emitter.active != 1) return 0;
    if (command.type == 1) { GpuStopRoot(emitter,false); GpuReleaseEmitter(emitter); }
    else if (command.type == 2) GpuPlayEmitter(emitter);
    else if (command.type == 3) GpuStopRoot(emitter,command.p0 != 0);
    else if (command.type == 4) {
        emitter.basisX = command.basisX;
        emitter.basisY = command.basisY;
        emitter.basisZ = command.basisZ;
        emitter.origin = command.origin;
        emitter.flags = (emitter.flags & ~GPU_PAPI_LOCAL) | (command.p0 != 0 ? GPU_PAPI_LOCAL : 0u);
        if (command.p0 == 0) {
            emitter.actionBasisX = command.basisX;
            emitter.actionBasisY = command.basisY;
            emitter.actionBasisZ = command.basisZ;
            emitter.actionOrigin = command.origin;
            emitter.parentVelocity = command.velocity.xyz;
            emitter.flags |= GPU_PAPI_PARENT_SET;
        }
    }
    else if (command.type == 5) {
        uint hud = command.p0 != 0 ? GPU_PAPI_HUD : 0u;
        emitter.flags = (emitter.flags & ~GPU_PAPI_HUD) | hud;
        uint child = emitter.pad0;
        while (child != GPU_PAPI_INVALID) {
            g_Emitters[child].flags = (g_Emitters[child].flags & ~GPU_PAPI_HUD) | hud;
            child = g_Emitters[child].pad0;
        }
    }
    else if (command.type == 6) {
        if (!GpuReserveTick(emitter,command.p0)) return 2;
        return 3;
    }
    else if (command.type == 7) {
        emitter.childBirth = command.p0;
        emitter.childPlay = command.p1;
        emitter.childDead = command.p2;
        emitter.childFlags = command.p3;
    }
    return 1;
}
[numthreads(64,1,1)]
void main(uint3 groupID : SV_GroupID,uint lane : SV_GroupIndex) {
    uint workID = groupID.x;
    if (workID >= g_Papi.rootCapacity) return;
    uint2 range = g_RootCommands[workID];
    if (range.y == 0) return;
    uint index = g_Commands[range.x].emitter;
    uint rootID = index / 2;
    GpuPapiEmitter emitter = (GpuPapiEmitter)0;
    if (lane == 0) emitter = g_Emitters[index];
    GpuSyncEmitter(emitter,lane);
    uint lastCommand = range.x + range.y - 1;
    uint lastGeneration = g_Commands[lastCommand].generation;
    if (lastGeneration < emitter.generation ||
        (lastGeneration == emitter.generation && g_Commands[lastCommand].serial <= emitter.completedSerial)) return;
    bool changed = false,boundsDirty = false;
    for (uint offset = 0; offset < range.y; ++offset) {
        GpuPapiCommand command = g_Commands[range.x + offset];
        if (lane == 0) g_GroupControl = GpuPrepareCommand(emitter,command,index);
        GpuSyncEmitter(emitter,lane);
        uint control = g_GroupControl;
        GroupMemoryBarrierWithGroupSync();
        if (control == 2) break;
        if (control == 0) continue;
        if (control == 3) {
            GpuAdvanceRoot(emitter,command.p0,lane);
            if (lane == 0) GpuReleaseReservation();
        }
        changed = true;
        boundsDirty = boundsDirty || command.type == 0 || command.type == 1 || command.type == 3 || command.type == 4 || command.type == 6;
        if (lane == 0) emitter.completedSerial = command.serial;
        GpuSyncEmitter(emitter,lane);
    }
    if (!changed) return;
    GpuPapiStatus status = (GpuPapiStatus)0;
    status.generation = emitter.generation;
    status.serial = emitter.completedSerial;
    status.retired = emitter.retired;
    status.boundsMin = boundsDirty ? float3(3.402823466e+38,3.402823466e+38,3.402823466e+38) : emitter.boundsMin;
    status.boundsMax = boundsDirty ? -status.boundsMin : emitter.boundsMax;
    if (emitter.active == 1) {
        GpuBounds(emitter,status,boundsDirty,lane);
        uint child = emitter.pad0;
        while (child != GPU_PAPI_INVALID) {
            GpuPapiEmitter childEmitter = (GpuPapiEmitter)0;
            if (lane == 0) childEmitter = g_Emitters[child];
            GpuSyncEmitter(childEmitter,lane);
            GpuBounds(childEmitter,status,boundsDirty,lane);
            child = childEmitter.pad0;
        }
        if (status.count == 0) {
            status.boundsMin = emitter.origin.xyz - 0.001;
            status.boundsMax = emitter.origin.xyz + 0.001;
        }
    }
    if (lane == 0) {
        emitter.boundsMin = status.boundsMin;
        emitter.boundsMax = status.boundsMax;
        g_Emitters[index] = emitter;
        g_Status[rootID] = status;
    }
}
