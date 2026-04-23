#include "stdafx.h"

#include "FrameGraphRendererModule.h"

namespace xray::render::fg
{
constexpr pcstr RENDERER_FG_MODE = "renderer_fg";
constexpr int   RENDERER_FG_ID   = 6;

class FrameGraphRendererModule final : public RendererModule
{
    xr_vector<std::pair<pcstr, int>> modes;

public:
    const xr_vector<std::pair<pcstr, int>>& ObtainSupportedModes() override
    {
        ZoneScoped;
        if (modes.empty())
        {
            modes.emplace_back(RENDERER_FG_MODE, RENDERER_FG_ID);
        }
        return modes;
    }

    bool CheckGameRequirements() override
    {
        return true;
    }

    void SetupEnv(pcstr mode) override
    {
        ZoneScoped;
        Msg("* [FrameGraphRendererModule] SetupEnv(%s) - skeleton stub, not wired", mode);
    }

    void ClearEnv() override
    {
        modes.clear();
        Msg("* [FrameGraphRendererModule] ClearEnv - skeleton stub");
    }
} static s_fg_module;

RendererModule* GetFrameGraphRendererModule()
{
    return &s_fg_module;
}
}
