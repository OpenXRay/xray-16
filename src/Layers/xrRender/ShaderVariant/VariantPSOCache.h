#pragma once

#include "xrCore/xrCore.h"
#include "ShaderVariantRegistry.h"
#include "VariantPartitionConfig.h"
#include <nvrhi/nvrhi.h>

namespace xray::render
{

struct VariantPSOKey
{
    u32 variantIndex;
    u32 passIndex;
    u32 vertexFormat;

    bool operator<(const VariantPSOKey& o) const
    {
        if (variantIndex != o.variantIndex) return variantIndex < o.variantIndex;
        if (passIndex != o.passIndex) return passIndex < o.passIndex;
        return vertexFormat < o.vertexFormat;
    }
};

enum VertexFormatID : u32
{
    VF_MDI = 0,
    VF_SKINNED_NONHQ = 1,
    VF_SKINNED_HQ1W = 2,
    VF_SKINNED_HQ4W = 3,
    VF_SKINNED_HQ2W = 4,
    VF_SKINNED_HQ3W = 5,
};

class VariantPSOCache
{
public:
    static VariantPSOCache& Instance();

    nvrhi::IGraphicsPipeline* GetOrCreatePSO(
        nvrhi::IDevice* device,
        nvrhi::IFramebuffer* framebuffer,
        u32 variantIndex,
        const ShaderVariantDesc& variant,
        u32 passIndex,
        u32 vertexFormat,
        nvrhi::IInputLayout* inputLayout,
        nvrhi::IBindingLayout* passBindingLayout,
        nvrhi::IBindingLayout* bindlessLayout
    );

    void Shutdown();

private:
    VariantPSOCache() = default;

    nvrhi::ShaderHandle LoadShader(nvrhi::ShaderType type, const char* name);

    xr_map<VariantPSOKey, nvrhi::GraphicsPipelineHandle> m_cache;
    xr_map<shared_str, nvrhi::ShaderHandle> m_shaderCache;
};

struct VariantPartitionDrawConfig
{
    nvrhi::IGraphicsPipeline* defaultPipeline = nullptr;
    nvrhi::IInputLayout* inputLayout = nullptr;
    nvrhi::IBindingLayout* passLayout = nullptr;
    nvrhi::IBindingLayout* bindlessLayout = nullptr;
    nvrhi::IBindingSet* bindlessTable = nullptr;
    nvrhi::ISampler* sampler = nullptr;
    nvrhi::IBuffer* staticGlobalsCB = nullptr;
    nvrhi::IBuffer* lightingCB = nullptr;
    nvrhi::IBuffer* materialBuffer = nullptr;
    nvrhi::IBuffer* variantTexBuffer = nullptr;
    nvrhi::IBuffer* instanceBuffer = nullptr;
    nvrhi::IBuffer* megaVertexBuffer = nullptr;
    VariantPartitionConfig partition;
    bool selectTransparent = false;
};

void DrawVariantPartition(
    nvrhi::ICommandList* cmdList,
    nvrhi::IDevice* nvDevice,
    nvrhi::IFramebuffer* framebuffer,
    nvrhi::GraphicsState& state,
    const VariantPartitionDrawConfig& cfg);

} // namespace xray::render
