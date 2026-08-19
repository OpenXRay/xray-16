#include "stdafx.h"
#include "fgFlareRender.h"
#include "r_FrameGraphRenderer.h"
#include "FrameGraph/ShaderLoader.h"

namespace xray::render::fg
{
void FGFlareRender::Copy(IFlareRender& _in)
{
    *this = *static_cast<FGFlareRender*>(&_in);
}

void FGFlareRender::CreateShader(LPCSTR sh_name, LPCSTR tex_name)
{
    if (!tex_name || !tex_name[0])
        return;

    m_shaderName = sh_name;
    m_textureName = tex_name;

    auto* shaderLoader = RImplementation.GetShaderLoader();
    if (!shaderLoader)
        return;

    const char* vsName = sh_name;
    const char* psName = "sun_forward";
    if (sh_name)
    {
        if (strstr(sh_name, "flare"))
            vsName = "effects_flare";
        else if (strstr(sh_name, "sun"))
            vsName = "effects_sun";
    }

    auto vsResult = shaderLoader->LoadVertexShader(vsName, "main");
    auto psResult = shaderLoader->LoadPixelShader(psName, "main");

    if (vsResult.handle && psResult.handle)
    {
        m_vsHandle = vsResult.handle;
        m_psHandle = psResult.handle;
    }
    else if (psResult.handle)
    {
        auto fallbackVs = shaderLoader->LoadVertexShader("sun_forward", "main");
        if (fallbackVs.handle)
        {
            m_vsHandle = fallbackVs.handle;
            m_psHandle = psResult.handle;
        }
    }
}

void FGFlareRender::DestroyShader()
{
    m_vsHandle = nullptr;
    m_psHandle = nullptr;
    m_shaderName = nullptr;
    m_textureName = nullptr;
}
}
