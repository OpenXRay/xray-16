#include "xrEngine/stdafx.h"
#include "Include/xrRender/xrRender.h"
#include "src/Layers/xrRenderVK/VulkanLevelRender.h"
#include "src/Layers/xrRenderVK/VulkanRenderFactory.h"
#include "src/Layers/xrRenderVK/VulkanUIRender.h"
#include "src/Layers/xrRenderVK/VulkanDrawUtils.h"
#include "src/Layers/xrRenderVK/VulkanDebugRender.h"
#include "src/Layers/xrRenderVK/VulkanEnvironmentRender.h"
#include "Include/xrRender/RenderFactory.h"

#include <cassert>

namespace xray::render::vulkan
{
RendererModule* GetRendererModule();
}

int main()
{
    Core.Initialize("vulkan_renderer_module_test", nullptr, false);
    auto* module = xray::render::vulkan::GetRendererModule();
    for (int i = 0; i < 2; ++i)
    {
        module->SetupEnv("renderer_vulkan");
        assert(dynamic_cast<xray::render::vulkan::VulkanLevelRender*>(GEnv.Render));
        assert(dynamic_cast<xray::render::vulkan::VulkanRenderFactory *>(GEnv.RenderFactory));
        auto* environment = GEnv.RenderFactory->CreateEnvironmentRender();
        auto& groups = environment->particles_systems_library();
        assert(groups.particles_group_begin() == groups.particles_group_end());
        xr_vector<shared_str> ids;
        groups.particles_group_ids(ids);
        assert(ids.empty());
        GEnv.RenderFactory->DestroyEnvironmentRender(environment);
        assert(dynamic_cast<xray::render::vulkan::VulkanDrawUtils *>(GEnv.DU));
        assert(dynamic_cast<xray::render::vulkan::VulkanUIRender *>(GEnv.UIRender));
        u32 flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
        GEnv.Render->ObtainRequiredWindowFlags(flags);
        assert((flags & SDL_WINDOW_VULKAN) && !(flags & SDL_WINDOW_OPENGL));
#ifdef DEBUG
        assert(dynamic_cast<xray::render::vulkan::VulkanDebugRender *>(GEnv.DRender));
#endif
        module->ClearEnv();
        assert(!GEnv.Render && !GEnv.RenderFactory && !GEnv.DU && !GEnv.UIRender && !GEnv.DRender);
    }
}
