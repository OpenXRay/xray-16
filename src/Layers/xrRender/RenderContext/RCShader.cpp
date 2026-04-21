#include "stdafx.h"
#include "RCShader.h"

namespace xray::render::fg {

RCShader::RCShader(ShaderStage stage,
                   nvrhi::ShaderHandle nvrhiShader,
                   const char* debugName)
    : m_stage(stage)
    , m_nvrhiShader(nvrhiShader)
    , m_debugName(debugName)
{
    VERIFY(m_nvrhiShader);
}

} // namespace xray::render::fg
