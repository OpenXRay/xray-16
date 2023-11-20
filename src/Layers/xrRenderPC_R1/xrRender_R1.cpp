#include "stdafx.h"

#include "Layers/xrRender/dxRenderFactory.h"
#include "Layers/xrRender/dxUIRender.h"
#include "Layers/xrRender/dxDebugRender.h"
#include "Layers/xrRender/D3DUtils.h"

#include "Layers/xrRenderDX9/dx9shader_utils.h"

constexpr pcstr RENDERER_R1_MODE = "renderer_r1";

class R1RendererModule final : public RendererModule
{
    xr_vector<pcstr> modes;

public:
    bool CheckCanAddMode() const
    {
        // don't duplicate
        if (!modes.empty())
        {
            return false;
        }
        // Check if shaders are available
        if (!FS.exist("$game_shaders$", RImplementation.getShaderPath()))
        {
            Log("~ No shaders found for xrRender_R1");
            return false;
        }
        CHW hw;
        hw.CreateD3D();
        const bool result = hw.pD3D && hw.pD3D->GetAdapterCount() > 0;
        hw.DestroyD3D();

#ifdef USE_D3DX
        const bool shaderCompilerAvailable = XRay::ModuleHandle{ "d3dx9_31" }.IsLoaded();
#else
        constexpr bool shaderCompilerAvailable = true;
#endif

        return result && shaderCompilerAvailable;
    }

    const xr_vector<pcstr>& ObtainSupportedModes() override
    {
        if (CheckCanAddMode())
        {
            modes.emplace_back(RENDERER_R1_MODE);
        }
        return modes;
    }

    void CheckModeConsistency(pcstr mode) const
    {
        R_ASSERT3(0 == xr_strcmp(mode, RENDERER_R1_MODE),
            "Wrong mode passed to xrRender_R1", mode);
    }

    void SetupEnv(pcstr mode) override
    {
        CheckModeConsistency(mode);
        GEnv.Render = &RImplementation;
        GEnv.RenderFactory = &RenderFactoryImpl;
        GEnv.DU = &DUImpl;
        GEnv.UIRender = &UIRenderImpl;
#ifdef DEBUG
        GEnv.DRender = &DebugRenderImpl;
#endif
        xrRender_initconsole();
    }
} static s_r1_module;

extern "C"
{
XR_EXPORT RendererModule* GetRendererModule()
{
    return &s_r1_module;
}
}
