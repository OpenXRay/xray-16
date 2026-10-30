#pragma once
#include <cmath>
#include <cstdint>
namespace xray::render::vulkan {
inline float sun_shadow_span(uint32_t quality)
{
    // Spend the fixed 1024 texels on nearby receivers on mobile presets.
    // Default and higher keep the original long-range coverage.
    return quality == 0 ? 64.f : quality == 1 ? 96.f : 180.f;
}
inline void stabilize_shadow_projection(float* matrix, uint32_t size)
{
    if(!matrix || !size) return;
    const double scale=double(size)*.5;
    // Snap the projected world origin; camera motion below one shadow texel
    // must not continuously slide the entire raster over static geometry.
    for(unsigned index=12;index<14;++index)
        if(std::isfinite(matrix[index])) matrix[index]=float(std::round(double(matrix[index])*scale)/scale);
}
}
