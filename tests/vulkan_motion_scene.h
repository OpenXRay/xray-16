#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>

namespace motion_scene
{
constexpr unsigned Width = 128;
constexpr unsigned Height = 96;
using Point = std::array<float, 3>;
using Pixel = std::array<float, 4>;
using Frame = std::array<Pixel, Width * Height>;
using Matrix = std::array<float, 16>;

inline float dot(const Point& a, const Point& b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

struct Camera
{
    Point eye, right, up, forward;
    float tan_x, tan_y;
    static constexpr float A = 40.f / 39.9f;
    static constexpr float B = -.1f * A;

    Camera(Point position, float yaw, float pitch, float vertical_tangent)
        : eye(position), right{std::cos(yaw), 0.f, -std::sin(yaw)},
          up{std::sin(yaw) * std::sin(pitch), std::cos(pitch), std::cos(yaw) * std::sin(pitch)},
          forward{std::sin(yaw) * std::cos(pitch), -std::sin(pitch), std::cos(yaw) * std::cos(pitch)},
          tan_x(vertical_tangent * Width / Height), tan_y(vertical_tangent)
    {}

    Matrix projection() const
    {
        Matrix result{};
        for (unsigned i = 0; i < 3; ++i)
        {
            result[4 * i] = right[i] / tan_x;
            result[4 * i + 1] = up[i] / tan_y;
            result[4 * i + 2] = A * forward[i];
            result[4 * i + 3] = forward[i];
        }
        result[12] = -dot(right, eye) / tan_x;
        result[13] = -dot(up, eye) / tan_y;
        result[15] = -dot(forward, eye);
        result[14] = A * result[15] + B;
        return result;
    }

    Matrix inverse_projection() const
    {
        Matrix result{};
        for (unsigned i = 0; i < 3; ++i)
        {
            result[i] = right[i] * tan_x;
            result[4 + i] = up[i] * tan_y;
            result[8 + i] = eye[i] / B;
            result[12 + i] = forward[i] - A * eye[i] / B;
        }
        result[11] = 1.f / B;
        result[15] = -A / B;
        return result;
    }

    Point ray(unsigned x, unsigned y) const
    {
        const float dx = (2.f * (x + .5f) / Width - 1.f) * tan_x;
        const float dy = (1.f - 2.f * (y + .5f) / Height) * tan_y;
        return {forward[0] + dx * right[0] + dy * up[0],
                forward[1] + dx * right[1] + dy * up[1],
                forward[2] + dx * right[2] + dy * up[2]};
    }

    Point screen(Point p) const
    {
        for (unsigned i = 0; i < 3; ++i)
            p[i] -= eye[i];
        const float w = dot(forward, p);
        return {Width * (.5f + .5f * dot(right, p) / (tan_x * w)),
                Height * (.5f - .5f * dot(up, p) / (tan_y * w)), A + B / w};
    }

    // The engine projects the light bounds, then rounds outward and clips
    // them to the viewport. All eight corners in this fixture are in front.
    unsigned clipped_light_area(const Point& center, float range) const
    {
        float left = Width, top = Height, right_edge = 0, bottom = 0;
        for (int x : {-1, 1})
            for (int y : {-1, 1})
                for (int z : {-1, 1})
                {
                    const auto p = screen({center[0] + x * range, center[1] + y * range, center[2] + z * range});
                    left = std::min(left, p[0]);
                    top = std::min(top, p[1]);
                    right_edge = std::max(right_edge, p[0]);
                    bottom = std::max(bottom, p[1]);
                }
        const auto w = std::ceil(std::clamp(right_edge, 0.f, float(Width))) -
                       std::floor(std::clamp(left, 0.f, float(Width)));
        const auto h = std::ceil(std::clamp(bottom, 0.f, float(Height))) -
                       std::floor(std::clamp(top, 0.f, float(Height)));
        return static_cast<unsigned>(w * h);
    }
};

inline void write_pair(const std::string& path, const Frame& left, const Frame& right)
{
    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << Width * 2 << " " << Height << "\n255\n";
    for (unsigned y = 0; y < Height; ++y)
        for (const auto* panel : {&left, &right})
            for (unsigned x = 0; x < Width; ++x)
                for (unsigned channel = 0; channel < 3; ++channel)
                    out.put(static_cast<char>(std::lround(255.f * std::clamp((*panel)[y * Width + x][channel], 0.f, 1.f))));
    if (!out)
        throw std::runtime_error("cannot write motion scene frame " + path);
}
} // namespace motion_scene
