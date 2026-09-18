#pragma once

#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/simd_math.h"

namespace AnimationECS {

class IDebugDrawContext
{
public:
    virtual ~IDebugDrawContext() = default;

    virtual void DrawLine(const ozz::math::Float3& start,
                         const ozz::math::Float3& end,
                         const ozz::math::Float4& color) = 0;

    virtual void DrawSphere(const ozz::math::Float3& center,
                           float radius,
                           const ozz::math::Float4& color,
                           int segments = 12) = 0;

    virtual void DrawBoneShape(const ozz::math::Float3& head,
                              const ozz::math::Float3& tail,
                              float radius,
                              const ozz::math::Float4& color) = 0;

    virtual void DrawAxes(const ozz::math::Float4x4& transform,
                         float scale,
                         const ozz::math::Float4& color_x,
                         const ozz::math::Float4& color_y,
                         const ozz::math::Float4& color_z) = 0;

    struct BoneInstance {
        ozz::math::Float3 head;
        ozz::math::Float3 tail;
        float radius;
        ozz::math::Float4 color;
    };

    struct SphereInstance {
        ozz::math::Float3 center;
        float radius;
        ozz::math::Float4 color;
    };

    virtual void DrawBoneShapesInstanced(const BoneInstance* instances, size_t count) = 0;

    virtual void DrawSpheresInstanced(const SphereInstance* instances, size_t count) = 0;
};

}
