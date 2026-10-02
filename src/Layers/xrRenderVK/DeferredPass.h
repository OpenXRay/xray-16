#pragma once

#include "FrameContext.h"
#include "LevelModels.h"
#include "ScenePass.h"

namespace xray::render::vulkan
{
struct DeferredLight
{
    float direction_ambient[4];
    float color[4];
};

struct DeferredEnvironment
{
    float sun_direction[3]{0.f, -1.f, 0.f};
    float sun_color[3]{1.f, 1.f, 1.f};
    float ambient_color[3]{};
    float hemi_color[3]{};
};

enum class GeometryPhase
{
    OpaqueAndAlphaTest,
    Transparent,
    Hud
};

DeferredLight make_environment_deferred_light(const DeferredEnvironment& environment);

// Framebuffer attachment order: albedo, normal, depth. Color images must
// support color attachment and sampling; depth must support attachment and
// sampling because the render pass preserves it in a read-only layout.
bool create_gbuffer_render_pass(VkDevice device, VkFormat albedo_format,
    VkFormat normal_format, VkFormat depth_format, const FrameDispatch& vk,
    VkRenderPass& result, std::string& error);

// Geometry pass expects two color attachments (RGBA albedo, encoded normal)
// and depth. It owns opaque, alpha-test and blended pipelines. HUD geometry and
// lighting share the swapchain pass; the caller records HUD after lighting.
// Attachments, render-pass transitions and per-frame synchronization belong to
// the caller.
class DeferredPass
{
public:
    ~DeferredPass() { destroy(); }
    DeferredPass(const DeferredPass&) = delete;
    DeferredPass& operator=(const DeferredPass&) = delete;
    DeferredPass() = default;

    bool initialize(VkDevice device, VkRenderPass geometry_pass, VkRenderPass light_pass,
        VkShaderModule geometry_vertex, VkShaderModule geometry_fragment,
        VkShaderModule alpha_test_fragment,
        VkShaderModule light_vertex, VkShaderModule light_fragment,
        const ScenePassDispatch& dispatch, std::string& error);
    bool material(VkImageView albedo, VkSampler sampler, VkDescriptorSet& set, std::string& error);
    bool gbuffer(VkImageView albedo, VkImageView normal, VkSampler sampler,
        VkDescriptorSet& set, std::string& error);
    void release_gbuffer(VkDescriptorSet& set);
    void rebind_compatible_render_passes(VkRenderPass geometry_pass, VkRenderPass light_pass)
    {
        geometry_pass_ = geometry_pass;
        light_pass_ = light_pass;
    }
    bool record_geometry(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
        uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set,
        SurfaceMode mode = SurfaceMode::Opaque, uint32_t first_index = 0) const;
    bool record_hud(const FrameRecordingContext& frame, VkBuffer vertices, VkBuffer indices,
        uint32_t index_count, const float (&mvp)[16], VkDescriptorSet material_set,
        uint32_t first_index = 0) const;
    bool record_lighting(const FrameRecordingContext& frame, VkDescriptorSet gbuffer_set,
        const DeferredLight& light) const;
    void destroy();

private:
    bool allocate(VkDescriptorSetLayout layout, VkImageView first, VkImageView second,
        VkSampler sampler, VkDescriptorSet& set, std::string& error);
    VkDevice device_{};
    VkRenderPass geometry_pass_{}, light_pass_{};
    VkPipeline geometry_{}, alpha_test_{}, transparent_{}, hud_{}, lighting_{};
    VkPipelineLayout geometry_layout_{}, light_layout_{};
    VkDescriptorSetLayout material_layout_{}, gbuffer_layout_{};
    VkDescriptorPool pool_{};
    ScenePassDispatch vk_{};
};
}
