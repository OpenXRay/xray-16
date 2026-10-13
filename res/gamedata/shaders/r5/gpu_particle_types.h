#ifndef GPU_PARTICLE_TYPES_H
#define GPU_PARTICLE_TYPES_H
#ifdef __cplusplus
#include "xrCore/_vector2.h"
#include "xrCore/_vector3d.h"
#include "xrCore/_vector4.h"
namespace xray::render::fg {
using float2 = Fvector2;
using float3 = Fvector;
using float4 = Fvector4;
using uint = u32;
#endif
#define GPU_PAPI_INVALID 0xffffffffu
#define GPU_PAPI_PLAYING 1u
#define GPU_PAPI_STOPPING 2u
#define GPU_PAPI_LOCAL 4u
#define GPU_PAPI_HUD 8u
#define GPU_PAPI_ATTACHED 16u
#define GPU_PAPI_REWIND 32u
#define GPU_PAPI_DESTROY 64u
#define GPU_PAPI_PARENT_SET 128u
struct GpuPapiDomain {
    uint type,pad0,pad1,pad2;
    float4 p1,p2,u,v,radii;
};
struct GpuPapiAction {
    uint type,flags,pad0,stateIndex;
    GpuPapiDomain domains[5];
    float4 p0,p1,p2,p3;
};
struct GpuPapiParticle {
    float3 position; float age;
    float3 velocity; uint frame;
    float3 size; uint color;
    float3 rotation; uint flags;
    float3 positionB; uint child;
    uint emitter,program,generation,pad;
};
struct GpuPapiProgram {
    uint flags,materialID,blendMode,shaderVariant;
    float2 frameSize; float frameSpeed; uint frameCount;
    float3 alignRotation; uint frameDimX;
    float3 velocityScale; uint maxParticles;
    uint actionFirst,actionCount,stateCount,pad1;
    float collisionFriction,collisionResilience,collisionCutoff,timeLimit;
};
struct GpuPapiEmitter {
    float4 basisX,basisY,basisZ,origin;
    uint flags,program,generation,particleFirst;
    uint count,capacity,actionStateFirst,rng;
    uint parent,parentParticle,childBirth,childPlay;
    uint childDead,childFlags,root,active;
    float3 parentVelocity; float elapsed;
    float3 boundsMin; uint completedSerial;
    float3 boundsMax; uint commandSerial;
    uint commandFirst,commandCount,memDT,tickMillis;
    float remainingLimit; uint retired,pad0,pad1;
    float4 actionBasisX,actionBasisY,actionBasisZ,actionOrigin;
};
struct GpuPapiCommand {
    uint type,emitter,generation,serial;
    uint p0,p1,p2,p3;
    float4 basisX,basisY,basisZ,origin,velocity;
};
struct GpuPapiStatus {
    uint generation,serial,count,playing;
    float3 boundsMin; uint error;
    float3 boundsMax; uint retired;
};
struct GpuPapiConstants {
    uint particleCapacity,emitterCapacity,actionStateCapacity,rootCapacity;
    uint commandCount,programCount,phase,pad;
};
#ifdef __cplusplus
static_assert(sizeof(GpuPapiDomain) == 96);
static_assert(sizeof(GpuPapiAction) == 560);
static_assert(sizeof(GpuPapiParticle) == 96);
static_assert(sizeof(GpuPapiProgram) == 96);
static_assert(sizeof(GpuPapiEmitter) == 272);
static_assert(sizeof(GpuPapiCommand) == 112);
static_assert(sizeof(GpuPapiStatus) == 48);
}
#endif
#endif
