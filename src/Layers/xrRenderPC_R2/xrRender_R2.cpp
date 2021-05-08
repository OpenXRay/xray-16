#include "stdafx.h"
#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"
#include "Layers/xrRender/D3DUtils.h"

constexpr pcstr RENDERER_R2A_MODE   = "renderer_r2a";
constexpr pcstr RENDERER_R2_MODE    = "renderer_r2";
constexpr pcstr RENDERER_R2_5_MODE  = "renderer_r2.5";

class R2RendererModule final : public RendererModule
{
    xr_vector<pcstr> modes;

public:
    const xr_vector<pcstr>& ObtainSupportedModes() override
    {
        if (!modes.empty())
        {
            return modes;
        }
        modes.emplace_back(RENDERER_R2A_MODE);
        modes.emplace_back(RENDERER_R2_MODE);

        D3DCAPS9 caps;
        CHW hw;
        hw.CreateD3D();
        hw.pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
        hw.DestroyD3D();

        u16 ps_ver_major = u16(u32(u32(caps.PixelShaderVersion) & u32(0xf << 8ul)) >> 8);
        if (ps_ver_major >= 3)
        {
            modes.emplace_back(RENDERER_R2_5_MODE);
        }
        return modes;
    }

    void CheckModeConsistency(pcstr mode) const
    {
        bool modeIsCorrect = false;
        if (0 == xr_strcmp(mode, RENDERER_R2A_MODE) ||
            0 == xr_strcmp(mode, RENDERER_R2_MODE) ||
            0 == xr_strcmp(mode, RENDERER_R2_5_MODE))
        {
            modeIsCorrect = true;
        }
        R_ASSERT3(modeIsCorrect, "Wrong mode passed to xrRender_R2.dll", mode);
    }

    void SetupEnv(pcstr mode) override
    {
        CheckModeConsistency(mode);

        ps_r2_sun_static = xr_strcmp(mode, RENDERER_R2A_MODE) == 0;
        ps_r2_advanced_pp = xr_strcmp(mode, RENDERER_R2_5_MODE) == 0;

        GEnv.Render = &RImplementation;
        GEnv.RenderFactory = &RenderFactoryImpl;
        GEnv.DU = &DUImpl;
        GEnv.UIRender = &UIRenderImpl;
#ifdef DEBUG
        GEnv.DRender = &DebugRenderImpl;
#endif
        xrRender_initconsole();
    }
} static s_r2_module;

extern "C"
{
XR_EXPORT RendererModule* GetRendererModule()
{
    return &s_r2_module;
}
}
