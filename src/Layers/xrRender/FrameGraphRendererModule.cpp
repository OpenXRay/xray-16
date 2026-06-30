#include "stdafx.h"

#include "FrameGraphRendererModule.h"
#include "Layers/xrRender/fgRenderFactory.h"
#include "Layers/xrRender/D3DUtils.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"

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
            modes.emplace_back(RENDERER_FG_MODE, RENDERER_FG_ID);
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
        }
    }
} static s_fg_module;

RendererModule* GetFrameGraphRendererModule()
{
    return &s_fg_module;
}
}
