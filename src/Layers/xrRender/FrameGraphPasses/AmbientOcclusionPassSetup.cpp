#include "stdafx.h"
#include "AmbientOcclusionPassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/device.h"

namespace xray::render::fg::passes {

using namespace framegraph;

struct alignas(16) AOConstantsGPU
{
    Fvector4 ao_screen; // xy size, zw 1/size
    Fvector4 ao_params; // radius, bias, strength, quality
    Fvector4 ao_proj;   // gtao focal, frame, pad, pad
};
static_assert(sizeof(AOConstantsGPU) == 48, "AOConstantsGPU must be 48 bytes");

struct AOPassData
{
    VirtualResourceHandle sceneColor;
    VirtualResourceHandle depth;
    VirtualResourceHandle normal;
    VirtualResourceHandle worldPos;
    VirtualResourceHandle aoRaw;
    VirtualResourceHandle aoFiltered;
    VirtualResourceHandle gtaoPacked;
    VirtualResourceHandle output;
    u32 width = 0;
    u32 height = 0;
    u32 genWidth = 0;
    u32 genHeight = 0;
    u32 mode = 0;
    u32 quality = 0;
    bool doBlur = false;
    bool halfRes = false;
    AmbientOcclusionPassState* passState = nullptr;
};

static nvrhi::GraphicsPipelineHandle CreateFullscreenPS(
    const char* cacheName,
    nvrhi::IShader* vs,
    nvrhi::IShader* ps,
    nvrhi::IBindingLayout* layout,
    nvrhi::Format colorFormat,
    nvrhi::IDevice* device)
{
    if (!vs || !ps || !layout)
        return nullptr;

    nvrhi::GraphicsPipelineDesc desc;
    desc.setVertexShader(vs);
    desc.setPixelShader(ps);
    desc.addBindingLayout(layout);
    desc.setPrimType(nvrhi::PrimitiveType::TriangleList);
    desc.renderState.blendState.targets[0].setBlendEnable(false);
    desc.renderState.depthStencilState.setDepthTestEnable(false);
    desc.renderState.depthStencilState.setDepthWriteEnable(false);
    desc.renderState.rasterState.setCullMode(nvrhi::RasterCullMode::None);

    nvrhi::FramebufferInfoEx fbInfo;
    fbInfo.addColorFormat(colorFormat);
    return GetPassResourceCache().GetOrCreatePipeline(cacheName, desc, fbInfo, device);
}

void InitializeAmbientOcclusionPass(nvrhi::IDevice* device, AmbientOcclusionPassState& state)
{
    if (state.initialized || !device)
        return;

    auto* loader = GEnv.Render->GetShaderLoader();
    if (!loader)
    {
        state.initialized = true;
        return;
    }

    auto vs = loader->LoadVertexShader("fullscreen");
    auto ssaoPs = loader->LoadPixelShader("ao/ssao");
    auto hbaoPs = loader->LoadPixelShader("ao/hbao");
    auto hdaoPs = loader->LoadPixelShader("ao/hdao");
    auto gtaoRenderPs = loader->LoadPixelShader("ao/gtao_render");
    auto gtaoFilterPs = loader->LoadPixelShader("ao/gtao_filter");
    auto blurPs = loader->LoadPixelShader("ao/ao_blur");
    auto applyPs = loader->LoadPixelShader("ao/ao_apply");

    if (!vs.handle)
    {
        Msg("! [AO] Failed to load fullscreen VS");
        state.initialized = true;
        return;
    }

    auto& cache = GetPassResourceCache();

    if (ssaoPs.handle && ssaoPs.reflection)
    {
        state.genLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "AO_Gen", *vs.reflection, *ssaoPs.reflection, device);
        state.ssaoPipeline = CreateFullscreenPS(
            "AO_SSAO", vs.handle, ssaoPs.handle, state.genLayout, nvrhi::Format::R16_FLOAT, device);
    }
    if (hbaoPs.handle && hbaoPs.reflection && state.genLayout)
    {
        state.hbaoPipeline = CreateFullscreenPS(
            "AO_HBAO", vs.handle, hbaoPs.handle, state.genLayout, nvrhi::Format::R16_FLOAT, device);
    }
    else if (hbaoPs.handle && hbaoPs.reflection)
    {
        state.genLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "AO_Gen", *vs.reflection, *hbaoPs.reflection, device);
        state.hbaoPipeline = CreateFullscreenPS(
            "AO_HBAO", vs.handle, hbaoPs.handle, state.genLayout, nvrhi::Format::R16_FLOAT, device);
    }
    if (hdaoPs.handle && hdaoPs.reflection)
    {
        if (!state.genLayout)
            state.genLayout = cache.GetOrCreateBindingLayoutFromReflection(
                "AO_Gen", *vs.reflection, *hdaoPs.reflection, device);
        state.hdaoPipeline = CreateFullscreenPS(
            "AO_HDAO", vs.handle, hdaoPs.handle, state.genLayout, nvrhi::Format::R16_FLOAT, device);
    }

    if (gtaoRenderPs.handle && gtaoRenderPs.reflection)
    {
        state.gtaoRenderLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "AO_GTAO_Render", *vs.reflection, *gtaoRenderPs.reflection, device);
        state.gtaoRenderPipeline = CreateFullscreenPS(
            "AO_GTAO_Render", vs.handle, gtaoRenderPs.handle, state.gtaoRenderLayout,
            nvrhi::Format::R32_UINT, device);
    }
    if (gtaoFilterPs.handle && gtaoFilterPs.reflection)
    {
        state.gtaoFilterLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "AO_GTAO_Filter_v2", *vs.reflection, *gtaoFilterPs.reflection, device);
        state.gtaoFilterPipeline = CreateFullscreenPS(
            "AO_GTAO_Filter_v2", vs.handle, gtaoFilterPs.handle, state.gtaoFilterLayout,
            nvrhi::Format::R16_FLOAT, device);
    }
    if (blurPs.handle && blurPs.reflection)
    {
        state.blurLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "AO_Blur", *vs.reflection, *blurPs.reflection, device);
        state.blurPipeline = CreateFullscreenPS(
            "AO_Blur", vs.handle, blurPs.handle, state.blurLayout, nvrhi::Format::R16_FLOAT, device);
    }
    if (applyPs.handle && applyPs.reflection)
    {
        state.applyLayout = cache.GetOrCreateBindingLayoutFromReflection(
            "AO_Apply_v2", *vs.reflection, *applyPs.reflection, device);
        state.applyPipeline = CreateFullscreenPS(
            "AO_Apply_v2", vs.handle, applyPs.handle, state.applyLayout, nvrhi::Format::RGBA16_FLOAT, device);
    }

    state.initialized = true;
    Msg("* [AO] Pass initialized (ssao=%d hbao=%d hdao=%d gtao=%d apply=%d)",
        state.ssaoPipeline ? 1 : 0,
        state.hbaoPipeline ? 1 : 0,
        state.hdaoPipeline ? 1 : 0,
        state.gtaoRenderPipeline ? 1 : 0,
        state.applyPipeline ? 1 : 0);
}

static AOConstantsGPU MakeAOConstants(u32 width, u32 height, u32 quality)
{
    AOConstantsGPU c{};
    const float w = static_cast<float>(width);
    const float h = static_cast<float>(height);
    c.ao_screen.set(w, h, 1.0f / w, 1.0f / h);

    const float q = static_cast<float>(std::max(1u, quality));
    // Per-mode radii live in shaders (classic r3 / IX-Ray). CB keeps quality + unused slots.
    const float radius = 1.0f; // unused by classic HDAO/HBAO/GTAO; SSAO uses fixed kernel
    const float bias = 0.0f;   // classic HBAO g_AngleBias
    const float strength = 1.0f; // apply uses fixed ambient weight; modes bake their own contrast
    c.ao_params.set(radius, bias, strength, q);

    // IX-Ray gtao_parameters = focal * 0.5; SSAO/HBAO use full focal via WorldRadiusToUV when needed
    const float focal = h / (2.0f * std::tan(Device.fFOV * 0.5f * (3.14159265f / 180.0f)));
    c.ao_proj.set(focal * 0.5f, static_cast<float>(Device.dwFrame), 0.f, 0.f);
    return c;
}

static void DrawFullscreen(
    nvrhi::ICommandList* cmdList,
    nvrhi::IDevice* device,
    nvrhi::IGraphicsPipeline* pipeline,
    nvrhi::IBindingSet* bindingSet,
    nvrhi::ITexture* outputTex,
    u32 width,
    u32 height,
    const char* fbName)
{
    if (!pipeline || !bindingSet || !outputTex)
        return;

    nvrhi::FramebufferDesc fbDesc;
    fbDesc.addColorAttachment(outputTex);
    auto framebuffer = GetPassResourceCache().GetOrCreateFramebuffer(fbName, fbDesc, device);

    nvrhi::Viewport viewport;
    viewport.minX = 0;
    viewport.minY = 0;
    viewport.maxX = static_cast<float>(width);
    viewport.maxY = static_cast<float>(height);
    viewport.minZ = 0.0f;
    viewport.maxZ = 1.0f;

    nvrhi::GraphicsState state;
    state.pipeline = pipeline;
    state.framebuffer = framebuffer;
    state.viewport.addViewportAndScissorRect(viewport);
    state.addBindingSet(bindingSet);
    cmdList->setGraphicsState(state);
    cmdList->draw(nvrhi::DrawArguments().setVertexCount(3));
}

VirtualResourceHandle setupAmbientOcclusionPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle sceneColor,
    VirtualResourceHandle depth,
    VirtualResourceHandle normal,
    VirtualResourceHandle worldPos,
    u32 width,
    u32 height,
    AmbientOcclusionPassState& passState)
{
    if (device && device->GetNVRHIDevice())
        InitializeAmbientOcclusionPass(device->GetNVRHIDevice(), passState);

    const u32 mode = ps_r_ssao_mode;
    const u32 quality = ps_r_ssao;
    if (mode == ssao_mode_off || quality == 0 || !passState.applyPipeline)
        return sceneColor;

    const bool isGtao = (mode == ssao_mode_gtao);
    const bool isHbao = (mode == ssao_mode_hbao);
    const bool isHdao = (mode == ssao_mode_hdao);
    nvrhi::IGraphicsPipeline* genPipe = nullptr;
    if (isGtao)
        genPipe = passState.gtaoRenderPipeline;
    else if (isHbao)
        genPipe = passState.hbaoPipeline;
    else if (isHdao)
        genPipe = passState.hdaoPipeline;
    else
        genPipe = passState.ssaoPipeline;

    if (!genPipe)
        return sceneColor;

    const bool doBlur = !isGtao && (ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_BLUR) || quality >= 2);
    // GTAO always half-res gen+filter (apply bilinear-upsamples). Other modes honor r2_ssao_half_data.
    const bool halfRes = isGtao || ps_r2_ls_flags_ext.test(R2FLAGEXT_SSAO_HALF_DATA);
    const u32 genW = halfRes ? std::max(1u, (width + 1) / 2) : width;
    const u32 genH = halfRes ? std::max(1u, (height + 1) / 2) : height;

    ResourceDesc aoDesc;
    aoDesc.type = ResourceDesc::Type::Texture2D;
    aoDesc.width = genW;
    aoDesc.height = genH;
    aoDesc.format = nvrhi::Format::R16_FLOAT;
    aoDesc.isRenderTarget = true;
    aoDesc.isTransient = true;
    aoDesc.debugName = "rt_AO";
    auto aoRaw = fg.CreateTexture("rt_AO", aoDesc);

    VirtualResourceHandle aoFiltered = aoRaw;
    if (doBlur || isGtao)
    {
        ResourceDesc filtDesc = aoDesc;
        filtDesc.debugName = "rt_AO_Filtered";
        aoFiltered = fg.CreateTexture("rt_AO_Filtered", filtDesc);
    }

    VirtualResourceHandle gtaoPacked;
    if (isGtao)
    {
        ResourceDesc packedDesc;
        packedDesc.type = ResourceDesc::Type::Texture2D;
        packedDesc.width = genW;
        packedDesc.height = genH;
        packedDesc.format = nvrhi::Format::R32_UINT;
        packedDesc.isRenderTarget = true;
        packedDesc.isTransient = true;
        packedDesc.debugName = "rt_GTAO_Packed";
        gtaoPacked = fg.CreateTexture("rt_GTAO_Packed", packedDesc);
    }

    ResourceDesc outDesc;
    outDesc.type = ResourceDesc::Type::Texture2D;
    outDesc.width = width;
    outDesc.height = height;
    outDesc.format = nvrhi::Format::RGBA16_FLOAT;
    outDesc.isRenderTarget = true;
    outDesc.isUAV = true;
    outDesc.isTransient = true;
    outDesc.debugName = "rt_AO_Applied";
    auto output = fg.CreateTexture("rt_AO_Applied", outDesc);

    auto& passData = fg.addCallbackPass<AOPassData>(
        "AmbientOcclusion",
        [=, &passState](FrameGraph& builder, PassHandle passHandle, AOPassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            data.width = width;
            data.height = height;
            data.genWidth = genW;
            data.genHeight = genH;
            data.mode = mode;
            data.quality = quality;
            data.doBlur = doBlur;
            data.halfRes = halfRes;
            data.passState = &passState;
            data.sceneColor = passBuilder.read(sceneColor, ResourceState::ShaderResource);
            data.depth = passBuilder.read(depth, ResourceState::ShaderResource);
            data.normal = passBuilder.read(normal, ResourceState::ShaderResource);
            data.worldPos = passBuilder.read(worldPos, ResourceState::ShaderResource);
            if (isGtao)
            {
                data.gtaoPacked = passBuilder.write(gtaoPacked, ResourceState::RenderTarget);
                data.aoFiltered = passBuilder.write(aoFiltered, ResourceState::RenderTarget);
            }
            else
            {
                data.aoRaw = passBuilder.write(aoRaw, ResourceState::RenderTarget);
                if (doBlur)
                    data.aoFiltered = passBuilder.write(aoFiltered, ResourceState::RenderTarget);
                else
                    data.aoFiltered = data.aoRaw;
            }
            data.output = passBuilder.write(output, ResourceState::RenderTarget);
        },
        [](const AOPassData& data, const FrameGraph& fg, fg::RenderContext* ctx) {
            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            nvrhi::IDevice* nvDevice = cmdList->getDevice();
            auto* ps = data.passState;
            if (!ps || !ps->applyPipeline)
                return;

            auto* colorTex = fg.GetPhysicalTexture(data.sceneColor);
            auto* depthTex = fg.GetPhysicalTexture(data.depth);
            auto* normalTex = fg.GetPhysicalTexture(data.normal);
            auto* worldPosTex = fg.GetPhysicalTexture(data.worldPos);
            auto* outputTex = fg.GetPhysicalTexture(data.output);
            if (!colorTex || !depthTex || !normalTex || !worldPosTex || !outputTex)
                return;

            auto& cache = GetPassResourceCache();
            auto staticGlobalsCB = cache.GetOrCreateVolatileCB(
                "Frame", "StaticGlobals", sizeof(StaticGlobals), ctx->GetDevice());
            auto aoCB = cache.GetOrCreateVolatileCB(
                "AO", "AOConstants", sizeof(AOConstantsGPU), ctx->GetDevice());

            // Constants match gen resolution (half or full); apply upsamples via linear sampler.
            AOConstantsGPU aoConsts = MakeAOConstants(data.genWidth, data.genHeight, data.quality);
            cmdList->writeBuffer(aoCB, &aoConsts, sizeof(aoConsts));

            auto* loader = GEnv.Render->GetShaderLoader();
            auto* vsRefl = loader->GetCachedReflection("fullscreen", ".vs");
            if (!vsRefl)
                return;

            nvrhi::ITexture* aoForApply = nullptr;
            const bool isGtao = (data.mode == ssao_mode_gtao);
            const bool isHbao = (data.mode == ssao_mode_hbao);
            const bool isHdao = (data.mode == ssao_mode_hdao);
            const u32 gw = data.genWidth;
            const u32 gh = data.genHeight;

            if (isGtao)
            {
                auto* packedTex = fg.GetPhysicalTexture(data.gtaoPacked);
                auto* filteredTex = fg.GetPhysicalTexture(data.aoFiltered);
                if (!packedTex || !filteredTex || !ps->gtaoRenderPipeline || !ps->gtaoFilterPipeline)
                    return;

                auto* gtaoRefl = loader->GetCachedReflection("ao/gtao_render", ".ps");
                if (!gtaoRefl)
                    return;
                BindingSetBuilder genBsb(*vsRefl, *gtaoRefl, nvDevice, "AO.GTAORender");
                genBsb.ConstantBuffer("static_globals", staticGlobalsCB)
                    .ConstantBuffer("AOConstants", aoCB)
                    .Texture("g_Depth", depthTex)
                    .Texture("g_Normal", normalTex)
                    .Texture("g_WorldPos", worldPosTex);
                auto genSet = cache.GetOrCreateBindingSet(genBsb.Build(), ps->gtaoRenderLayout, nvDevice);
                DrawFullscreen(cmdList, nvDevice, ps->gtaoRenderPipeline, genSet, packedTex,
                    gw, gh, data.halfRes ? "AO_GTAO_Render_Half" : "AO_GTAO_Render");

                cmdList->setTextureState(packedTex, nvrhi::AllSubresources,
                    nvrhi::ResourceStates::ShaderResource);

                auto* filtRefl = loader->GetCachedReflection("ao/gtao_filter", ".ps");
                if (!filtRefl)
                    return;
                BindingSetBuilder filtBsb(*vsRefl, *filtRefl, nvDevice, "AO.GTAOFilter");
                filtBsb.Texture("t_gtao_packed", packedTex);
                auto filtSet = cache.GetOrCreateBindingSet(filtBsb.Build(), ps->gtaoFilterLayout, nvDevice);
                DrawFullscreen(cmdList, nvDevice, ps->gtaoFilterPipeline, filtSet, filteredTex,
                    gw, gh, data.halfRes ? "AO_GTAO_Filter_Half" : "AO_GTAO_Filter");

                cmdList->setTextureState(filteredTex, nvrhi::AllSubresources,
                    nvrhi::ResourceStates::ShaderResource);
                aoForApply = filteredTex;
            }
            else
            {
                auto* rawTex = fg.GetPhysicalTexture(data.aoRaw);
                if (!rawTex)
                    return;

                const char* genShader =
                    isHbao ? "ao/hbao" : (isHdao ? "ao/hdao" : "ao/ssao");
                nvrhi::IGraphicsPipeline* genPipe =
                    isHbao ? ps->hbaoPipeline.Get() : (isHdao ? ps->hdaoPipeline.Get() : ps->ssaoPipeline.Get());
                if (!genPipe || !ps->genLayout)
                    return;

                auto* genRefl = loader->GetCachedReflection(genShader, ".ps");
                if (!genRefl)
                    return;
                BindingSetBuilder genBsb(*vsRefl, *genRefl, nvDevice, "AO.Gen");
                genBsb.ConstantBuffer("static_globals", staticGlobalsCB)
                    .ConstantBuffer("AOConstants", aoCB)
                    .Texture("g_Depth", depthTex)
                    .Texture("g_Normal", normalTex)
                    .Texture("g_WorldPos", worldPosTex);
                auto genSet = cache.GetOrCreateBindingSet(genBsb.Build(), ps->genLayout, nvDevice);
                DrawFullscreen(cmdList, nvDevice, genPipe, genSet, rawTex,
                    gw, gh, data.halfRes ? "AO_Gen_Half" : "AO_Gen");

                cmdList->setTextureState(rawTex, nvrhi::AllSubresources,
                    nvrhi::ResourceStates::ShaderResource);

                if (data.doBlur && ps->blurPipeline && ps->blurLayout)
                {
                    auto* filteredTex = fg.GetPhysicalTexture(data.aoFiltered);
                    if (!filteredTex)
                        return;
                    auto* blurRefl = loader->GetCachedReflection("ao/ao_blur", ".ps");
                    if (!blurRefl)
                        return;
                    BindingSetBuilder blurBsb(*vsRefl, *blurRefl, nvDevice, "AO.Blur");
                    blurBsb.ConstantBuffer("AOConstants", aoCB)
                        .Texture("g_AO", rawTex)
                        .Texture("g_Depth", depthTex);
                    auto blurSet = cache.GetOrCreateBindingSet(blurBsb.Build(), ps->blurLayout, nvDevice);
                    DrawFullscreen(cmdList, nvDevice, ps->blurPipeline, blurSet, filteredTex,
                        gw, gh, data.halfRes ? "AO_Blur_Half" : "AO_Blur");
                    cmdList->setTextureState(filteredTex, nvrhi::AllSubresources,
                        nvrhi::ResourceStates::ShaderResource);
                    aoForApply = filteredTex;
                }
                else
                {
                    aoForApply = rawTex;
                }
            }

            if (!aoForApply)
                return;

            auto* applyRefl = loader->GetCachedReflection("ao/ao_apply", ".ps");
            if (!applyRefl)
                return;
            BindingSetBuilder applyBsb(*vsRefl, *applyRefl, nvDevice, "AO.Apply");
            applyBsb.Texture("g_Color", colorTex)
                .Texture("g_AO", aoForApply)
                .Texture("g_Depth", depthTex);
            auto applySet = cache.GetOrCreateBindingSet(applyBsb.Build(), ps->applyLayout, nvDevice);
            DrawFullscreen(cmdList, nvDevice, ps->applyPipeline, applySet, outputTex,
                data.width, data.height, "AO_Apply");
        });

    return passData.output;
}

} // namespace xray::render::fg::passes
