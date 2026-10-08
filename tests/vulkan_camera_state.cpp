#include "src/Layers/xrRenderVK/VulkanCameraState.h"
#include "src/Layers/xrRenderVK/VulkanRenderContextState.h"

#include <cassert>

using namespace xray::render::vulkan;

int main()
{
    VulkanCameraState camera;
    assert(!camera.has_view_projection());
    assert(!camera.has_camera_position());

    float view[4][4]{};
    float project[4][4]{};
    for (int i = 0; i < 4; ++i)
    {
        view[i][i] = 1.f;
        project[i][i] = 1.f;
    }
    view[3][0] = -4.f;
    view[3][1] = 2.f;
    view[3][2] = -9.f;
    project[0][0] = 2.f;
    project[1][1] = 3.f;

    camera.set_cache_xform(view, project);
    assert(camera.has_view_projection());
    const auto& transform = camera.view_projection();
    assert(transform[0][0] == 2.f);
    assert(transform[1][1] == 3.f);
    assert(transform[3][0] == -8.f);
    assert(transform[3][1] == 6.f);
    assert(transform[3][2] == -9.f);

    float cached[4][4]{};
    camera.copy_view_projection(cached);
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column)
            assert(cached[row][column] == transform[row][column]);

    float device_transform[4][4]{};
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column)
            device_transform[row][column] = static_cast<float>(row * 4 + column + 1);
    camera.on_camera_updated(device_transform, 1.5f, -2.f, 7.f);
    assert(camera.has_camera_position());
    assert(camera.view_projection()[0][0] == 1.f);
    assert(camera.view_projection()[3][3] == 16.f);
    assert(camera.camera_position()[0] == 1.5f);
    assert(camera.camera_position()[1] == -2.f);
    assert(camera.camera_position()[2] == 7.f);

    VulkanRenderContextState contexts;
    assert(contexts.current() == VulkanRenderContextState::NoContext);
    contexts.device_created();
    assert(contexts.current() == VulkanRenderContextState::PrimaryContext);
    assert(contexts.make_current(VulkanRenderContextState::HelperContext));
    assert(contexts.current() == VulkanRenderContextState::PrimaryContext);
    assert(contexts.make_current(VulkanRenderContextState::NoContext));
    assert(contexts.current() == VulkanRenderContextState::NoContext);
    assert(!contexts.make_current(42));
    contexts.device_destroyed();
    assert(contexts.current() == VulkanRenderContextState::NoContext);
}
