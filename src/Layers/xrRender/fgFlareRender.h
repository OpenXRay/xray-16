#pragma once

#include "Include/xrRender/LensFlareRender.h"
#include <nvrhi/nvrhi.h>

namespace xray::render::fg
{
class FGFlareRender : public IFlareRender
{
public:
    void Copy(IFlareRender& _in) override;
    void CreateShader(LPCSTR sh_name, LPCSTR tex_name) override;
    void DestroyShader() override;

    shared_str m_shaderName;
    shared_str m_textureName;
    nvrhi::ShaderHandle m_vsHandle;
    nvrhi::ShaderHandle m_psHandle;
};
}
