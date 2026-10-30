#pragma once
#include "LevelModels.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace xray::render::vulkan
{
using LevelCullSphere = std::array<float, 4>;

inline std::vector<LevelCullSphere> level_hierarchy_bounds(const std::vector<LevelVisual>& visuals)
{
    std::vector<LevelCullSphere> result(visuals.size(), {0, 0, 0, -1});
    std::vector<uint8_t> state(visuals.size());
    const auto valid = [](const LevelCullSphere& s)
    { return s[3] > 0 && std::all_of(s.begin(), s.end(), [](float f) { return std::isfinite(f); }); };
    const auto visit = [&](const auto& self, size_t i, unsigned depth) -> bool
    {
        if (i >= visuals.size() || state[i] == 1 || depth > 512) return false;
        if (state[i] == 2) return valid(result[i]);
        state[i] = 1;
        const auto& node = visuals[i];
        LevelCullSphere sphere{node.bounds[6], node.bounds[7], node.bounds[8], node.bounds[9]};
        bool have = false, okay = true;
        // A hierarchy's exported bounds may miss a child. Rebuild from the
        // children, retaining the original bounds only for its own geometry.
        if (node.children.empty() || node.mesh >= 0 || node.type == 6)
        {
            have = valid(sphere);
            okay = have;
        }
        for (uint32_t child : node.children)
        {
            if (!self(self, child, depth + 1)) { okay = false; continue; }
            const auto& other = result[child];
            if (!have) { sphere = other; have = true; continue; }
            float delta[3]{other[0]-sphere[0], other[1]-sphere[1], other[2]-sphere[2]};
            const float distance = std::sqrt(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]);
            if (distance + other[3] <= sphere[3]) continue;
            if (distance + sphere[3] <= other[3]) { sphere = other; continue; }
            const float radius = (distance + sphere[3] + other[3]) * .5f;
            if (distance > 0)
                for (unsigned axis=0; axis<3; ++axis)
                    sphere[axis] += delta[axis] * ((radius-sphere[3])/distance);
            sphere[3] = radius;
        }
        if (okay && have && valid(sphere))
        {
            sphere[3] += std::max(.001f, sphere[3]*1e-5f); // roundoff guard
            result[i] = sphere;
        }
        state[i] = 2;
        return valid(result[i]);
    };
    for (size_t i=0; i<visuals.size(); ++i) visit(visit, i, 0);
    return result;
}
}
