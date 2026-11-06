#pragma once
#include "LevelModels.h"
#include "VulkanGameLightingMath.h"
#include <cmath>
#include <cstring>
#include <unordered_map>

namespace xray::render::vulkan
{
// Flight in level chunk 6: controller:u32 + Flight:104. R_Light in
// build.lights chunk 1:104, with different field offsets. Decode fixed-width
// fields rather than depending on host struct packing.
inline bool decode_level_lights(LevelBytes bytes, bool hemi, std::vector<VulkanLightSnapshot> &result,
                                std::string &error)
{
    const size_t stride = hemi ? 104 : 108;
    if ((!bytes.data && bytes.size) || bytes.size % stride || bytes.size / stride > 250000)
    {
        error = "invalid level light record table";
        return false;
    }
    const auto integer = [](const uint8_t *p) {
        return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
    };
    const auto real = [&](const uint8_t *p) {
        const uint32_t bits = integer(p);
        float f;
        std::memcpy(&f, &bits, 4);
        return f;
    };
    std::vector<VulkanLightSnapshot> prepared;
    for (size_t at = 0; at < bytes.size; at += stride)
    {
        const auto *p = bytes.data + at + (hemi ? 0 : 4);
        const uint32_t type = hemi ? (uint32_t(p[0]) | uint32_t(p[1]) << 8) : integer(p);
        if (hemi ? type != 1 : type == 3)
            continue; // Weather owns the directional sun.
        if (!hemi && type != 1 && type != 2)
        {
            error = "invalid Flight type";
            return false;
        }
        VulkanLightSnapshot light;
        light.active = true;
        light.shadow = !hemi;
        light.hemi_only = hemi;
        for (unsigned axis = 0; axis < 3; ++axis)
        {
            light.color[axis] = real(p + 4 + axis * 4);
            light.position[axis] = real(p + (hemi ? 16 : 52) + axis * 4);
            if (!std::isfinite(light.color[axis]) || !std::isfinite(light.position[axis]) ||
                std::abs(light.position[axis]) > 1e7f || light.color[axis] < 0)
            {
                error = "non-finite or invalid level light color/position";
                return false;
            }
        }
        light.range = real(p + (hemi ? 40 : 76));
        if (!std::isfinite(light.range) || light.range <= 0 || light.range > 100000)
        {
            error = "invalid level light range";
            return false;
        }
        if (hemi)
        {
            light.attenuation = {real(p + 52), real(p + 56), real(p + 60), real(p + 48)};
            for (float f : light.attenuation)
                if (!std::isfinite(f))
                {
                    error = "invalid build.lights attenuation";
                    return false;
                }
        }
        light.id = (uint64_t(1) << 63) | (uint64_t(hemi) << 62) | uint64_t(at / stride + 1);
        prepared.push_back(light);
    }
    result = std::move(prepared);
    error.clear();
    return true;
}

// Match CLight_DB::add_light in the desktop dynamic renderer. The
// level-chunk lights are the legacy R1 set; they are opt-in for R2/GL.
inline void collect_scene_lights(std::vector<VulkanLightSnapshot>& scene,
    const std::vector<VulkanLightSnapshot>& runtime, const std::vector<VulkanLightSnapshot>& legacy,
    bool allow_legacy)
{
    scene.assign(runtime.begin(), runtime.end());
    if (allow_legacy)
        scene.insert(scene.end(), legacy.begin(), legacy.end());
}

// Static hemi sources can number in the thousands. Query the object's cell,
// then its exact sphere, instead of scanning all sources for every ROS.
class VulkanHemiLightGrid
{
    static constexpr float cell_size = 32.f;
    std::vector<VulkanLightSnapshot> sources_;
    std::unordered_map<uint64_t, std::vector<size_t>> cells_;
    std::vector<size_t> broad_;
    static int cell(float value)
    {
        return int(std::floor(value / cell_size));
    }
    static uint64_t key(int x, int z)
    {
        return uint64_t(uint32_t(x)) << 32 | uint32_t(z);
    }

  public:
    void reset(std::vector<VulkanLightSnapshot> sources = {})
    {
        sources_ = std::move(sources);
        cells_.clear();
        broad_.clear();
        for (size_t i = 0; i < sources_.size(); ++i)
        {
            const auto &s = sources_[i];
            if (!std::isfinite(s.range) || s.range <= 0 || s.range > 100000)
                continue;
            bool valid = true;
            for (float v : s.position)
                valid &= std::isfinite(v) && std::abs(v) <= 1e7f;
            if (!valid)
                continue;
            const int x0 = cell(s.position[0] - s.range), x1 = cell(s.position[0] + s.range);
            const int z0 = cell(s.position[2] - s.range), z1 = cell(s.position[2] + s.range);
            if (int64_t(x1 - x0 + 1) * (z1 - z0 + 1) > 1024)
            {
                broad_.push_back(i);
                continue;
            }
            for (int x = x0; x <= x1; ++x)
                for (int z = z0; z <= z1; ++z)
                    cells_[key(x, z)].push_back(i);
        }
    }
    size_t size() const
    {
        return sources_.size();
    }
    void append(const std::array<float, 3> &position, std::vector<VulkanLightSnapshot> &out) const
    {
        for (float f : position)
            if (!std::isfinite(f) || std::abs(f) > 1e7f)
                return;
        const auto add = [&](size_t index) {
            const auto &s = sources_[index];
            double d = 0;
            for (unsigned axis = 0; axis < 3; ++axis)
            {
                const double delta = position[axis] - s.position[axis];
                d += delta * delta;
            }
            if (d < double(s.range) * s.range)
                out.push_back(s);
        };
        const auto it = cells_.find(key(cell(position[0]), cell(position[2])));
        if (it != cells_.end())
            for (size_t i : it->second)
                add(i);
        for (size_t i : broad_)
            add(i);
    }
};
} // namespace xray::render::vulkan
