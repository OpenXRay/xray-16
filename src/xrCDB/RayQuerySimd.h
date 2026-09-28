#pragma once

namespace CDB
{
constexpr bool use_simd_ray_query([[maybe_unused]] bool hasSSE) noexcept
{
#if defined(__ARM_NEON) || defined(__aarch64__) || defined(_M_ARM64)
    // sse2neon supplies the same ray/AABB intrinsics on ARM builds.
    return true;
#else
    return hasSSE;
#endif
}
}
