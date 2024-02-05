#include "stdafx.h"
#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"
#include "Layers/xrRender/D3DUtils.h"

constexpr pcstr RENDERER_R5_MODE = "renderer_r5";

class R5RendererModule final : public RendererModule
{
    xr_vector<pcstr> modes;

public:
    BOOL CheckCanAddMode() const
    {
        // don't duplicate
        if (!modes.empty())
        {
            return FALSE;
        }
        // Check if shaders are available
        if (!FS.exist("$game_shaders$", RImplementation.getShaderPath()))
        {
            Log("~ No shaders found for xrRender_R5");
            return FALSE;
        }
        return xrRender_test_hw();
    }

    const xr_vector<pcstr>& ObtainSupportedModes() override
    {
        switch (CheckCanAddMode())
        {
        case TRUE:
            modes.emplace_back(RENDERER_R5_MODE);
            break;
        }
        return modes;
    }

    void CheckModeConsistency(pcstr mode) const
    {
        bool modeIsCorrect = false;
        if (0 == xr_strcmp(mode, RENDERER_R5_MODE))
        {
            modeIsCorrect = true;
        }
        R_ASSERT3(modeIsCorrect, "Wrong mode passed to xrRender_R5", mode);
    }

    void SetupEnv(pcstr mode) override
    {
        CheckModeConsistency(mode);
        ps_r2_sun_static = false;
        ps_r2_advanced_pp = true;
        GEnv.Render = &RImplementation;
        GEnv.RenderFactory = &RenderFactoryImpl;
        GEnv.DU = &DUImpl;
        GEnv.UIRender = &UIRenderImpl;
#ifdef DEBUG
        GEnv.DRender = &DebugRenderImpl;
#endif
        xrRender_initconsole();
    }
} static s_r5_module;

extern "C"
{
XR_EXPORT RendererModule* GetRendererModule()
{
    return &s_r5_module;
}
}
