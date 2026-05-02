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

    auto vsResult = shaderLoader->LoadVertexShader(sh_name, "main");
    auto psResult = shaderLoader->LoadPixelShader(sh_name, "main");

    if (vsResult.handle && psResult.handle)
    {
        m_vsHandle = vsResult.handle;
        m_psHandle = psResult.handle;
        Msg("* [FGFlareRender] Compiled flare shader: %s (tex: %s)", sh_name, tex_name);
    }
    else
    {
        Msg("! [FGFlareRender] Failed to compile flare shader: %s", sh_name);
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
