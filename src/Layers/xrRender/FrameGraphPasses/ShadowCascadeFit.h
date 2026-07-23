#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/_matrix.h"
#include "xrCore/_vector3d.h"

namespace xray::render::fg::passes
{

// Classic-style frustum-fit cascade (fixes pitch: cascades follow view frustum).
// Fits a fixed ortho of mapSize around the camera frustum slice [viewZNear, viewZFar].
void ComputeFrustumFitCascadeMatrices(
    const Fvector& sunDirTowardLight, // direction toward the sun (env.sun_dir style)
    float mapSize,
    float viewZNear,
    float viewZFar,
    u32 cascadeIndex,
    u32 smapResolution,
    Fmatrix& outClipVP,
    Fmatrix& outSampleVP);

} // namespace xray::render::fg::passes
