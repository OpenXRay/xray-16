#pragma once

namespace xray::render::fg
{

constexpr u32 kClusterAttrDirections = 9;
constexpr u32 kClusterAttrWeighted = kClusterAttrDirections + 2;
constexpr u32 kClusterAttrStride = kClusterAttrWeighted + 2;
constexpr u32 kClusterAttrProtectMask =
    ((1u << (kClusterAttrStride - kClusterAttrDirections)) - 1u) << kClusterAttrDirections;

bool ClusterMeshDeviation(
    const float* positions, size_t positionStride,
    const float* attributes, size_t attributeStride,
    const float* attributeWeights, u32 attributeCount,
    const u32* source, size_t sourceCount,
    const u32* result, size_t resultCount,
    float& outDeviation);

}
