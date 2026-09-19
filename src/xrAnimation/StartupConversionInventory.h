#pragma once

#include <cstddef>

namespace XRay
{
namespace Animation
{
struct StartupConversionStats
{
    std::size_t baked = 0;
    std::size_t skipped = 0;
    std::size_t failed = 0;
    double total_time_seconds = 0.0;
};

void PrebakeLegacyMotionLibraries(StartupConversionStats& out_stats);
}
}
