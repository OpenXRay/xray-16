#pragma once
#include <array>
#include <cmath>
namespace xray::render::vulkan {
struct ParticleSpriteCorner { float right, up, u, v; };
struct ParticleSpriteBasis { std::array<float,3> right,up; };
inline ParticleSpriteBasis particle_path_basis(std::array<float,3> direction,
    const std::array<float,3>& camera_direction)
{
    const auto cross=[](const auto& a,const auto& b) { return std::array<float,3>{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}; };
    const auto unit=[](auto& v) { const float n=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);if(!std::isfinite(n)||n<1e-6f)return false;for(float& x:v)x/=n;return true; };
    if(!unit(direction)) direction={0,1,0};
    auto right=cross(direction,camera_direction);
    if(!unit(right))
    {
        const std::array<float,3> axis=std::abs(direction[2])<.9f?std::array<float,3>{0,0,1}:std::array<float,3>{1,0,0};
        right=cross(direction,axis);unit(right);
    }
    return {right,direction};
}
// Face alignment uses the direction as a surface normal.
// A path billboard instead uses it as its vertical edge.
inline ParticleSpriteBasis particle_face_basis(std::array<float, 3> direction)
{
    const float length =
        std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
    if (!std::isfinite(length) || length < 1e-6f)
        direction = {0, 1, 0};
    else
        for (float &value : direction)
            value /= length;
    const auto basis = particle_path_basis(direction, std::abs(direction[1]) > .99f ? std::array<float, 3>{0, 0, 1}
                                                                                    : std::array<float, 3>{0, 1, 0});
    const std::array<float, 3> r{-basis.right[0], -basis.right[1], -basis.right[2]};
    return {r,
            {direction[1] * r[2] - direction[2] * r[1], direction[2] * r[0] - direction[0] * r[2],
             direction[0] * r[1] - direction[1] * r[0]}};
}
inline std::array<float, 3> project_particle_to_water(std::array<float, 3> position,
    const std::array<float, 3>& origin, std::array<float, 3> normal)
{
    const float length = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    if (!std::isfinite(length) || length < 1e-6f)
        normal = {0.f, 1.f, 0.f};
    else
        for (float& axis : normal)
            axis /= length;
    if (normal[1] < 0.f)
        for (float& axis : normal)
            axis = -axis;
    const float height = (position[0] - origin[0]) * normal[0] +
                         (position[1] - origin[1]) * normal[1] +
                         (position[2] - origin[2]) * normal[2];
    for (unsigned axis = 0; axis < 3; ++axis)
        position[axis] -= (height - .015f) * normal[axis];
    return position;
}

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
