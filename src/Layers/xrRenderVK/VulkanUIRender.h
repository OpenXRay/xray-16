#pragma once

#include "Include/xrRender/UIRender.h"
#include "BufferResource.h"
#include "ScenePass.h"

#include <array>
#include <vector>
#include <functional>
struct ImDrawData;

namespace xray::render::vulkan
{
class VulkanUIShader;
struct UiDescriptorLeases
{
    std::function<bool(VkDescriptorSet)> retain;
    std::function<void(VkDescriptorSet)> release;
};
// Accumulates engine UI primitives on the CPU and records Vulkan indexed
// draws when FrameContext reaches the swapchain render pass.
class VulkanUIRender final : public IUIRender
{
  public:
    void configure(VkDevice device, const VkPhysicalDeviceMemoryProperties &memory, const BufferResourceDispatch &dispatch, const ScenePass &pass,
                   UiDescriptorLeases leases = {});
    void setup_states();
    void reset_frame();
    bool record(const FrameRecordingContext &frame, std::string &error);
    void append_imgui(ImDrawData* data);
    VulkanUIShader* current_shader() const { return shader_; }
    uint32_t draw_calls() const { return static_cast<uint32_t>(batches_.size()); }
    uint32_t triangles() const { return static_cast<uint32_t>(indices_.size() / 3); }

    void CreateUIGeom() override;
    void DestroyUIGeom() override;
    void SetShader(IUIShader& shader) override;
    void SetTextureDescriptor(VkDescriptorSet descriptor);
    void SetAlphaRef(int aref) override;
    void SetScissor(Irect* rect = nullptr) override;
    void PushPoint(float x, float y, float z, u32 color, float u, float v) override;
    void StartPrimitive(u32 max_vertices, ePrimitiveType type, ePointType point_type) override;
    void FlushPrimitive() override;
    LPCSTR UpdateShaderName(LPCSTR texture, LPCSTR shader) override;
    void CacheSetXformWorld(const Fmatrix& matrix) override;
    void CacheSetCullMode(CullMode mode) override;

private:
    struct Batch
    {
        uint32_t first_index{}, count{};
        VkDescriptorSet texture{};
        VkRect2D scissor{};
        bool has_scissor{};
        float alpha_ref{};
        int blend_mode{1};
    };
    struct FrameBuffers
    {
        BufferResource vertices, indices;
    };
    bool ensure(BufferResource& buffer, VkDeviceSize bytes, VkBufferUsageFlags usage,
        std::string& error);
    void add_line(uint32_t a, uint32_t b);
    void add_triangle(uint32_t a, uint32_t b, uint32_t c);

    VkDevice device_{};
    VkPhysicalDeviceMemoryProperties memory_{};
    BufferResourceDispatch dispatch_{};
    const ScenePass *pass_{};
    UiDescriptorLeases leases_;
    std::array<FrameBuffers, FrameContext::FramesInFlight> frames_{};
    std::vector<UiVertex> vertices_;
    std::vector<Fvector> world_positions_;
    std::vector<uint8_t> visible_;
    std::vector<uint32_t> indices_;
    std::vector<Batch> batches_;
    VkDescriptorSet texture_{};
    VulkanUIShader* shader_{};
    VkRect2D scissor_{};
    bool has_scissor_{};
    ePrimitiveType primitive_{ptNone};
    ePointType point_type_{pttNone};
    uint32_t first_vertex_{}, limit_{};
    int alpha_ref_{};
    int blend_mode_{1};
    int material_alpha_ref_{};
    CullMode cull_{cmNONE};
    Fmatrix world_{};
};
} // namespace xray::render::vulkan
