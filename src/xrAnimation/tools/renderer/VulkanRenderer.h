#pragma once

#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "Camera.h"
#include "InstancedSkeletonRenderer.h"
#include "InstancedMeshRenderer.h"
#include "DebugRenderer.h"

#include "../../ExtendedBoneMetadata.h"
#include "../../IDebugDrawContext.h"

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>
#include <vector>
#include <memory>

namespace ozz {
namespace sample {
struct Mesh;
} // namespace sample
} // namespace ozz

struct GLFWwindow;
struct GLFWmonitor;
struct ImGuiContext;

namespace xray {
namespace animation {
namespace renderer {

// Main Vulkan renderer - coordinates all rendering
class VulkanRenderer : public AnimationECS::IDebugDrawContext {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    bool Initialize(GLFWwindow* window);
   void Shutdown();

   void BeginFrame();
   void EndFrame();

    // Scene rendering
    void RenderScene();

    void SetClearColor(float r, float g, float b);
    void GetClearColor(float& r, float& g, float& b) const;

    Camera& GetCamera() { return camera_; }
    const Camera& GetCamera() const { return camera_; }
    VulkanDevice* GetDevice() { return &device_; }
    InstancedSkeletonRenderer& GetSkeletonRenderer() { return skeleton_renderer_; }
    const InstancedSkeletonRenderer& GetSkeletonRenderer() const { return skeleton_renderer_; }
    DebugRenderer& GetDebugRenderer() { return debug_renderer_; }
    const DebugRenderer& GetDebugRenderer() const { return debug_renderer_; }
    InstancedMeshRenderer& GetMeshRenderer() { return mesh_renderer_; }
    const InstancedMeshRenderer& GetMeshRenderer() const { return mesh_renderer_; }
    bool LoadBundleMesh(const ozz::sample::Mesh& mesh, const ozz::animation::Skeleton& skeleton);
    bool HasMeshLoaded() const { return mesh_loaded_; }
    bool HasSkeletonLoaded() const { return skeleton_loaded_; }
    bool SetSkeletonDebugData(const ozz::animation::Skeleton& skeleton,
        const XRay::Animation::ExtendedBoneMetadataCollection& metadata);

    // ImGui helpers
    bool IsImGuiInitialized() const { return imgui_initialized_; }
    ImGuiContext* GetImGuiContext() const { return imgui_context_; }

    // Feature toggles
    void SetShowTriangle(bool show);
    bool GetShowTriangle() const { return show_triangle_; }
    void SetShowSkeletonLines(bool show);
    bool GetShowSkeletonLines() const { return show_skeleton_lines_; }
    void SetShowSkinnedMesh(bool show);
    bool GetShowSkinnedMesh() const { return show_skinned_mesh_; }
    void SetUseGpuMeshInstancing(bool enabled);
    bool GetUseGpuMeshInstancing() const { return use_gpu_mesh_instancing_; }
    void SetUseGpuSkeletonInstancing(bool enabled);
    bool GetUseGpuSkeletonInstancing() const { return use_gpu_skeleton_instancing_; }

    // Frame timing
    float GetFrameDeltaSeconds() const { return static_cast<float>(frame_delta_seconds_); }
    float GetFrameDeltaMilliseconds() const { return static_cast<float>(frame_delta_seconds_ * 1000.0); }
    float GetFrameRate() const { return frame_delta_seconds_ > 0.0 ? static_cast<float>(1.0 / frame_delta_seconds_) : 0.0f; }

    // Simple animation controls
    void SetMeshRotationSpeed(float radians_per_second);
    float GetMeshRotationSpeed() const { return mesh_rotation_speed_; }
    void SetAnimateMesh(bool enabled);
    bool GetAnimateMesh() const { return animate_mesh_; }
    void SetMeshAnimationTime(float time_seconds);
    float GetMeshAnimationTime() const { return mesh_animation_time_; }
    void SetActiveAnimation(const ozz::animation::Animation* animation);
    void SetAnimationPlaybackSpeed(float speed) { animation_playback_speed_ = speed; }
    float GetAnimationPlaybackSpeed() const { return animation_playback_speed_; }

    // Bind pose mode
    bool GetShowBindPose() const { return show_bind_pose_; }
    void SetShowBindPose(bool show_bind_pose) { show_bind_pose_ = show_bind_pose; }
    const ozz::animation::Animation* GetActiveAnimation() const { return active_animation_; }
    bool HasActiveAnimation() const { return active_animation_ != nullptr; }

    // ECS Multi-instance rendering
    void SetECSInstances(const std::vector<MeshInstanceData>& instances, const std::vector<ozz::math::Float4x4>& bone_matrices);
    void ClearECSInstances();

    // IDebugDrawContext implementation (adapter pattern - forwards to DebugRenderer)
    void DrawLine(const ozz::math::Float3& start,
                  const ozz::math::Float3& end,
                  const ozz::math::Float4& color) override;

    void DrawSphere(const ozz::math::Float3& center,
                    float radius,
                    const ozz::math::Float4& color,
                    int segments = 12) override;

    void DrawBoneShape(const ozz::math::Float3& head,
                       const ozz::math::Float3& tail,
                       float radius,
                       const ozz::math::Float4& color) override;

    void DrawAxes(const ozz::math::Float4x4& transform,
                  float scale,
                  const ozz::math::Float4& color_x,
                  const ozz::math::Float4& color_y,
                  const ozz::math::Float4& color_z) override;

    void DrawBoneShapesInstanced(const BoneInstance* instances, size_t count) override;
    void DrawSpheresInstanced(const SphereInstance* instances, size_t count) override;

    // Mesh data accessors for skinning computation
    const std::vector<uint16_t>& GetMeshJointRemaps() const { return mesh_joint_remaps_; }
    const std::vector<ozz::math::Float4x4>& GetMeshInverseBindPoses() const { return mesh_inverse_bind_poses_; }
    const std::vector<ozz::math::Float4x4>& GetCurrentBoneMatrices() const { return mesh_bone_matrices_; }
    const ozz::math::Float4x4& GetMeshWorldTransform() const {
        return mesh_instances_.empty() ? ozz::math::Float4x4::identity() : mesh_instances_[0].transform;
    }

private:
    bool InitializeTrianglePipeline();
    bool InitializeDebugMesh();
    void RenderSkinnedMeshes(VkCommandBuffer cmd);
    void UpdateMeshAnimation(float delta_time_seconds);
    bool InitializeImGui();
    void ShutdownImGui();
    void BeginImGuiFrame();
    void RenderImGui(VkCommandBuffer cmd);
    void SetupGlfwCallbacks();
    void HandleFramebufferResize(int width, int height);
    void HandleMouseButton(int button, int action, int mods);
    void HandleCursorPosition(double xpos, double ypos);
    void HandleScroll(double xoffset, double yoffset);
    void HandleKeyEvent(int key, int scancode, int action, int mods);
    void HandleCursorEnter(int entered);
    void HandleWindowFocus(int focused);
    void RenderDebugPrimitives(VkCommandBuffer cmd);
    void ApplyPaletteToInstances(const std::vector<ozz::math::Float4x4>& palette);

    static void GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void GlfwCharCallback(GLFWwindow* window, unsigned int c);
    static void GlfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void GlfwScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void GlfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void GlfwCursorEnterCallback(GLFWwindow* window, int entered);
    static void GlfwWindowFocusCallback(GLFWwindow* window, int focused);
    static void GlfwFramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void GlfwMonitorCallback(GLFWmonitor* monitor, int event);

    GLFWwindow* window_ = nullptr;
    VulkanDevice device_;
    Camera camera_;
    InstancedSkeletonRenderer skeleton_renderer_;
    InstancedMeshRenderer mesh_renderer_;
    DebugRenderer debug_renderer_;
    std::vector<VkCommandBuffer> command_buffers_;
    float clear_color_[3] = {0.1f, 0.1f, 0.2f};  // Dark blue default

    // Triangle test pipeline
    VulkanPipeline triangle_pipeline_;
    bool triangle_pipeline_initialized_ = false;
    bool skeleton_renderer_initialized_ = false;
    bool mesh_renderer_initialized_ = false;
    bool mesh_loaded_ = false;
    bool debug_renderer_initialized_ = false;
    bool skeleton_loaded_ = false;
    bool show_triangle_ = true;
    bool show_skeleton_lines_ = true;
    bool show_skinned_mesh_ = true;
    bool use_gpu_mesh_instancing_ = true;
    bool use_gpu_skeleton_instancing_ = true;

    std::vector<MeshInstanceData> mesh_instances_;
    std::vector<ozz::math::Float4x4> mesh_bone_matrices_;
    std::vector<ozz::math::Float4x4> mesh_bind_pose_palette_;
    std::vector<ozz::math::Float4x4> sampled_palette_;
    std::vector<ozz::math::Float4x4> skeleton_rest_models_;
    std::vector<ozz::math::Float4x4> skeleton_pose_models_;
    std::vector<int> skeleton_parents_;
    XRay::Animation::ExtendedBoneMetadataCollection bone_metadata_;
    ozz::math::Float4x4 skeleton_world_transform_{ ozz::math::Float4x4::identity() };
    const ozz::animation::Skeleton* skeleton_source_ = nullptr;
    const ozz::animation::Animation* active_animation_ = nullptr;
    ozz::animation::SamplingJob::Context sampling_context_;
    ozz::vector<ozz::math::SoaTransform> local_transforms_;
    ozz::vector<ozz::math::Float4x4> model_transforms_;
    std::vector<uint16_t> mesh_joint_remaps_;
    std::vector<ozz::math::Float4x4> mesh_inverse_bind_poses_;

    // ImGui integration
    ImGuiContext* imgui_context_ = nullptr;
    VkDescriptorPool imgui_descriptor_pool_ = VK_NULL_HANDLE;
    bool imgui_initialized_ = false;
    bool imgui_frame_started_ = false;
    uint32_t imgui_image_count_ = 0;
    bool input_callbacks_installed_ = false;

    // Frame timing
    double frame_delta_seconds_ = 0.0;
    double last_frame_timestamp_ = 0.0;
    bool frame_time_initialized_ = false;

    // Mesh animation controls
    float mesh_rotation_speed_ = 0.75f;
    bool animate_mesh_ = true;
    float mesh_animation_time_ = 0.0f;
    bool show_bind_pose_ = false;
    float animation_playback_speed_ = 1.0f;  // Animation metadata speed multiplier
};

} // namespace renderer
} // namespace animation
} // namespace xray
