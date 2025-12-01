// cull_debug.h - Shared debug visualization definitions
// Used by: object_cull_debug.cs, cull_debug.vs, cull_debug.ps
//

#ifndef CULL_DEBUG_H
#define CULL_DEBUG_H

// Culling result states (written by debug compute shader, read by debug VS/PS)
#define CULL_STATE_VISIBLE           0
#define CULL_STATE_OCCLUDER          1
#define CULL_STATE_CULLED_DISTANCE   2
#define CULL_STATE_CULLED_FRUSTUM    3
#define CULL_STATE_CULLED_OCCLUSION  4
#define CULL_STATE_PARTICLE_VISIBLE  5
#define CULL_STATE_PARTICLE_CULLED   6

static const float3 DEBUG_COLOR_VISIBLE         = float3(0.0, 1.0, 0.0);
static const float3 DEBUG_COLOR_OCCLUDER        = float3(0.0, 0.5, 1.0);
static const float3 DEBUG_COLOR_CULLED_DISTANCE = float3(0.5, 0.0, 0.0);
static const float3 DEBUG_COLOR_CULLED_FRUSTUM  = float3(1.0, 0.0, 0.0);
static const float3 DEBUG_COLOR_CULLED_OCCLUSION= float3(1.0, 1.0, 0.0);
static const float3 DEBUG_COLOR_PARTICLE_VISIBLE= float3(0.0, 1.0, 1.0);
static const float3 DEBUG_COLOR_PARTICLE_CULLED = float3(1.0, 0.0, 1.0);

// Per-object debug data (output from compute shader)
struct CullDebugData
{
    float3 position;     // World-space sphere center
    float radius;        // Sphere radius
    uint cullState;      // One of CULL_STATE_* values
    float objectDepth;   // Normalized depth (for additional info)
    float hiZDepth;      // Hi-Z depth sampled (for additional info)
    uint objectIndex;    // Original object index
};

// Get debug color based on cull state
float3 GetCullDebugColor(uint cullState)
{
    switch (cullState)
    {
        case CULL_STATE_VISIBLE:           return DEBUG_COLOR_VISIBLE;
        case CULL_STATE_OCCLUDER:          return DEBUG_COLOR_OCCLUDER;
        case CULL_STATE_CULLED_DISTANCE:   return DEBUG_COLOR_CULLED_DISTANCE;
        case CULL_STATE_CULLED_FRUSTUM:    return DEBUG_COLOR_CULLED_FRUSTUM;
        case CULL_STATE_CULLED_OCCLUSION:  return DEBUG_COLOR_CULLED_OCCLUSION;
        case CULL_STATE_PARTICLE_VISIBLE:  return DEBUG_COLOR_PARTICLE_VISIBLE;
        case CULL_STATE_PARTICLE_CULLED:   return DEBUG_COLOR_PARTICLE_CULLED;
        default:                           return float3(1.0, 1.0, 1.0);
    }
}

#endif // CULL_DEBUG_H
