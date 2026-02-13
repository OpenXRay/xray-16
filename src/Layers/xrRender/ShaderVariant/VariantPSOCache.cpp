#include "stdafx.h"
#include "VariantPSOCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/GPUCullingManager.h"

namespace xray::render
{

VariantPSOCache& VariantPSOCache::Instance()
{
    static VariantPSOCache instance;
    return instance;
}

nvrhi::ShaderHandle VariantPSOCache::LoadShader(nvrhi::ShaderType type, const char* name)
{
    string256 cacheKey;
    xr_sprintf(cacheKey, "%d:%s", static_cast<int>(type), name);
    shared_str key(cacheKey);
    auto it = m_shaderCache.find(key);
    if (it != m_shaderCache.end())
        return it->second;

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return nullptr;

    auto result = type == nvrhi::ShaderType::Vertex
        ? shaderLoader->LoadVertexShader(name, "main")
        : shaderLoader->LoadPixelShader(name, "main");
    if (!result.handle)
    {
        Msg("! [VariantPSO] Failed to load %s: %s",
            type == nvrhi::ShaderType::Vertex ? "VS" : "PS", name);
        return nullptr;
    }

    m_shaderCache[key] = result.handle;
    return result.handle;
}

static const char* GetDefaultSkinnedVS(u32 vertexFormat)
{
    switch (vertexFormat)
    {
    case VF_SKINNED_NONHQ: return "bindless_skinned";
    case VF_SKINNED_HQ1W:
    case VF_SKINNED_HQ2W:
    case VF_SKINNED_HQ3W:
    case VF_SKINNED_HQ4W:  return "bindless_skinned_hq";
    default: return nullptr;
    }
}

nvrhi::IGraphicsPipeline* VariantPSOCache::GetOrCreatePSO(
    nvrhi::IDevice* device,
    nvrhi::IFramebuffer* framebuffer,
    u32 variantIndex,
    const ShaderVariantDesc& variant,
    u32 passIndex,
    u32 vertexFormat,
    nvrhi::IInputLayout* inputLayout,
    nvrhi::IBindingLayout* passBindingLayout,
    nvrhi::IBindingLayout* bindlessLayout)
{
    VariantPSOKey key{variantIndex, passIndex, vertexFormat};

    auto it = m_cache.find(key);
    if (it != m_cache.end())
        return it->second.Get();

    if (passIndex >= variant.GetPassCount())
        return nullptr;

    const auto& pass = variant.GetPass(passIndex);

    const char* vsName = pass.vsName.c_str();
    if (vertexFormat >= VF_SKINNED_NONHQ && !xr_strcmp(vsName, "bindless_forward"))
    {
        const char* skinnedVS = GetDefaultSkinnedVS(vertexFormat);
        if (skinnedVS)
            vsName = skinnedVS;
    }

    auto vs = LoadShader(nvrhi::ShaderType::Vertex, vsName);
    auto ps = LoadShader(nvrhi::ShaderType::Pixel, pass.psName.c_str());
    if (!vs || !ps)
        return nullptr;

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = vs;
    pipeDesc.PS = ps;
    pipeDesc.inputLayout = inputLayout;
    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;

    if (bindlessLayout)
        pipeDesc.bindingLayouts = {passBindingLayout, bindlessLayout};
    else
        pipeDesc.bindingLayouts = {passBindingLayout};

    pipeDesc.renderState.depthStencilState = pass.depthStencil;
    pipeDesc.renderState.rasterState = pass.rasterState;
    pipeDesc.renderState.rasterState.frontCounterClockwise = false;

    if (pass.blendEnabled)
    {
        pipeDesc.renderState.blendState.targets[0] = pass.blendRT;
        pipeDesc.renderState.blendState.alphaToCoverageEnable = pass.alphaToCoverage;
    }

    auto pipeline = device->createGraphicsPipeline(pipeDesc, framebuffer);
    if (!pipeline)
    {
        Msg("! [VariantPSO] Failed to create pipeline for '%s' pass %d", variant.name.c_str(), passIndex);
        return nullptr;
    }

    Msg("* [VariantPSO] Created pipeline: '%s' pass=%d vs=%s ps=%s blend=%s",
        variant.name.c_str(), passIndex, vsName, pass.psName.c_str(),
        pass.blendEnabled ? "yes" : "no");

    m_cache[key] = pipeline;
    return pipeline.Get();
}

void VariantPSOCache::Shutdown()
{
    m_cache.clear();
    m_shaderCache.clear();
}

void DrawVariantPartition(
    nvrhi::ICommandList* cmdList,
    nvrhi::IDevice* nvDevice,
    nvrhi::IFramebuffer* framebuffer,
    nvrhi::GraphicsState& state,
    const VariantPartitionDrawConfig& cfg)
{
    auto& registry = ShaderVariantRegistry::Instance();
    auto& psoCache = VariantPSOCache::Instance();

    const auto& p = cfg.partition;

    for (u32 v = 0; v < p.variantCount; v++) {
        nvrhi::IGraphicsPipeline* pso;
        if (v == 0) {
            pso = cfg.defaultPipeline;
        } else {
            const auto* variant = registry.GetVariantByIndex(v);
            if (!variant) continue;
            if (cfg.selectTransparent ? !variant->transparent : variant->transparent) continue;
            pso = psoCache.GetOrCreatePSO(nvDevice, framebuffer, v, *variant, 0, VF_MDI,
                cfg.inputLayout, cfg.passLayout, cfg.bindlessLayout);
            if (!pso) continue;
        }

        nvrhi::BindingSetDesc bindDesc;
        bindDesc.bindings = {
            nvrhi::BindingSetItem::ConstantBuffer(2, cfg.staticGlobalsCB),
            nvrhi::BindingSetItem::ConstantBuffer(4, cfg.lightingCB),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(8, cfg.materialBuffer),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(10, cfg.variantTexBuffer),
            nvrhi::BindingSetItem::Sampler(0, cfg.sampler),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(14, cfg.instanceBuffer),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(15, p.batchIndicesBuffer),
            nvrhi::BindingSetItem::StructuredBuffer_SRV(16, p.materialIDsBuffer),
        };
        auto bindingSet = nvDevice->createBindingSet(bindDesc, cfg.passLayout);
        if (!bindingSet) continue;

        state.pipeline = pso;
        state.bindings = { bindingSet };
        if (cfg.bindlessTable)
            state.addBindingSet(cfg.bindlessTable);
        state.vertexBuffers = {
            {cfg.megaVertexBuffer, 0, 0},
            {p.drawIndexBuffer, 1, 0}
        };
        state.indirectParams = p.drawArgsBuffer;
        state.indirectCountBuffer = p.countBuffer;

        cmdList->setGraphicsState(state);
        cmdList->drawIndexedIndirectCount(
            v * p.binCapacity * sizeof(RENDER_NAMESPACE::IndirectDrawArgs),
            v * sizeof(u32),
            p.binCapacity);
    }

    cmdList->setBufferState(p.drawArgsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(p.countBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(p.batchIndicesBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(p.materialIDsBuffer, nvrhi::ResourceStates::UnorderedAccess);
}

} // namespace xray::render
