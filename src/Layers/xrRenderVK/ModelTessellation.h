#pragma once

#include "ModelGeometry.h"

namespace xray::render::vulkan
{
struct HeightField
{
    uint32_t width{}, height{};
    std::vector<uint8_t> values;
    float sample(float u, float v) const;
};

// Subdivide each triangle into four patches. PN uses cubic position control
// points and quadratic normal interpolation; HM samples the bump auxiliary
// height channel at the new vertices. Each sliding window retains its range.
bool tessellate_model(ModelGeometry& geometry, uint8_t mode,
    const HeightField* height, float height_scale, std::string& error);
bool tessellate_level(LevelModel& geometry, uint8_t mode,
    const HeightField* height, float height_scale, std::string& error);
}
