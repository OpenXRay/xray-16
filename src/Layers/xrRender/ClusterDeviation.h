#pragma once

namespace xray::render::fg
{

float ClusterMeshDeviation(
    const float* positions, size_t positionStride,
    const float* attributes, size_t attributeStride,
    const float* attributeWeights, u32 attributeCount,
    const u32* source, size_t sourceCount,
    const u32* result, size_t resultCount);

}
