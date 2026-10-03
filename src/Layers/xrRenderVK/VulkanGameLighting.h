#pragma once

#include "xrEngine/IRenderable.h"
#include "xrEngine/Render.h"
#include "VulkanGameLightingMath.h"

#include <array>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
class VulkanLight final : public IRender_Light
{
public:
    void set_type(LT type) override;
    void set_active(bool active) override { state_.active = active; }
    bool get_active() override { return state_.active; }
    void set_shadow(bool value) override { state_.shadow = value; }
    void set_volumetric(bool value) override { volumetric_ = value; }
    void set_volumetric_quality(float value) override { volumetric_quality_ = value; }
    void set_volumetric_intensity(float value) override { volumetric_intensity_ = value; }
    void set_volumetric_distance(float value) override { volumetric_distance_ = value; }
    void set_indirect(bool value) override { indirect_ = value; }
    void set_position(const Fvector& value) override;
    void set_rotation(const Fvector& direction, const Fvector& right) override;
    void set_cone(float angle) override;
    void set_range(float range) override;
    void set_virtual_size(float size) override { virtual_size_ = size; }
    void set_texture(pcstr name) override { texture_ = name ? name : ""; }
    void set_color(const Fcolor& color) override;
    void set_color(float r, float g, float b) override;
    void set_hud_mode(bool value) override { hud_mode_ = value; }
    bool get_hud_mode() override { return hud_mode_; }

    const VulkanLightSnapshot& snapshot() const { return state_; }

private:
    VulkanLightSnapshot state_;
    std::array<float, 3> right_{1.f, 0.f, 0.f};
    std::string texture_;
    float virtual_size_{0.05f};
    float volumetric_quality_{1.f};
    float volumetric_intensity_{1.f};
    float volumetric_distance_{1.f};
    bool volumetric_{};
    bool indirect_{};
    bool hud_mode_{};
};

class VulkanGlow final : public IRender_Glow
{
public:
    void set_active(bool active) override { active_ = active; }
    bool get_active() override { return active_; }
    void set_position(const Fvector& value) override;
    void set_direction(const Fvector& value) override;
    void set_radius(float radius) override { radius_ = radius; }
    void set_texture(pcstr name) override { texture_ = name ? name : ""; }
    void set_color(const Fcolor& color) override;
    void set_color(float r, float g, float b) override;
    bool active() const { return active_; }
    const std::array<float, 3>& position() const { return position_; }
    const std::array<float, 4>& color() const { return color_; }
    const std::string& texture() const { return texture_; }
    float radius() const { return radius_; }

private:
    std::array<float, 3> position_{};
    std::array<float, 3> direction_{0.f, -1.f, 0.f};
    std::array<float, 4> color_{1.f, 1.f, 1.f, 1.f};
    std::string texture_;
    float radius_{1.f};
    bool active_{};
};

class VulkanObjectSpecific final : public IRender_ObjectSpecific
{
public:
    explicit VulkanObjectSpecific(IRenderable* parent) : parent_(parent) {}
    void force_mode(u32 mode) override { mode_ = mode & TRACE_ALL; }
    float get_luminocity() override { return luminocity_; }
    float get_luminocity_hemi() override { return hemi_luminocity_; }
    float* get_luminocity_hemi_cube() override { return hemi_cube_.data(); }
    void update(const float* ambient, const float* hemi, const float* sun,
        const std::vector<VulkanLightSnapshot>& lights);

private:
    IRenderable* parent_{};
    u32 mode_{TRACE_ALL};
    float luminocity_{};
    float hemi_luminocity_{};
    std::array<float, 6> hemi_cube_{};
};
}
