#pragma once
#include <cmath>
#include <cstdint>
namespace xray::render::vulkan {
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
