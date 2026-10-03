#include "src/Layers/xrRenderVK/DeferredPass.h"
#include "vulkan_mock_device.h"

#include <cassert>

using namespace xray::render::vulkan;
int main()
{
    DeferredPass pass;
    std::string error;
    const auto shader = [](uintptr_t n) { return vk_mock::handle<VkShaderModule>(n); };
    assert(pass.initialize(vk_mock::handle<VkDevice>(1),
        vk_mock::handle<VkRenderPass>(2), vk_mock::handle<VkRenderPass>(3),
        shader(4), shader(5), shader(6), shader(7), shader(8), shader(9), shader(10),
        vk_mock::scene_dispatch(), error,
        vk_mock::handle<VkRenderPass>(11), shader(12), shader(13), shader(14),
        vk_mock::handle<VkRenderPass>(15), shader(16), shader(17)));
    VkDescriptorSet material{}, local{};
    const VkImageView image = vk_mock::handle<VkImageView>(18);
    const VkSampler sampler = vk_mock::handle<VkSampler>(19);
    assert(pass.material(image, sampler, material, error));
    assert(pass.local_light_set(image, sampler, vk_mock::handle<VkBuffer>(20),
        1024, 512, local, error));
    assert(pass.water_set(0, image, image, image, sampler,
        vk_mock::handle<VkBuffer>(20), error));
    VkDescriptorSet gbuffer{};
    assert(pass.gbuffer(image, image, image, sampler, gbuffer, error));
    const VkBuffer vertices = vk_mock::handle<VkBuffer>(21);
    const VkBuffer indices = vk_mock::handle<VkBuffer>(22);
    vk_mock::buffers[vertices] = 1024;
    vk_mock::buffers[indices] = 1024;
    float mvp[16]{};
    mvp[0] = mvp[5] = mvp[10] = mvp[15] = 1.f;
    FrameRecordingContext shadow{vk_mock::handle<VkCommandBuffer>(23),
        vk_mock::handle<VkRenderPass>(15), VK_NULL_HANDLE, {512,512}, 0, 0};
    assert(pass.record_sun_shadow(shadow, vertices, indices, 6, mvp, material, true));
    FrameRecordingContext frame{shadow.command_buffer, vk_mock::handle<VkRenderPass>(3),
        VK_NULL_HANDLE, {640,480}, 0, 0};
    assert(pass.record_local_light(frame, gbuffer, local));
    assert(pass.record_water(frame, vertices, indices, 6, mvp, material, 0, 2.f));
    assert(vk_mock::draws == 2);
    pass.release_water_sets();
    pass.release_gbuffer(gbuffer);
    pass.release_gbuffer(local);
    pass.release_gbuffer(material);
    pass.destroy();
    vk_mock::buffers.clear();
}
