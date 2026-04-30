#include "stdafx.h"

#include "FrameGraphRendererModule.h"
#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"
#include "Layers/xrRender/D3DUtils.h"
#include "Layers/xrRender_R2/r2.h"

namespace xray::render::fg
{
constexpr pcstr RENDERER_FG_MODE = "renderer_fg";
constexpr int   RENDERER_FG_ID   = 5;

class FrameGraphRendererModule final : public RendererModule
{
    xr_vector<std::pair<pcstr, int>> modes;

public:
    const xr_vector<std::pair<pcstr, int>>& ObtainSupportedModes() override
    {
        ZoneScoped;
        if (modes.empty())
        {
            if (xrRender_test_hw())
                modes.emplace_back(RENDERER_FG_MODE, RENDERER_FG_ID);
        }
        return modes;
    }

    bool CheckGameRequirements() override
    {
        if (!FS.exist("$game_shaders$", RImplementation.getShaderPath()))
        {
            Log("~ No shaders found for FrameGraph renderer");
            return false;
        }
        return true;
    }

    void SetupEnv(pcstr mode) override
    {
        ZoneScoped;
        ps_r2_advanced_pp = true;

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
} static s_fg_module;

RendererModule* GetFrameGraphRendererModule()
{
    return &s_fg_module;
}
}
