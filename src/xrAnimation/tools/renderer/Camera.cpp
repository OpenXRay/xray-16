#include "stdafx.h"
#include "Camera.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>

namespace xray {
namespace animation {
namespace renderer {

namespace {
    constexpr float kPI = 3.14159265358979323846f;
    constexpr float k2PI = kPI * 2.0f;

    // Camera control constants (from ozz sample)
    constexpr float kAngleFactor = 0.01f;
    constexpr float kDistanceFactor = 0.1f;
    constexpr float kScrollFactor = 0.03f;

    ozz::math::Float4x4 LookAt(const ozz::math::Float3& eye,
                               const ozz::math::Float3& center,
                               const ozz::math::Float3& up) {
        ozz::math::Float3 f = center - eye;
        float f_len = std::sqrt(f.x * f.x + f.y * f.y + f.z * f.z);
        f.x /= f_len;
        f.y /= f_len;
        f.z /= f_len;

        ozz::math::Float3 s;
        s.x = f.y * up.z - f.z * up.y;
        s.y = f.z * up.x - f.x * up.z;
        s.z = f.x * up.y - f.y * up.x;
        float s_len = std::sqrt(s.x * s.x + s.y * s.y + s.z * s.z);
        s.x /= s_len;
        s.y /= s_len;
        s.z /= s_len;

        ozz::math::Float3 u;
        u.x = s.y * f.z - s.z * f.y;
        u.y = s.z * f.x - s.x * f.z;
        u.z = s.x * f.y - s.y * f.x;

        ozz::math::Float4x4 result;
        result.cols[0] = ozz::math::simd_float4::Load(s.x, u.x, -f.x, 0.0f);
        result.cols[1] = ozz::math::simd_float4::Load(s.y, u.y, -f.y, 0.0f);
        result.cols[2] = ozz::math::simd_float4::Load(s.z, u.z, -f.z, 0.0f);
        result.cols[3] = ozz::math::simd_float4::Load(
            -(s.x * eye.x + s.y * eye.y + s.z * eye.z),
            -(u.x * eye.x + u.y * eye.y + u.z * eye.z),
            f.x * eye.x + f.y * eye.y + f.z * eye.z,
            1.0f
        );

        return result;
    }

    ozz::math::Float4x4 Perspective(float fov_radians, float aspect, float near_plane, float far_plane) {
        const float tan_half_fov = std::tan(fov_radians / 2.0f);

        ozz::math::Float4x4 result;

        result.cols[0] = ozz::math::simd_float4::Load(1.0f / (aspect * tan_half_fov), 0.0f, 0.0f, 0.0f);
        result.cols[1] = ozz::math::simd_float4::Load(0.0f, -1.0f / tan_half_fov, 0.0f, 0.0f);
        result.cols[2] = ozz::math::simd_float4::Load(
            0.0f,
            0.0f,
            far_plane / (near_plane - far_plane),
            -1.0f
        );
        result.cols[3] = ozz::math::simd_float4::Load(
            0.0f,
            0.0f,
            -(far_plane * near_plane) / (far_plane - near_plane),
            0.0f
        );

        return result;
    }
}

Camera::Camera()
    : center_(0.0f, 0.5f, 0.0f)  // Default center (slightly above origin)
    , angles_(-kPI / 12.0f, kPI / 5.0f)  // Default pitch and yaw
    , distance_(8.0f)  // Default distance
    , up_vector_(0.0f, 1.0f, 0.0f)  // Y-up
    , fov_radians_(kPI / 3.0f)  // 60 degrees
    , aspect_ratio_(16.0f / 9.0f)
    , near_plane_(0.5f)
    , far_plane_(50.0f)
    , viewport_width_(1600)
    , viewport_height_(900)
    , last_mouse_x_(0.0)
    , last_mouse_y_(0.0)
    , left_button_down_(false)
    , middle_button_down_(false)
    , right_button_down_(false)
    , eye_position_(0.0f, 0.0f, 0.0f)
    , look_at_target_(center_)
    , matrices_dirty_(true) {
}

void Camera::Initialize(float viewport_width, float viewport_height) {
    viewport_width_ = static_cast<int>(viewport_width);
    viewport_height_ = static_cast<int>(viewport_height);
    aspect_ratio_ = viewport_width / viewport_height;
    matrices_dirty_ = true;
}

void Camera::Update(float delta_time) {
    (void)delta_time;
    if (matrices_dirty_) {
        UpdateMatrices();
    }
}

void Camera::Update(GLFWwindow* window, float delta_time) {
    (void)window;
    Update(delta_time);
}

ozz::math::Float4x4 Camera::GetViewMatrix() const {
    if (matrices_dirty_) {
        UpdateMatrices();
    }
    return view_matrix_;
}

ozz::math::Float4x4 Camera::GetProjectionMatrix() const {
    if (matrices_dirty_) {
        UpdateMatrices();
    }
    return projection_matrix_;
}

ozz::math::Float4x4 Camera::GetViewProjectionMatrix() const {
    if (matrices_dirty_) {
        UpdateMatrices();
    }
    return projection_matrix_ * view_matrix_;
}

void Camera::SetPosition(const ozz::math::Float3& position) {
    // Convert position to center/angles/distance
    // This is tricky - we assume we want to look at current center
    const ozz::math::Float3 dir = center_ - position;
    distance_ = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

    if (distance_ > 0.001f) {
        const float norm_x = dir.x / distance_;
        const float norm_y = dir.y / distance_;
        const float norm_z = dir.z / distance_;

        angles_.x = asinf(norm_y);
        angles_.y = atan2(-norm_x, -norm_z);
    }

    matrices_dirty_ = true;
}

void Camera::SetTarget(const ozz::math::Float3& target) {
    center_ = target;
    matrices_dirty_ = true;
}

void Camera::SetUp(const ozz::math::Float3& up) {
    up_vector_ = up;
    matrices_dirty_ = true;
}

void Camera::SetFOV(float fov_degrees) {
    fov_radians_ = fov_degrees * kPI / 180.0f;
    matrices_dirty_ = true;
}

void Camera::SetNearFar(float near_plane, float far_plane) {
    near_plane_ = near_plane;
    far_plane_ = far_plane;
    matrices_dirty_ = true;
}

void Camera::SetAspectRatio(float aspect) {
    aspect_ratio_ = aspect;
    matrices_dirty_ = true;
}

void Camera::SetDistance(float distance) {
    distance_ = std::max(0.1f, distance);
    matrices_dirty_ = true;
}

float Camera::GetDistance() const {
    return distance_;
}

void Camera::OnMouseButton(int button, int action, int mods, double xpos, double ypos) {
    const bool pressed = (action == 1);  // GLFW_PRESS = 1

    if (button == 0) {  // GLFW_MOUSE_BUTTON_LEFT
        left_button_down_ = pressed;
    } else if (button == 1) {  // GLFW_MOUSE_BUTTON_RIGHT
        right_button_down_ = pressed;
    } else if (button == 2) {  // GLFW_MOUSE_BUTTON_MIDDLE
        middle_button_down_ = pressed;
    }

    if (pressed) {
        last_mouse_x_ = xpos;
        last_mouse_y_ = ypos;
    }
}

void Camera::OnMouseMove(double xpos, double ypos) {
    const double dx = xpos - last_mouse_x_;
    const double dy = ypos - last_mouse_y_;

    last_mouse_x_ = xpos;
    last_mouse_y_ = ypos;

    // Rotation (middle mouse button - like ozz RMB)
    if (middle_button_down_) {
        // Update angles using ozz's approach
        angles_.x = std::fmod(angles_.x - static_cast<float>(dy) * kAngleFactor, k2PI);
        angles_.y = std::fmod(angles_.y - static_cast<float>(dx) * kAngleFactor, k2PI);

        matrices_dirty_ = true;
    }

    // Panning (right mouse button - like ozz MMB)
    if (right_button_down_) {
        // Calculate world-space pan using ozz's approach
        if (viewport_width_ > 0 && viewport_height_ > 0 && distance_ > 0.001f) {
            const float aspect = static_cast<float>(viewport_width_) / static_cast<float>(viewport_height_);
            const float world_height = 2.0f * distance_ * std::tan(fov_radians_ * 0.5f);
            const float world_width = world_height * aspect;
            const float pan_scale_x = world_width / static_cast<float>(viewport_width_);
            const float pan_scale_y = world_height / static_cast<float>(viewport_height_);
            const float pan_world_x = -static_cast<float>(dx) * pan_scale_x;
            const float pan_world_y = static_cast<float>(dy) * pan_scale_y;

            // Build rotation matrix from angles to get right and up vectors
            const ozz::math::Float4x4 y_rotation = ozz::math::Float4x4::FromAxisAngle(
                ozz::math::simd_float4::y_axis(),
                ozz::math::simd_float4::Load1(angles_.y));
            const ozz::math::Float4x4 x_rotation = ozz::math::Float4x4::FromAxisAngle(
                ozz::math::simd_float4::x_axis(),
                ozz::math::simd_float4::Load1(angles_.x));
            const ozz::math::Float4x4 rotation = y_rotation * x_rotation;

            // Extract right and up vectors
            ozz::math::Float3 right, up;
            ozz::math::Store3PtrU(rotation.cols[0], &right.x);
            ozz::math::Store3PtrU(rotation.cols[1], &up.x);

            // Apply pan to center
            center_.x += pan_world_x * right.x + pan_world_y * up.x;
            center_.y += pan_world_x * right.y + pan_world_y * up.y;
            center_.z += pan_world_x * right.z + pan_world_y * up.z;

            matrices_dirty_ = true;
        }
    }
}

void Camera::OnMouseScroll(double xoffset, double yoffset) {
    (void)xoffset;

    // Zoom using ozz's approach
    distance_ *= 1.0f + static_cast<float>(-yoffset * kScrollFactor);
    distance_ = std::max(0.01f, std::min(distance_, 1000.0f));

    matrices_dirty_ = true;
}

void Camera::UpdateMatrices() const {
    // Build view matrix using ozz's approach: Invert(center * rotation * distance)

    // Build rotation matrices from Euler angles
    const ozz::math::Float4x4 y_rotation = ozz::math::Float4x4::FromAxisAngle(
        ozz::math::simd_float4::y_axis(),
        ozz::math::simd_float4::Load1(angles_.y));
    const ozz::math::Float4x4 x_rotation = ozz::math::Float4x4::FromAxisAngle(
        ozz::math::simd_float4::x_axis(),
        ozz::math::simd_float4::Load1(angles_.x));
    const ozz::math::Float4x4 rotation = y_rotation * x_rotation;

    // Build translation matrices
    const ozz::math::Float4x4 center_translation = ozz::math::Float4x4::Translation(
        ozz::math::simd_float4::Load(center_.x, center_.y, center_.z, 1.0f));
    const ozz::math::Float4x4 distance_translation = ozz::math::Float4x4::Translation(
        ozz::math::simd_float4::Load(0.0f, 0.0f, distance_, 1.0f));

    // Concatenate: center * rotation * distance
    const ozz::math::Float4x4 transform = center_translation * rotation * distance_translation;

    // Invert to get view matrix
    view_matrix_ = ozz::math::Invert(transform);

    // Compute eye position and look-at target for compatibility
    ozz::math::Store3PtrU(transform.cols[3], &eye_position_.x);
    look_at_target_ = center_;

    // Build projection matrix
    projection_matrix_ = Perspective(fov_radians_, aspect_ratio_, near_plane_, far_plane_);

    matrices_dirty_ = false;
}

} // namespace renderer
} // namespace animation
} // namespace xray
