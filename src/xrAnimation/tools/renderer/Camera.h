#pragma once

#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/vec_float.h>

struct GLFWwindow;

namespace xray {
namespace animation {
namespace renderer {

// Clean, simple camera implementation following Vulkan tutorial conventions
class Camera {
public:
    Camera();

    void Initialize(float viewport_width, float viewport_height);
    void Update(float delta_time);

    // Get matrices
    ozz::math::Float4x4 GetViewMatrix() const;
    ozz::math::Float4x4 GetProjectionMatrix() const;
    ozz::math::Float4x4 GetViewProjectionMatrix() const;

    // Camera controls
    void SetPosition(const ozz::math::Float3& position);
    void SetTarget(const ozz::math::Float3& target);
    void SetUp(const ozz::math::Float3& up);

    // Compatibility with old Camera interface
    void Update(GLFWwindow* window, float delta_time);
    void OnMouseButton(int button, int action, int mods, double xpos, double ypos);
    void OnMouseMove(double xpos, double ypos);
    void OnMouseScroll(double xoffset, double yoffset);
    bool SetPivotFromScreen(double xpos, double ypos) { return false; }  // Stub for compatibility
    void SetDistance(float distance);
    float GetDistance() const;
    void Reset() {
        SetPosition(ozz::math::Float3(2.0f, 2.0f, 2.0f));
        SetTarget(ozz::math::Float3(0.0f, 0.0f, 0.0f));
    }

    // Projection settings
    void SetFOV(float fov_degrees);
    void SetNearFar(float near_plane, float far_plane);
    void SetAspectRatio(float aspect);

    // Get camera state
    const ozz::math::Float3& GetPosition() const { return eye_position_; }
    const ozz::math::Float3& GetTarget() const { return look_at_target_; }
    float GetNear() const { return near_plane_; }
    float GetFar() const { return far_plane_; }

private:
    void UpdateMatrices() const;

    // Camera state (ozz-style orbit camera)
    ozz::math::Float3 center_;     // Rotation center (look-at point)
    ozz::math::Float2 angles_;     // Euler angles (pitch, yaw) in radians
    float distance_;               // Distance from center
    ozz::math::Float3 up_vector_;  // Up vector

    // Projection parameters
    float fov_radians_;
    float aspect_ratio_;
    float near_plane_;
    float far_plane_;

    // Viewport size (needed for panning calculations)
    int viewport_width_;
    int viewport_height_;

    // Mouse tracking
    double last_mouse_x_;
    double last_mouse_y_;
    bool left_button_down_;
    bool middle_button_down_;
    bool right_button_down_;

    // Cached matrices and state
    mutable ozz::math::Float3 eye_position_;  // Computed from center + angles + distance
    mutable ozz::math::Float3 look_at_target_; // Same as center_ (for compatibility)
    mutable ozz::math::Float4x4 view_matrix_;
    mutable ozz::math::Float4x4 projection_matrix_;
    mutable bool matrices_dirty_;
};

} // namespace renderer
} // namespace animation
} // namespace xray
