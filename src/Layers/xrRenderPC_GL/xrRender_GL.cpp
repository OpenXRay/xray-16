#include "stdafx.h"

#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"
#include "Layers/xrRender/D3DUtils.h"

#include "Include/xrRender/xrRender.h"

namespace xray::render::RENDER_NAMESPACE
{
constexpr pcstr RENDERER_R2_MODE   = "renderer_r2";   // id 2
constexpr pcstr RENDERER_R2_5_MODE = "renderer_r2.5"; // id 3
constexpr pcstr RENDERER_R3_MODE   = "renderer_r3";   // id 4
constexpr pcstr RENDERER_R4_MODE   = "renderer_r4";   // id 5
constexpr pcstr RENDERER_RGL_MODE = "renderer_rgl";   // id 6
#if defined(XR_PLATFORM_ANDROID)
constexpr pcstr RENDERER_GLES_MODE = "renderer_gles";       // id 6
constexpr pcstr RENDERER_VULKAN_MODE = "renderer_vulkan";   // id 7, GLES fallback
#endif

class RGLRendererModule final : public RendererModule
{
    xr_vector<std::pair<pcstr, int>> modes;

public:
    bool CheckCanAddMode() const
    {
        // don't duplicate
        if (!modes.empty())
        {
            return false;
        }
        return xrRender_test_hw();
    }

    const xr_vector<std::pair<pcstr, int>>& ObtainSupportedModes() override
    {
        ZoneScoped;

        if (CheckCanAddMode())
        {
#ifdef XR_PLATFORM_WINDOWS
            modes.emplace_back(RENDERER_RGL_MODE, 6);
#elif defined(XR_PLATFORM_ANDROID)
            // Keep both names visible to the standard renderer selector.
            // Vulkan is currently an opt-in compatibility mode backed by the
            // proven GLES renderer while native Vulkan work continues.
            modes.emplace_back(RENDERER_GLES_MODE, 6);
            modes.emplace_back(RENDERER_VULKAN_MODE, 7);
#else
            //modes.emplace_back(RENDERER_R2_MODE, 2);
            //modes.emplace_back(RENDERER_R2_5_MODE, 3);
            modes.emplace_back(RENDERER_R3_MODE, 4);
            //modes.emplace_back(RENDERER_R4_MODE, 5);
#endif
        }
        return modes;
    }

    bool CheckGameRequirements() override
    {
        // Check if shaders are available
        if (!FS.exist("$game_shaders$", RImplementation.getShaderPath()))
        {
            Log("~ No shaders found for OpenGL");
            return false;
        }
        return true;
    }

    void SetupEnv(pcstr mode) override
    {
        ZoneScoped;

        ps_r2_sun_static = false;
        ps_r2_advanced_pp = true;

        switch (strhash(mode))
        {
        //case strhash(RENDERER_R2A_MODE):
        //    //ps_r2_sun_static = true;
        //    [[fallthrough]];

        case strhash(RENDERER_R2_MODE):
            ps_r2_advanced_pp = false;
            break;

        case strhash(RENDERER_R2_5_MODE):
        case strhash(RENDERER_R3_MODE):
        case strhash(RENDERER_R4_MODE):
        case strhash(RENDERER_RGL_MODE):
#if defined(XR_PLATFORM_ANDROID)
        case strhash(RENDERER_GLES_MODE):
        case strhash(RENDERER_VULKAN_MODE):
#endif
            ps_r2_advanced_pp = true;
            break;
        }

        GEnv.Render = &RImplementation;
        GEnv.RenderFactory = &RenderFactoryImpl;
        GEnv.DU = &DUImpl;
        GEnv.UIRender = &UIRenderImpl;
#ifdef DEBUG
        GEnv.DRender = &DebugRenderImpl;
        rdebug_render->Register();
#endif
        xrRender_initconsole();
    }

    void ClearEnv() override
    {
        modes.clear();

        if (GEnv.Render == &RImplementation)
        {
            GEnv.Render = nullptr;
            GEnv.RenderFactory = nullptr;
            GEnv.DU = nullptr;
            GEnv.UIRender = nullptr;
            GEnv.DRender = nullptr;
#ifdef DEBUG
            rdebug_render->Unregister();
#endif
        }
    }
} static s_rgl_module;

RendererModule* GetRendererModule()
{
    return &s_rgl_module;
}
} // namespace xray::render::RENDER_NAMESPACE
