#include "stdafx.h"
#include "Frustum.h"
#include "Intersect.hpp"
#include "ModelTree.h"
#include "xrCDB.h"

#ifdef XRAY_CDB_TEST_HOOKS
#    include "tests/Instrumentation.h"
#endif

#pragma push_macro("FLT_MAX")
#undef FLT_MAX
#define FLT_MAX (std::numeric_limits<float>::max())
#include <Jolt/Geometry/RayAABox.h>
#pragma pop_macro("FLT_MAX")

namespace CDB
{
static JPH::Vec3 ToJolt(const Fvector& vector)
{
    return JPH::Vec3(vector.x, vector.y, vector.z);
}

static bool IsFinite(const Fvector& vector)
{
    return std::isfinite(vector.x) && std::isfinite(vector.y) && std::isfinite(vector.z);
}

static RESULT TriangleResult(const MODEL& model, u32 id)
{
    const auto& triangle = model.get_tris()[id];
    const auto* vertices = model.get_verts();
    RESULT result{};
    result.id = id;
    result.dummy = triangle.dummy;
    for (u32 i = 0; i < 3; i++)
        result.verts[i] = vertices[triangle.verts[i]];
    return result;
}

void COLLIDER::ray_query(u32 ray_mode, const MODEL* model, const Fvector& start, const Fvector& direction, float range)
{
    ZoneScoped;
    model->syncronize();
    r_clear();
    if (!model->tree || !IsFinite(start) || !IsFinite(direction) || !std::isfinite(range) || range <= 0.f)
        return;

    const auto origin = ToJolt(start);
    const JPH::RayInvDirection inverse(ToJolt(direction));
    const bool nearest = (ray_mode & OPT_ONLYNEAREST) != 0;
    const bool first = (ray_mode & OPT_ONLYFIRST) != 0;
    const bool cull = (ray_mode & OPT_CULL) != 0;
    model->tree->Query(
        [&](const JPH::AABox& bounds, u32&)
        {
            return JPH::RayAABoxHits(origin, inverse, bounds.mMin, bounds.mMax, range);
        },
        [&](u32 id)
        {
            auto result = TriangleResult(*model, id);
            if (!TestRayTri(start, direction, result.verts, result.u, result.v, result.range, cull) || result.range <= 0.f || result.range > range)
            {
                return false;
            }

            if (nearest && r_count())
            {
                if (result.range < r_begin()->range)
                    *r_begin() = result;
            }
            else
                r_add() = result;

            if (nearest)
                range = result.range;
            return first;
        });
}

static bool TriangleOverlapsBox(const RESULT& triangle, JPH::Vec3Arg center, JPH::Vec3Arg extents, bool fullTest)
{
    const JPH::Vec3 vertices[3] = {
        ToJolt(triangle.verts[0]) - center,
        ToJolt(triangle.verts[1]) - center,
        ToJolt(triangle.verts[2]) - center,
    };
    const auto minimum = JPH::Vec3::sMin(vertices[0], JPH::Vec3::sMin(vertices[1], vertices[2]));
    const auto maximum = JPH::Vec3::sMax(vertices[0], JPH::Vec3::sMax(vertices[1], vertices[2]));
    if (JPH::Vec3::sGreater(minimum, extents).TestAnyXYZTrue() || JPH::Vec3::sLess(maximum, -extents).TestAnyXYZTrue())
    {
        return false;
    }

    const JPH::Vec3 edges[3] = {
        vertices[1] - vertices[0],
        vertices[2] - vertices[1],
        vertices[0] - vertices[2],
    };
    const auto normal = edges[0].Cross(edges[1]);
    if (std::abs(normal.Dot(vertices[0])) > extents.Dot(normal.Abs()))
        return false;

    if (fullTest)
    {
        const JPH::Vec3 axes[3] = { JPH::Vec3::sAxisX(), JPH::Vec3::sAxisY(), JPH::Vec3::sAxisZ() };
        for (const auto& edge : edges)
        {
            for (const auto& axis : axes)
            {
                const auto separatingAxis = edge.Cross(axis);
                const float radius = extents.Dot(separatingAxis.Abs());
                const float p0 = vertices[0].Dot(separatingAxis);
                const float p1 = vertices[1].Dot(separatingAxis);
                const float p2 = vertices[2].Dot(separatingAxis);
                if (std::min({ p0, p1, p2 }) > radius || std::max({ p0, p1, p2 }) < -radius)
                    return false;
            }
        }
    }
    return true;
}

void COLLIDER::box_query(u32 box_mode, const MODEL* model, const Fvector& center, const Fvector& extents)
{
    ZoneScoped;
    model->syncronize();
    r_clear();
    if (!model->tree || !IsFinite(center) || !IsFinite(extents) || extents.x < 0.f || extents.y < 0.f || extents.z < 0.f)
        return;

    const auto boxCenter = ToJolt(center);
    const auto boxExtents = ToJolt(extents);
    const JPH::AABox box(boxCenter - boxExtents, boxCenter + boxExtents);
    model->tree->Query(
        [&](const JPH::AABox& bounds, u32&)
        {
            return box.Overlaps(bounds);
        },
        [&](u32 id)
        {
            const auto result = TriangleResult(*model, id);
            if (!TriangleOverlapsBox(result, boxCenter, boxExtents, (box_mode & OPT_FULL_TEST) != 0))
                return false;
            r_add() = result;
            return (box_mode & OPT_ONLYFIRST) != 0;
        });
}

void COLLIDER::frustum_query(u32 frustum_mode, const MODEL* model, const CFrustum& frustum)
{
    ZoneScoped;
    model->syncronize();
    r_clear();
    if (!model->tree)
        return;

    model->tree->Query(
        [&](const JPH::AABox& bounds, u32& mask)
        {
#ifdef XRAY_CDB_TEST_HOOKS
            NotifyTestObserver(TestEvent::FrustumBounds, model);
#endif
            const float minimumMaximum[6] = {
                bounds.mMin.GetX(),
                bounds.mMin.GetY(),
                bounds.mMin.GetZ(),
                bounds.mMax.GetX(),
                bounds.mMax.GetY(),
                bounds.mMax.GetZ(),
            };
            return frustum.testAABB(minimumMaximum, mask) != fcvNone;
        },
        [&](u32 id)
        {
            auto result = TriangleResult(*model, id);
            if (frustum_mode & OPT_FULL_TEST)
            {
#ifdef XRAY_CDB_TEST_HOOKS
                NotifyTestObserver(TestEvent::FrustumTriangle, model);
#endif
                sPoly source(result.verts, 3);
                sPoly destination;
                if (!frustum.ClipPoly(source, destination))
                    return false;
            }
            r_add() = result;
            return (frustum_mode & OPT_ONLYFIRST) != 0;
        },
        frustum.getMask());
}
}
