#include "stdafx.h"
#include "HiZBuildPassSetup.h"
#include "PassVertexFormats.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/BindingSetBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include <nvrhi/utils.h>

namespace fg
{
    extern xray::render::FrameGraphRenderer RImplementation;
}

namespace xray::render::fg::passes {

using namespace framegraph;

void InitializeHiZResources(fg::RenderDevice* device, u32 width, u32 height, HiZBuildPassState& state)
{
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice) {
        Msg("! [HiZBuild] NVRHI device not available");
        return;
    }

    bool needsReinit = !state.initialized ||
                       state.currentWidth != width ||
                       state.currentHeight != height;

    if (!needsReinit) {
        return;
    }

    if (!state.initialized) {
        auto csResult = GEnv.Render->GetShaderLoader()->LoadComputeShader("hiz_build");

        if (csResult.handle) {
            Msg("* [HiZBuild] Loaded hiz_build compute shader: OK");
            state.computeEnabled = true;
        } else {
            Msg("! [HiZBuild] hiz_build.cs not found - Hi-Z culling disabled");
            state.computeEnabled = false;
        }

        auto& cache = framegraph::GetPassResourceCache();

        if (state.computeEnabled) {
            state.layout = cache.GetOrCreateBindingLayoutFromReflection("HiZBuildPass", *csResult.reflection, nvDevice);

            if (state.layout) {
                nvrhi::ComputePipelineDesc pipeDesc;
                pipeDesc.CS = csResult.handle;
                pipeDesc.bindingLayouts = { state.layout };
                state.pipeline = cache.GetOrCreateComputePipeline("HiZBuildPass", pipeDesc, nvDevice);

                if (state.pipeline) {
                    Msg("* [HiZBuild] Compute pipeline created successfully");
                } else {
                    Msg("! [HiZBuild] Failed to create compute pipeline");
                    state.computeEnabled = false;
                }
            } else {
                Msg("! [HiZBuild] Failed to create binding layout");
                state.computeEnabled = false;
            }
        }
    }

    state.currentWidth = width;
    state.currentHeight = height;
    state.mipLevels = CalculateHiZMipLevels(width, height);

    Msg("* [HiZBuild] Initialized: %dx%d, %d mips (texture managed by FrameGraph)", width, height, state.mipLevels);

    state.initialized = true;
}

HiZPyramidOutput setupHiZBuildPass(
    FrameGraph& fg,
    fg::RenderDevice* device,
    VirtualResourceHandle depthInput,
    u32 width,
    u32 height,
    HiZBuildPassState& hizState)
{
    InitializeHiZResources(device, width, height, hizState);

    if (!hizState.computeEnabled) {
        Msg("! [HiZBuild] Compute disabled, returning invalid output");
        HiZPyramidOutput output;
        output.pyramid = VirtualResourceHandle();
        output.mipLevels = 0;
        output.width = width;
        output.height = height;
        return output;
    }

    u32 hizWidth = std::max(1u, width / 2);
    u32 hizHeight = std::max(1u, height / 2);
    u32 hizMipLevels = CalculateHiZMipLevels(hizWidth, hizHeight);

    ResourceDesc hizDesc;
    hizDesc.type = ResourceDesc::Type::Texture2D;
    hizDesc.debugName = "HiZPyramid";
    hizDesc.width = hizWidth;
    hizDesc.height = hizHeight;
    hizDesc.format = nvrhi::Format::R32_FLOAT;
    hizDesc.mipLevels = hizMipLevels;
    hizDesc.isUAV = true;
    hizDesc.isTransient = true;

    VirtualResourceHandle hizHandle = fg.CreateTexture("rt_HiZPyramid", hizDesc);

    auto& passData = fg.addCallbackPass<HiZBuildData>(
        "Hi-Z Build",

        [&, width, height, hizWidth, hizHeight, hizMipLevels](FrameGraph& builder, PassHandle passHandle, HiZBuildData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);
            passBuilder.asyncCompute();

            data.device = device;
            data.width = hizWidth;
            data.height = hizHeight;
            data.mipLevels = hizMipLevels;
            data.passState = &hizState;

            data.depthInput = passBuilder.read(depthInput, ResourceState::ShaderResource);
            data.hizPyramid = passBuilder.write(hizHandle, ResourceState::UnorderedAccess);
        },

        [](const HiZBuildData& data,
           const FrameGraph& fg,
           fg::RenderContext* ctx) {

            if (!data.passState->computeEnabled || !data.passState->pipeline) {
                return;
            }

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

            nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(data.depthInput);
            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);

            if (!depthTexture) {
                Msg("! [HiZBuild] Depth texture not available");
                return;
            }

            if (!hizTexture) {
                Msg("! [HiZBuild] Hi-Z pyramid texture not available");
                return;
            }

            auto hizCB = framegraph::GetPassResourceCache().GetOrCreateVolatileCB("HiZBuild", "HiZCB", sizeof(HiZCB), data.device, 64);
            if (!hizCB) {
                Msg("! [HiZBuild] Constant buffer is NULL at execute time!");
                return;
            }

            for (u32 mip = 0; mip < data.mipLevels; mip++) {
                u32 outWidth = std::max(1u, data.width >> mip);
                u32 outHeight = std::max(1u, data.height >> mip);

                if (outWidth == 0 || outHeight == 0) break;

                HiZCB cb;
                cb.outputWidth = outWidth;
                cb.outputHeight = outHeight;
                cb.inputMipLevel = 0;
                cb.isFirstMip = (mip == 0) ? 1 : 0;

                cmdList->writeBuffer(hizCB, &cb, sizeof(cb));

                nvrhi::TextureSubresourceSet inputSubres;
                inputSubres.baseMipLevel = (mip > 0) ? mip - 1 : 0;
                inputSubres.numMipLevels = 1;
                inputSubres.baseArraySlice = 0;
                inputSubres.numArraySlices = 1;

                nvrhi::TextureSubresourceSet outputSubres;
                outputSubres.baseMipLevel = mip;
                outputSubres.numMipLevels = 1;
                outputSubres.baseArraySlice = 0;
                outputSubres.numArraySlices = 1;

                auto* hizRefl = GEnv.Render->GetShaderLoader()->GetCachedReflection("hiz_build", ".cs");
                framegraph::BindingSetBuilder bsb(*hizRefl, nvDevice, "HiZBuild");
                if (mip == 0)
                    bsb.Texture("g_input_depth", depthTexture);
                else
                    bsb.Texture("g_input_depth", hizTexture, nvrhi::Format::R32_FLOAT, inputSubres);
                bsb.TextureUAV("g_output_hiz", hizTexture, nvrhi::Format::R32_FLOAT, outputSubres);
                bsb.ConstantBuffer("HiZParams", hizCB);

                nvrhi::BindingSetHandle bindingSet = framegraph::GetPassResourceCache().GetOrCreateBindingSet(bsb.Build(), data.passState->layout, nvDevice);

                if (!bindingSet) {
                    Msg("! [HiZBuild] Failed to create binding set for mip %d", mip);
                    continue;
                }

                nvrhi::ComputeState state;
                state.pipeline = data.passState->pipeline;
                state.bindings = { bindingSet };
                cmdList->setComputeState(state);

                u32 groupsX = (outWidth + 7) / 8;
                u32 groupsY = (outHeight + 7) / 8;
                cmdList->dispatch(groupsX, groupsY, 1);

                if (mip < data.mipLevels - 1) {
                    nvrhi::utils::TextureUavBarrier(cmdList, hizTexture);
                }
            }

            cmdList->setTextureState(hizTexture, nvrhi::AllSubresources,
                nvrhi::ResourceStates::ShaderResource);
        }
    );

    HiZPyramidOutput output;
    output.pyramid = passData.hizPyramid;
    output.mipLevels = hizMipLevels;
    output.width = hizWidth;
    output.height = hizHeight;
    return output;
}

}
