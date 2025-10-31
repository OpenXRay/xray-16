#pragma once

#include <nvrhi/nvrhi.h>

namespace xray::render::ng {

// ═══════════════════════════════════════════════════
//  SHADER WRAPPER
// ═══════════════════════════════════════════════════

enum class ShaderStage {
    Vertex,
    Pixel,
    Geometry,
    Hull,      // Tessellation control
    Domain,    // Tessellation evaluation
    Compute
};

class RCShader {
public:
    RCShader(ShaderStage stage,
             nvrhi::ShaderHandle nvrhiShader,
             const char* debugName);

    ShaderStage GetStage() const { return m_stage; }
    nvrhi::IShader* GetNativeShader() const { return m_nvrhiShader.Get(); }
    const shared_str& GetDebugName() const { return m_debugName; }

private:
    ShaderStage m_stage;
    nvrhi::ShaderHandle m_nvrhiShader;
    shared_str m_debugName;
};

} // namespace xray::render::ng
