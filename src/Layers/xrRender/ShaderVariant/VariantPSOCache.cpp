#include "stdafx.h"
#include "VariantPSOCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/GPUCullingManager.h"
#include "Layers/xrRender/FrameGraphPasses/PassCommon.h"
#include "Layers/xrRender/FrameGraphPasses/IBLPrefilterPassSetup.h"

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

static bool IsSkinnedVertexFormat(u32 vertexFormat)
{
    return vertexFormat >= VF_SKINNED_NONHQ && vertexFormat <= VF_SKINNED_HQ3W;
}

static const char* GetDefaultSkinnedVS(u32 vertexFormat)
{
    switch (vertexFormat)
    {
    case VF_SKINNED_NONHQ: return "bindless_skinned";
    case VF_SKINNED_HQ1W:  return "bindless_skinned_hq";
    case VF_SKINNED_HQ2W:  return "bindless_skinned_2w";
    case VF_SKINNED_HQ3W:  return "bindless_skinned_3w";
    case VF_SKINNED_HQ4W:  return "bindless_skinned_4w";
    default: return nullptr;
    }
}

static const char* GetDefaultSkinnedPS(u32 vertexFormat)
{
    return IsSkinnedVertexFormat(vertexFormat) ? "bindless_skinned" : nullptr;
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
    if (!framebuffer)
        return nullptr;

    const auto& fbInfo = framebuffer->getFramebufferInfo();
    VariantPSOKey key{
        variantIndex,
        passIndex,
        vertexFormat,
        static_cast<u32>(fbInfo.colorFormats.size()),
        static_cast<u32>(fbInfo.depthFormat),
        fbInfo.sampleCount};

    auto it = m_cache.find(key);
    if (it != m_cache.end())
        return it->second.Get();

    if (passIndex >= variant.GetPassCount())
        return nullptr;

    const auto& pass = variant.GetPass(passIndex);

    const char* vsName = pass.vsName.c_str();
    const char* psName = pass.psName.c_str();
    bool remappedToSkinned = false;

    if (IsSkinnedVertexFormat(vertexFormat))
    {
        if (const char* skinnedVS = GetDefaultSkinnedVS(vertexFormat))
        {
            vsName = skinnedVS;
            remappedToSkinned = true;
        }
        if (const char* skinnedPS = GetDefaultSkinnedPS(vertexFormat))
        {
            psName = skinnedPS;
            remappedToSkinned = true;
        }
    }

    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return nullptr;

    auto vsResult = shaderLoader->LoadVertexShader(vsName, "main");
    auto psResult = shaderLoader->LoadPixelShader(psName, "main");
    if (!vsResult.handle || !psResult.handle)
        return nullptr;

    {
        string256 vsKey, psKey;
        xr_sprintf(vsKey, "%d:%s", static_cast<int>(nvrhi::ShaderType::Vertex), vsName);
        xr_sprintf(psKey, "%d:%s", static_cast<int>(nvrhi::ShaderType::Pixel), psName);
        m_shaderCache[shared_str(vsKey)] = vsResult.handle;
        m_shaderCache[shared_str(psKey)] = psResult.handle;
    }

    nvrhi::BindingLayoutHandle layoutHandle = passBindingLayout;
    const bool customShaders =
        !remappedToSkinned &&
        (xr_strcmp(vsName, "bindless_forward") != 0 ||
         xr_strcmp(psName, "bindless_forward") != 0);
    if (customShaders && vsResult.reflection && psResult.reflection)
    {
        string256 layoutName;
        xr_sprintf(layoutName, "Variant_%s_p%u_fmt%u", variant.name.c_str(), passIndex, vertexFormat);
        auto customLayout = framegraph::GetPassResourceCache().GetOrCreateBindingLayoutFromReflection(
            layoutName, *vsResult.reflection, *psResult.reflection, device);
        if (customLayout)
            layoutHandle = customLayout;
    }
    if (!layoutHandle)
        return nullptr;

    nvrhi::GraphicsPipelineDesc pipeDesc;
    pipeDesc.VS = vsResult.handle;
    pipeDesc.PS = psResult.handle;
    pipeDesc.inputLayout = inputLayout;
    pipeDesc.primType = nvrhi::PrimitiveType::TriangleList;

    if (bindlessLayout)
        pipeDesc.bindingLayouts = { layoutHandle, bindlessLayout };
    else
        pipeDesc.bindingLayouts = { layoutHandle };

    pipeDesc.renderState.depthStencilState = pass.depthStencil;
    pipeDesc.renderState.rasterState = pass.rasterState;
    pipeDesc.renderState.rasterState.frontCounterClockwise = false;
    if (variant.wmark)
    {
        pipeDesc.renderState.rasterState.depthBias = 16;
        pipeDesc.renderState.rasterState.slopeScaledDepthBias = 2.f;
    }

    if (pass.blendEnabled && !(remappedToSkinned && !variant.transparent))
    {
        pipeDesc.renderState.blendState.targets[0] = pass.blendRT;
        pipeDesc.renderState.blendState.alphaToCoverageEnable = pass.alphaToCoverage;
        if (variant.wmark)
            pipeDesc.renderState.blendState.targets[0].setColorWriteMask(
                nvrhi::ColorMask::Red | nvrhi::ColorMask::Green | nvrhi::ColorMask::Blue);
    }

    auto pipeline = device->createGraphicsPipeline(pipeDesc, framebuffer);
    if (!pipeline)
    {
        Msg("! [VariantPSO] Failed to create pipeline for '%s' pass %d", variant.name.c_str(), passIndex);
        return nullptr;
    }

    Msg("* [VariantPSO] Created pipeline: '%s' pass=%d vs=%s ps=%s blend=%s skinnedRemap=%d",
        variant.name.c_str(), passIndex, vsName, psName,
        pass.blendEnabled ? "yes" : "no", remappedToSkinned ? 1 : 0);

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
    auto& smpCache = framegraph::GetPassResourceCache();

    const auto& p = cfg.partition;
    auto* shaderLoader = GEnv.Render->GetShaderLoader();
    if (!shaderLoader)
        return;

    for (u32 v = 0; v < p.variantCount; v++) {
        nvrhi::IGraphicsPipeline* pso;
        const char* vsName = "bindless_forward";
        const char* psName = "bindless_forward";
        nvrhi::IBindingLayout* layout = cfg.passLayout;

        if (v == 0) {
            pso = cfg.defaultPipeline;
        } else {
            const auto* variant = registry.GetVariantByIndex(v);
            if (!variant) continue;
            if (cfg.selectTransparent ? !variant->transparent : variant->transparent) continue;
            pso = psoCache.GetOrCreatePSO(nvDevice, framebuffer, v, *variant, 0, VF_MDI,
                cfg.inputLayout, cfg.passLayout, cfg.bindlessLayout);
            if (!pso) continue;

            if (!variant->passes.empty())
            {
                vsName = variant->passes[0].vsName.c_str();
                psName = variant->passes[0].psName.c_str();
            }
            // Prefer layout baked into the variant PSO (may differ from default forward).
            const auto& pipeDesc = pso->getDesc();
            if (!pipeDesc.bindingLayouts.empty())
                layout = pipeDesc.bindingLayouts[0];
        }

        nvrhi::ITexture* dummy2D = smpCache.GetDummyShadowMap2D(nvDevice);

        nvrhi::ITexture* sky0 = cfg.envSky0 ? cfg.envSky0 : smpCache.GetDummyCubeMap(nvDevice);
        nvrhi::ITexture* sky1 = cfg.envSky1 ? cfg.envSky1 : smpCache.GetDummyCubeMap(nvDevice);

        auto* vsReflection = shaderLoader->GetCachedReflection(vsName, ".vs");
        auto* psReflection = shaderLoader->GetCachedReflection(psName, ".ps");
        if (!vsReflection || !psReflection || !layout)
        {
            Msg("! [VariantPartition] Missing reflection/layout for %s/%s — skip variant %u",
                vsName, psName, v);
            continue;
        }

        framegraph::BindingSetBuilder bsb(*vsReflection, *psReflection, nvDevice, "VariantPartition");
        bsb.ConstantBuffer("static_globals", cfg.staticGlobalsCB);
        fg::passes::BindBindlessMaterialTables(bsb);
        bsb.BufferSRV("g_InstanceData", cfg.instanceBuffer);
        bsb.BufferSRV("g_CompactBatchIndices", p.batchIndicesBuffer);
        bsb.BufferSRV("g_CompactMaterialIDs", p.materialIDsBuffer);

        if (cfg.lightDataBuffer)
            bsb.BufferSRV("g_LightData", cfg.lightDataBuffer);
        if (cfg.shadowDataBuffer)
            bsb.BufferSRV("g_ShadowData", cfg.shadowDataBuffer);
        if (cfg.clusterGridBuffer)
            bsb.BufferSRV("g_ClusterGrid", cfg.clusterGridBuffer);
        if (cfg.lightIndexListBuffer)
            bsb.BufferSRV("g_LightIndexList", cfg.lightIndexListBuffer);

        if (xr_strcmp(psName, "water") != 0)
        {
            static const char* kNames[3] = {"g_ShadowMap0", "g_ShadowMap1", "g_ShadowMap2"};
            for (u32 i = 0; i < 3; ++i)
            {
                nvrhi::ITexture* t = cfg.shadowCascades[i] ? cfg.shadowCascades[i]
                    : (i == 0 && cfg.shadowMapArray ? cfg.shadowMapArray : dummy2D);
                if (t)
                    bsb.Texture(kNames[i], t);
            }
            nvrhi::ITexture* contactHist = cfg.contactHistory
                ? cfg.contactHistory
                : smpCache.GetDummyContactHistory(nvDevice);
            if (contactHist)
                bsb.Texture("g_ContactHistory", contactHist);
            nvrhi::ITexture* localAtlas = cfg.localShadowAtlas
                ? cfg.localShadowAtlas
                : smpCache.GetDummyShadowMap(nvDevice);
            if (localAtlas)
                bsb.Texture("g_LocalShadowAtlas", localAtlas);
            nvrhi::ITexture* localEsm = cfg.localShadowESM
                ? cfg.localShadowESM
                : smpCache.GetDummyLocalShadowESM(nvDevice);
            if (localEsm)
                bsb.Texture("g_LocalShadowESM", localEsm);
            static const char* kHzb[3] = {"g_ShadowHZB0", "g_ShadowHZB1", "g_ShadowHZB2"};
            nvrhi::ITexture* dummyHzb = smpCache.GetDummyContactDepth(nvDevice);
            for (u32 i = 0; i < 3; ++i)
            {
                nvrhi::ITexture* hz = cfg.shadowHZB[i] ? cfg.shadowHZB[i] : dummyHzb;
                if (hz)
                    bsb.Texture(kHzb[i], hz);
            }
            nvrhi::ITexture* shadowMask = cfg.shadowMask
                ? cfg.shadowMask
                : smpCache.GetDummyContactHistory(nvDevice);
            if (shadowMask)
                bsb.Texture("g_ShadowMask", shadowMask);
        }
        if (sky0)
            bsb.Texture("s_env0", sky0);
        if (sky1)
            bsb.Texture("s_env1", sky1);
        xray::render::fg::passes::BindIBLResources(bsb, xray::render::fg::passes::GetCurrentIBLBindResources(), nvDevice);

        auto bindingSet = smpCache.GetOrCreateBindingSet(bsb.Build(), layout, nvDevice);
        if (!bindingSet)
        {
            Msg("! [VariantPartition] createBindingSet failed for variant %u (%s/%s)",
                v, vsName, psName);
            continue;
        }

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
        fg::passes::DrawIndexedIndirectCountOrFallback(
            cmdList,
            v * p.binCapacity * sizeof(fg::IndirectDrawArgs),
            v * sizeof(u32),
            p.binCapacity);
    }

    cmdList->setBufferState(p.drawArgsBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(p.countBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(p.batchIndicesBuffer, nvrhi::ResourceStates::UnorderedAccess);
    cmdList->setBufferState(p.materialIDsBuffer, nvrhi::ResourceStates::UnorderedAccess);
}

} // namespace xray::render
