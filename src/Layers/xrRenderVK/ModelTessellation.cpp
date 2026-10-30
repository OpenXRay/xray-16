#include "ModelTessellation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace xray::render::vulkan
{
namespace
{
using V3 = std::array<float, 3>;
V3 add(V3 a, V3 b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
V3 mul(V3 a, float s) { return {a[0] * s, a[1] * s, a[2] * s}; }
V3 sub(V3 a, V3 b) { return add(a, mul(b, -1.f)); }
float dot(V3 a, V3 b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
V3 unit(V3 a)
{
    const float length = std::sqrt(dot(a, a));
    return length > 1e-8f ? mul(a, 1.f / length) : V3{0.f, 1.f, 0.f};
}
template <typename V> V3 position(const V& v) { return {v.position[0], v.position[1], v.position[2]}; }
template <typename V> V3 normal(const V& v) { return unit({v.normal[0], v.normal[1], v.normal[2]}); }
V3 edge(V3 a, V3 b, V3 n)
{ return mul(sub(add(mul(a, 2.f), b), mul(n, dot(sub(b, a), n))), 1.f / 3.f); }
V3 edge_normal(V3 a, V3 b, V3 na, V3 nb)
{
    const V3 d = sub(b, a);
    const float length2 = dot(d, d);
    return unit(sub(add(na, nb), mul(d,
        length2 > 1e-8f ? 2.f * dot(d, add(na, nb)) / length2 : 0.f)));
}

template <typename V>
V interpolate(const V (&v)[3], const std::array<float, 3>& w,
    bool pn, const HeightField* height, float height_scale)
{
    V out{};
    const V3 p[3]{position(v[0]), position(v[1]), position(v[2])};
    const V3 n[3]{normal(v[0]), normal(v[1]), normal(v[2])};
    const float a = w[0], b = w[1], c = w[2];
    V3 result = add(add(mul(p[0], a), mul(p[1], b)), mul(p[2], c));
    if (pn)
    {
        const V3 e01 = edge(p[0], p[1], n[0]), e10 = edge(p[1], p[0], n[1]);
        const V3 e12 = edge(p[1], p[2], n[1]), e21 = edge(p[2], p[1], n[2]);
        const V3 e20 = edge(p[2], p[0], n[2]), e02 = edge(p[0], p[2], n[0]);
        const V3 e = mul(add(add(add(e01, e10), add(e12, e21)), add(e20, e02)), 1.f / 6.f);
        const V3 center = add(e, mul(sub(e, mul(add(add(p[0], p[1]), p[2]), 1.f / 3.f)), .5f));
        result = add(add(add(mul(p[0], a*a*a), mul(p[1], b*b*b)), mul(p[2], c*c*c)),
            add(add(add(mul(e01, 3*a*a*b), mul(e10, 3*a*b*b)),
                    add(mul(e12, 3*b*b*c), mul(e21, 3*b*c*c))),
                add(add(mul(e20, 3*c*c*a), mul(e02, 3*c*a*a)), mul(center, 6*a*b*c))));
    }
    V3 normal_value = unit(add(add(mul(n[0], a), mul(n[1], b)), mul(n[2], c)));
    if (pn)
    {
        normal_value = unit(add(add(add(mul(n[0], a*a), mul(n[1], b*b)), mul(n[2], c*c)),
            add(add(mul(edge_normal(p[0], p[1], n[0], n[1]), 2*a*b),
                    mul(edge_normal(p[1], p[2], n[1], n[2]), 2*b*c)),
                mul(edge_normal(p[2], p[0], n[2], n[0]), 2*c*a))));
    }
    for (size_t j = 0; j < 2; ++j)
        out.uv[j] = a*v[0].uv[j] + b*v[1].uv[j] + c*v[2].uv[j];
    if (height)
    {
        result = add(result, mul(normal_value,
            (height->sample(out.uv[0], out.uv[1]) - .5f) * height_scale));
        // Derivatives of the height field tilt the interpolated normal so
        // displacement also affects lighting, including skinned models.
        const float du = .5f / height->width, dv = .5f / height->height;
        const float hu = (height->sample(out.uv[0]+du, out.uv[1]) -
            height->sample(out.uv[0]-du, out.uv[1])) * height_scale / (2*du);
        const float hv = (height->sample(out.uv[0], out.uv[1]+dv) -
            height->sample(out.uv[0], out.uv[1]-dv)) * height_scale / (2*dv);
        const float u1 = v[1].uv[0]-v[0].uv[0], v1 = v[1].uv[1]-v[0].uv[1];
        const float u2 = v[2].uv[0]-v[0].uv[0], v2 = v[2].uv[1]-v[0].uv[1];
        const float uv_det = u1*v2 - v1*u2;
        if (std::abs(uv_det) > 1e-8f)
        {
            const V3 tangent = mul(sub(mul(sub(p[1],p[0]),v2),mul(sub(p[2],p[0]),v1)),1.f/uv_det);
            const V3 bitangent = mul(sub(mul(sub(p[2],p[0]),u1),mul(sub(p[1],p[0]),u2)),1.f/uv_det);
            const float tt = dot(tangent,tangent), bb = dot(bitangent,bitangent);
            const float tb = dot(tangent,bitangent), determinant = tt*bb-tb*tb;
            if (determinant > 1e-8f)
            {
                const V3 gradient = add(mul(tangent,(hu*bb-hv*tb)/determinant),
                    mul(bitangent,(hv*tt-hu*tb)/determinant));
                normal_value = unit(sub(normal_value,gradient));
            }
        }
    }
    for (size_t j = 0; j < 3; ++j)
    { out.position[j] = result[j]; out.normal[j] = normal_value[j]; }
    return out;
}

template <>
LevelVertex interpolate(const LevelVertex (&v)[3], const std::array<float, 3>& w,
    bool pn, const HeightField* height, float height_scale)
{
    // The shared evaluator also works for LevelVertex; interpolate the
    // lightmap and baked lighting separately to preserve their channels.
    struct Base { float position[3], normal[3], uv[2]; };
    const Base input[3]{{{v[0].position[0],v[0].position[1],v[0].position[2]},
        {v[0].normal[0],v[0].normal[1],v[0].normal[2]}, {v[0].uv[0],v[0].uv[1]}},
        {{v[1].position[0],v[1].position[1],v[1].position[2]},
        {v[1].normal[0],v[1].normal[1],v[1].normal[2]}, {v[1].uv[0],v[1].uv[1]}},
        {{v[2].position[0],v[2].position[1],v[2].position[2]},
        {v[2].normal[0],v[2].normal[1],v[2].normal[2]}, {v[2].uv[0],v[2].uv[1]}}};
    const Base base = interpolate(input, w, pn, height, height_scale);
    LevelVertex out{};
    std::copy_n(base.position, 3, out.position);
    std::copy_n(base.normal, 3, out.normal);
    std::copy_n(base.uv, 2, out.uv);
    for (size_t i = 0; i < 2; ++i)
        out.lightmap_uv[i] = w[0]*v[0].lightmap_uv[i] + w[1]*v[1].lightmap_uv[i] + w[2]*v[2].lightmap_uv[i];
    for (size_t i = 0; i < 4; ++i)
        out.baked[i] = w[0]*v[0].baked[i] + w[1]*v[1].baked[i] + w[2]*v[2].baked[i];
    return out;
}

template <>
ModelVertex interpolate(const ModelVertex (&v)[3], const std::array<float, 3>& w,
    bool pn, const HeightField* height, float height_scale)
{
    struct Base { float position[3], normal[3], uv[2]; };
    const Base input[3]{{{v[0].position[0],v[0].position[1],v[0].position[2]},
        {v[0].normal[0],v[0].normal[1],v[0].normal[2]}, {v[0].uv[0],v[0].uv[1]}},
        {{v[1].position[0],v[1].position[1],v[1].position[2]},
        {v[1].normal[0],v[1].normal[1],v[1].normal[2]}, {v[1].uv[0],v[1].uv[1]}},
        {{v[2].position[0],v[2].position[1],v[2].position[2]},
        {v[2].normal[0],v[2].normal[1],v[2].normal[2]}, {v[2].uv[0],v[2].uv[1]}}};
    const Base base = interpolate(input, w, pn, height, height_scale);
    ModelVertex out{};
    std::copy_n(base.position, 3, out.position);
    std::copy_n(base.normal, 3, out.normal);
    std::copy_n(base.uv, 2, out.uv);
    for (unsigned axis=0; axis<3; ++axis)
        for (unsigned corner=0; corner<3; ++corner)
        {
            out.tangent[axis] += w[corner]*v[corner].tangent[axis];
            out.bitangent[axis] += w[corner]*v[corner].bitangent[axis];
        }
    std::array<std::pair<uint16_t, float>, 12> influences{};
    size_t count = 0;
    for (size_t corner = 0; corner < 3; ++corner)
        for (size_t i = 0; i < 4; ++i)
        {
            const float weight = v[corner].weights[i] * w[corner];
            if (weight <= 0) continue;
            size_t match = 0;
            while (match < count && influences[match].first != v[corner].bones[i]) ++match;
            if (match == count) influences[count++] = {v[corner].bones[i], weight};
            else influences[match].second += weight;
        }
    std::sort(influences.begin(), influences.begin() + count,
        [](const auto& left, const auto& right) { return left.second > right.second; });
    float total = 0.f;
    for (size_t i = 0; i < std::min(count, size_t(4)); ++i) total += influences[i].second;
    for (size_t i = 0; i < std::min(count, size_t(4)); ++i)
    { out.bones[i] = influences[i].first; out.weights[i] = influences[i].second / std::max(total, 1e-8f); }
    if (!count) out.weights[0] = 1.f;
    return out;
}

template <typename V, typename G>
bool subdivide(G& geometry, uint8_t mode, const HeightField* height,
    float height_scale, std::string& error)
{
    if (!mode) return true;
    if (mode > 3 || ((mode & 2) && (!height || !height->width || !height->height || height->values.empty())) ||
        !std::isfinite(height_scale) || geometry.indices.size() % 3 != 0 ||
        geometry.indices.size() > 24'000'000 ||
        geometry.indices.size() / 3 > UINT32_MAX / 12)
    { error = "invalid or oversized PN/HM tessellation input"; return false; }
    constexpr std::array<std::array<float, 3>, 6> bary{{
        {{1,0,0}}, {{0,1,0}}, {{0,0,1}}, {{.5f,.5f,0}}, {{0,.5f,.5f}}, {{.5f,0,.5f}}}};
    constexpr uint32_t patch[]{0,3,5, 3,1,4, 5,4,2, 3,4,5};
    std::vector<V> vertices;
    std::vector<uint32_t> indices;
    const size_t triangles = geometry.indices.size() / 3;
    vertices.reserve(triangles * 6);
    indices.reserve(triangles * 12);
    for (size_t triangle = 0; triangle < triangles; ++triangle)
    {
        const auto& original = geometry.indices;
        const uint32_t i0 = original[triangle*3], i1 = original[triangle*3+1], i2 = original[triangle*3+2];
        if (i0 >= geometry.vertices.size() || i1 >= geometry.vertices.size() || i2 >= geometry.vertices.size())
        { error = "PN/HM tessellation triangle exceeds vertex buffer"; return false; }
        const V corners[3]{geometry.vertices[i0], geometry.vertices[i1], geometry.vertices[i2]};
        const uint32_t start = static_cast<uint32_t>(vertices.size());
        for (auto weights : bary)
            vertices.push_back(interpolate(corners, weights, (mode & 1) != 0,
                (mode & 2) ? height : nullptr, height_scale));
        for (auto index : patch) indices.push_back(start + index);
    }
    for (auto& window : geometry.windows)
    {
        if (window.offset % 3 || window.index_count % 3 ||
            window.offset > geometry.indices.size() ||
            window.index_count > geometry.indices.size() - window.offset)
        { error = "PN/HM tessellation has an invalid sliding window"; return false; }
        window.offset *= 4; window.index_count *= 4;
    }
    geometry.vertices = std::move(vertices);
    geometry.indices = std::move(indices);
    error.clear();
    return true;
}
}

float HeightField::sample(float u, float v) const
{
    if (!width || !height || values.size() != size_t(width) * height ||
        !std::isfinite(u) || !std::isfinite(v)) return .5f;
    const auto fract = [](float x) { return x - std::floor(x); };
    const float x = fract(u) * width - .5f, y = fract(v) * height - .5f;
    const int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y));
    const float fx = fract(x), fy = fract(y);
    const auto at = [&](int px, int py) -> float
    {
        const auto wrap = [](int coord, int size) { return (coord % size + size) % size; };
        return values[size_t(wrap(py, height)) * width + wrap(px, width)] / 255.f;
    };
    return (at(ix, iy)*(1-fx) + at(ix+1, iy)*fx)*(1-fy) +
        (at(ix, iy+1)*(1-fx) + at(ix+1, iy+1)*fx)*fy;
}

bool tessellate_model(ModelGeometry& geometry, uint8_t mode,
    const HeightField* height, float height_scale, std::string& error)
{ return subdivide<ModelVertex>(geometry, mode, height, height_scale, error); }
bool tessellate_level(LevelModel& geometry, uint8_t mode,
    const HeightField* height, float height_scale, std::string& error)
{ return subdivide<LevelVertex>(geometry, mode, height, height_scale, error); }
}
