#pragma once

#include <gli/texture.hpp>

namespace xray::render
{
inline gli::texture texture_mip_view(const gli::texture& texture, size_t firstLevel)
{
    // DDS loading supplies no custom swizzle. The view constructor applies
    // the format swizzle itself; passing texture.swizzles() applies it twice.
    return gli::texture(texture, texture.target(), texture.format(),
        texture.base_layer(), texture.max_layer(),
        texture.base_face(), texture.max_face(),
        texture.base_level() + firstLevel, texture.max_level());
}
}
