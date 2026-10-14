#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace xray::render::vulkan
{
struct TransparentDrawRef
{
    float distance{};
    bool model{};
    size_t index{};
    bool particle{};
    bool detail{};
    uint8_t priority{1};
};

// A single back-to-front order covers world geometry, OGF, particles and
// grass. Equal-distance draws retain submission order across these sources.
inline void sort_transparent_draws(std::vector<TransparentDrawRef>& draws)
{
    std::stable_sort(draws.begin(), draws.end(), [](const auto& a, const auto& b)
    { return a.priority == b.priority ? a.distance > b.distance : a.priority < b.priority; });
}
}
