#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

namespace xray::render::vulkan
{
class VulkanCameraState
{
public:
    using Matrix = std::array<std::array<float, 4>, 4>;

    void reset() noexcept
    {
        for (auto& row : view_projection_)
            row.fill(0.f);
        camera_position_.fill(0.f);
        has_view_projection_ = false;
        has_camera_position_ = false;
    }

    void on_camera_updated(const float (&view_projection)[4][4], float x, float y, float z) noexcept
    {
        for (size_t row = 0; row < 4; ++row)
            std::copy_n(view_projection[row], 4, view_projection_[row].begin());
        camera_position_ = {x, y, z};
        has_view_projection_ = true;
        has_camera_position_ = true;
    }

    void set_cache_xform(const float (&view)[4][4], const float (&project)[4][4]) noexcept
    {
        // Match Fmatrix::mul(project, view), whose row-major storage is used
        // by the engine's row-vector transform convention.
        Matrix combined{};
        for (size_t row = 0; row < 4; ++row)
        {
            for (size_t column = 0; column < 4; ++column)
            {
                for (size_t inner = 0; inner < 4; ++inner)
                    combined[row][column] += project[inner][column] * view[row][inner];
            }
        }
        view_projection_ = combined;
        has_view_projection_ = true;
    }

    bool has_view_projection() const noexcept { return has_view_projection_; }
    bool has_camera_position() const noexcept { return has_camera_position_; }
    const Matrix& view_projection() const noexcept { return view_projection_; }
    const std::array<float, 3>& camera_position() const noexcept { return camera_position_; }

    void copy_view_projection(float (&target)[4][4]) const noexcept
    {
        for (size_t row = 0; row < 4; ++row)
            std::copy(view_projection_[row].begin(), view_projection_[row].end(), target[row]);
    }

private:
    Matrix view_projection_{};
    std::array<float, 3> camera_position_{};
    bool has_view_projection_{};
    bool has_camera_position_{};
};
}
