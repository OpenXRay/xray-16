#include "stdafx.h"

#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"
#include "Layers/xrRender/D3DUtils.h"

constexpr pcstr RENDERER_R2A_MODE  = "renderer_r2a";  // id 1
constexpr pcstr RENDERER_R2_MODE   = "renderer_r2";   // id 2
constexpr pcstr RENDERER_R2_5_MODE = "renderer_r2.5"; // id 3
constexpr pcstr RENDERER_R3_MODE   = "renderer_r3";   // id 4
constexpr pcstr RENDERER_R4_MODE   = "renderer_r4";   // id 5

class R4RendererModule final : public RendererModule
{
    xr_vector<std::pair<pcstr, int>> modes;

public:
    BOOL CheckCanAddMode() const
    {
        // don't duplicate
        if (!modes.empty())
        {
            return FALSE;
        }
        return xrRender_test_hw();
    }

    const xr_vector<std::pair<pcstr, int>>& ObtainSupportedModes() override
    {
        ZoneTransient(tracy_scoped_zone, true);

        const BOOL result = CheckCanAddMode();
        if (result != FALSE)
        {
            //modes.emplace_back(RENDERER_R2A_MODE, 1);
            modes.emplace_back(RENDERER_R2_MODE, 2);
            modes.emplace_back(RENDERER_R2_5_MODE, 3);
        }
        switch (result)
        {
        case TRUE:
            modes.emplace_back(RENDERER_R3_MODE, 4);
            break;
        case TRUE+TRUE: // XXX: remove hack
            modes.emplace_back(RENDERER_R3_MODE, 4); // don't optimize this switch with fallthrough, because
            modes.emplace_back(RENDERER_R4_MODE, 5); // order matters: R3 should be first, R4 should be second.
        }
        return modes;
    }

    bool CheckGameRequirements() override
    {
        // Check if shaders are available
        if (!FS.exist("$game_shaders$", RImplementation.getShaderPath()))
        {
            Log("~ No shaders found for xrRender_R4");
            return false;
        }
        return true;
    }

    void SetupEnv(pcstr mode) override
    {
        ZoneScoped;

        ps_r2_sun_static = false;

        switch (strhash(mode))
        {
        case strhash(RENDERER_R2A_MODE):
            // vanilla shaders fail to compile with static sun enabled
            ps_r2_sun_static = true;
            [[fallthrough]];

        case strhash(RENDERER_R2_MODE):
            HW.DX10Only = true;
            ps_r2_advanced_pp = false;
            break;

        case strhash(RENDERER_R2_5_MODE):
        case strhash(RENDERER_R3_MODE):
            HW.DX10Only = true;
            [[fallthrough]];

        case strhash(RENDERER_R4_MODE):
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
} static s_r4_module;

extern "C"
{
XR_EXPORT RendererModule* GetRendererModule()
{
    return &s_r4_module;
}
}
