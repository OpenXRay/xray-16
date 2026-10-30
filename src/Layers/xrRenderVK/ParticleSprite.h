#pragma once
#include <array>
#include <cmath>
namespace xray::render::vulkan {
struct ParticleSpriteCorner { float right, up, u, v; };
inline ParticleSpriteCorner particle_sprite_corner(unsigned corner, float sx, float sy, float angle)
{
    const float x = corner == 0 || corner == 3 ? -sx : sx;
    const float y = corner < 2 ? -sy : sy;
    const float sine = std::sin(angle), cosine = std::cos(angle);
    // Match FillSprite_fpu: texture V=0 belongs to +CameraTop.
    return {cosine*x-sine*y, sine*x+cosine*y,
        corner == 0 || corner == 3 ? 0.f : 1.f, corner < 2 ? 1.f : 0.f};
}
}
