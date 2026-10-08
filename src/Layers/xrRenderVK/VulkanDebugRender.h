#pragma once

#ifdef DEBUG
#include "Include/xrRender/DebugRender.h"
#include "Include/xrRender/ObjectSpaceRender.h"
#include "xrCore/_matrix.h"
#include "xrCore/_sphere.h"
#include <vulkan/vulkan.h>

#include <array>
#include <vector>

namespace xray::render::vulkan
{
class VulkanGameDevice;
class VulkanDebugRender final : public IDebugRender
{
public:
    explicit VulkanDebugRender(VulkanGameDevice& device) : device_(device) { world_.identity(); }
    ~VulkanDebugRender() override;
    void OnDeviceDestroy();
    void Render() override;
    void add_lines(const Fvector* vertices, const u32& vertex_count,
        const u16* pairs, const u32& pair_count, const u32& color) override;
    void NextSceneMode() override;
    void ZEnable(bool enabled) override { depth_enabled_ = enabled; }
    void OnFrameEnd() override { Render(); }
    void SetShader(const debug_shader& shader) override;
    void CacheSetXformWorld(const Fmatrix& matrix) override { world_ = matrix; }
    void CacheSetCullMode(CullMode mode) override { cull_ = mode; }
    void SetAmbient(u32 color) override { ambient_ = color; }
    void SetDebugShader(dbgShaderHandle handle) override;
    void DestroyDebugShader(dbgShaderHandle handle) override;
    void dbg_DrawTRI(Fmatrix& transform, Fvector& a, Fvector& b,
        Fvector& c, u32 color) override;

private:
    struct Line { Fvector a, b; u32 color; };
    VulkanGameDevice& device_;
    std::vector<Line> lines_;
    Fmatrix world_{};
    CullMode cull_{cmNONE};
    bool depth_enabled_{true};
    u32 ambient_{};
    std::array<VkDescriptorSet, dbgShaderCount> shaders_{};
    VkDescriptorSet selected_{};
    unsigned scene_mode_{};
};

class VulkanObjectSpaceRender final : public IObjectSpaceRender
{
public:
    void Copy(IObjectSpaceRender& source) override;
    void dbgRender() override;
    void dbgAddSphere(const Fsphere& sphere, u32 color) override;
    void dbgReserveSphere(size_t count) override { spheres_.reserve(count); }
    void SetShader() override;
private:
    std::vector<std::pair<Fsphere, u32>> spheres_;
};
}
#endif
