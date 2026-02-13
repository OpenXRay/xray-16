#pragma once

#include "xrCore/xrCore.h"
#include <nvrhi/nvrhi.h>

namespace xray::render
{

struct ShaderPassDesc
{
    shared_str name;
    shared_str psName;
    shared_str vsName;

    nvrhi::BlendState::RenderTarget blendRT;
    bool blendEnabled = false;
    bool alphaToCoverage = false;

    nvrhi::DepthStencilState depthStencil;
    nvrhi::RasterState rasterState;
    nvrhi::ColorMask colorWriteMask = nvrhi::ColorMask::All;

    bool hasAlphaTestOverride = false;
    u32 alphaTestRef = 0;

    xr_map<shared_str, shared_str> textures;
};

struct ShaderVariantDesc
{
    shared_str name;
    shared_str csName;

    xr_vector<ShaderPassDesc> passes;

    u8 sortPriority = 1;
    bool backToFront = false;
    bool transparent = false;
    bool fog = true;
    bool distort = false;
    bool emissive = false;
    bool wmark = false;

    xr_map<shared_str, shared_str> textures;

    u32 GetPassCount() const { return static_cast<u32>(passes.size()); }
    const ShaderPassDesc& GetPass(u32 i) const { return passes[i]; }
};

class ShaderVariantRegistry
{
public:
    static ShaderVariantRegistry& Instance();

    void Initialize();
    void Shutdown();

    const ShaderVariantDesc* GetVariant(const char* shaderName) const;
    u32 GetVariantIndex(const char* shaderName) const;
    const ShaderVariantDesc* GetVariantByIndex(u32 index) const;
    u32 GetVariantCount() const { return static_cast<u32>(m_variants.size()); }

private:
    ShaderVariantRegistry() = default;

    void LoadVariantFile(const char* filename, const char* fileData);

    xr_vector<ShaderVariantDesc> m_variants;
    xr_map<shared_str, u32> m_nameToVariantIndex;
    bool m_initialized = false;
};

} // namespace xray::render
