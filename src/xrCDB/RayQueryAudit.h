#pragma once

#include <cmath>

namespace CDB
{
// Independent double-precision reference: no acceleration structure or SIMD.
struct RayQueryAudit
{
    struct Vec
    {
        double x, y, z;
        Vec operator-(const Vec& b) const { return {x - b.x, y - b.y, z - b.z}; }
        Vec cross(const Vec& b) const { return {y*b.z-z*b.y, z*b.x-x*b.z, x*b.y-y*b.x}; }
        double dot(const Vec& b) const { return x*b.x + y*b.y + z*b.z; }
    };

    Vec origin{}, direction{};
    int nearest = -1;
    double distance = 500.0;

    void test(unsigned id, const Vec& a, const Vec& b, const Vec& c)
    {
        const Vec e1 = b - a, e2 = c - a;
        const Vec p = direction.cross(e2);
        const double det = e1.dot(p);
        if (std::abs(det) < 0.00001)
            return;
        const Vec t = origin - a;
        const double u = t.dot(p) / det;
        if (u < 0 || u > 1)
            return;
        const Vec q = t.cross(e1);
        const double v = direction.dot(q) / det;
        if (v < 0 || u + v > 1)
            return;
        const double range = e2.dot(q) / det;
        if (range > 0 && range <= distance)
        {
            nearest = static_cast<int>(id);
            distance = range;
        }
    }
};
}
