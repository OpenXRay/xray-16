// smoke_common.h
// Shared types for GPU smoke simulation.
// Used by emit, simulate, and compact compute shaders.

#ifndef SMOKE_COMMON_H
#define SMOKE_COMMON_H

// ─────────────────────────────────────────────────────────
//  GPU SIMULATION POINT (must match SmokeSimPoint in C++)
//  48 bytes
// ─────────────────────────────────────────────────────────
struct SmokeSimPoint
{
    float px, py, pz;
    float lifetime;
    float vx, vy, vz;
    float age;
    float width;
    float seedF;
    float pad0;
    float pad1;
};

// ─────────────────────────────────────────────────────────
//  GPU TRAIL CONTROL POINT (must match GPUTrailControlPoint in C++)
//  32 bytes — output of compact CS, input to trail.vs at t10
//  Direction is PRE-SCALED to half-width (Stride parity).
// ─────────────────────────────────────────────────────────
struct TrailControlPoint
{
    float posX, posY, posZ;
    float dirX, dirY, dirZ;   // pre-scaled width direction (world-space, magnitude = halfWidth)
    float ageNorm;             // 0 = youngest (head), 1 = oldest (tail)
    float cumDist;             // cumulative distance from head
};

// ─────────────────────────────────────────────────────────
//  NOISE (4D Perlin + fBm + turbulence displacement)
// ─────────────────────────────────────────────────────────
#include "noise4d.h"

#endif // SMOKE_COMMON_H
