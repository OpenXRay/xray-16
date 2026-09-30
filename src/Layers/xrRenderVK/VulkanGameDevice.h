#pragma once

#include "DeferredFrame.h"
#include "DeferredShaderFactory.h"
#include "GameTextureFactory.h"
#include "VulkanUIRender.h"
#include "VulkanUIShader.h"
#include "VulkanWindowDevice.h"
#include "GpuModel.h"

#include <array>
#include <memory>

namespace xray::render::vulkan
{
// Owns the Vulkan gameplay frame resources. Destruction waits for submitted
// work before releasing descriptors, pipelines, images and the device.
class VulkanGameDevice
{
public:
    bool initialize(SDL_Window* window, VkExtent2D extent, std::string& error);
    bool render(const GpuLevel& level, const float (&mvp)[16],
        const DeferredLight& light, FrameStatus& status, std::string& error);
    void queue_model(GpuModel& model, IKinematics* skeleton, const float (&mvp)[16]);
    void destroy();
    ~VulkanGameDevice() { destroy(); }

    VulkanUIRender& ui() { return ui_; }
    std::unique_ptr<VulkanUIShader> create_ui_shader()
    {
        return std::make_unique<VulkanUIShader>(textures_, ui_pass_);
    }
    GameTextureFactory& textures() { return textures_; }
    DeferredPass& deferred() { return deferred_; }
    VulkanWindowDevice& window() { return window_; }
    const BufferUploadDispatch& buffer_upload() const { return buffer_upload_; }
    PFN_vkDeviceWaitIdle wait_idle() const { return frame_dispatch_.device_wait_idle; }

private:
    static void record_ui(const FrameRecordingContext& frame, void* user);
    static void record_models(const FrameRecordingContext& frame, void* user);
    struct ModelDraw
    {
        GpuModel* model{};
        IKinematics* skeleton{};
        std::array<float, 16> mvp{};
    };
    VulkanWindowDevice window_;
    FrameDispatch frame_dispatch_{};
    TextureUploadDispatch texture_dispatch_{};
    BufferUploadDispatch buffer_upload_{};
    GBufferTargets targets_;
    DeferredPass deferred_;
    ScenePass ui_pass_;
    GameTextureFactory textures_;
    VulkanUIRender ui_;
    DeferredFrame frame_;
    bool ui_recorded_{true};
    std::string ui_error_;
    std::vector<ModelDraw> model_draws_;
    bool models_recorded_{true};
    std::string model_error_;
};
}
