#pragma once

#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/vec_float.h>

struct SDL_Window;

namespace xray {
namespace animation {
namespace renderer {

class Camera {
public:
    Camera();

    void Initialize(float viewport_width, float viewport_height);
    void Update(float delta_time);

    ozz::math::Float4x4 GetViewMatrix() const;
    ozz::math::Float4x4 GetProjectionMatrix() const;
    ozz::math::Float4x4 GetViewProjectionMatrix() const;

    void SetPosition(const ozz::math::Float3& position);
    void SetTarget(const ozz::math::Float3& target);
    void SetUp(const ozz::math::Float3& up);

    void Update(SDL_Window* window, float delta_time);
    void OnMouseButton(int button, int action, int mods, double xpos, double ypos);
    void OnMouseMove(double xpos, double ypos);
    void OnMouseScroll(double xoffset, double yoffset);
    bool SetPivotFromScreen(double xpos, double ypos) { return false; }
    void SetDistance(float distance);
    float GetDistance() const;
    void Reset() {
        SetPosition(ozz::math::Float3(2.0f, 2.0f, 2.0f));
        SetTarget(ozz::math::Float3(0.0f, 0.0f, 0.0f));
    }

    void SetFOV(float fov_degrees);
    void SetNearFar(float near_plane, float far_plane);
    void SetAspectRatio(float aspect);

    const ozz::math::Float3& GetPosition() const { return eye_position_; }
    const ozz::math::Float3& GetTarget() const { return look_at_target_; }
    float GetNear() const { return near_plane_; }
    float GetFar() const { return far_plane_; }

private:
    void UpdateMatrices() const;

    ozz::math::Float3 center_;
    ozz::math::Float2 angles_;
    float distance_;
    ozz::math::Float3 up_vector_;

    float fov_radians_;
    float aspect_ratio_;
    float near_plane_;
    float far_plane_;

    int viewport_width_;
    int viewport_height_;

    double last_mouse_x_;
    double last_mouse_y_;
    bool left_button_down_;
    bool middle_button_down_;
    bool right_button_down_;

    mutable ozz::math::Float3 eye_position_;
    mutable ozz::math::Float3 look_at_target_;
    mutable ozz::math::Float4x4 view_matrix_;
    mutable ozz::math::Float4x4 projection_matrix_;
    mutable bool matrices_dirty_;
};

}
}
}
