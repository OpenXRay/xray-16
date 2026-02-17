// xrRender/FrameGraphPasses/HiZBuildPassSetup.cpp
#include "stdafx.h"
#include "HiZBuildPassSetup.h"
#include "PassVertexFormats.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include <nvrhi/utils.h>  // For TextureUavBarrier

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/dx11HW.h"
#endif

namespace RENDER_NAMESPACE
{
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;

static void InitializeHiZResources(ng::RenderDevice* device, u32 width, u32 height, HiZBuildPassState& state)
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
        ref_cs hiz_build_cs;
        hiz_build_cs.create("hiz_build");

        if (hiz_build_cs && hiz_build_cs->nvrhiShader) {
            Msg("* [HiZBuild] Loaded hiz_build compute shader: OK");
            state.computeEnabled = true;
        } else {
            Msg("! [HiZBuild] hiz_build.cs not found - Hi-Z culling disabled");
            state.computeEnabled = false;
        }

        auto& cache = framegraph::GetPassResourceCache();

        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.minFilter = false;
        samplerDesc.magFilter = false;
        samplerDesc.mipFilter = false;
        samplerDesc.addressU = nvrhi::SamplerAddressMode::Clamp;
        samplerDesc.addressV = nvrhi::SamplerAddressMode::Clamp;
        samplerDesc.addressW = nvrhi::SamplerAddressMode::Clamp;
        state.pointSampler = cache.GetOrCreateSampler("HiZBuildPass", samplerDesc, nvDevice);

        if (state.computeEnabled) {
            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.visibility = nvrhi::ShaderType::Compute;
            layoutDesc.bindings = {
                nvrhi::BindingLayoutItem::Texture_SRV(0),
                nvrhi::BindingLayoutItem::Texture_UAV(0),
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),
                nvrhi::BindingLayoutItem::Sampler(0)
            };
            state.layout = cache.GetOrCreateBindingLayout("HiZBuildPass", layoutDesc, nvDevice);

            if (state.layout) {
                nvrhi::ComputePipelineDesc pipeDesc;
                pipeDesc.CS = hiz_build_cs->nvrhiShader;
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
    ng::RenderDevice* device,
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

    // Create Hi-Z pyramid resource in framegraph
    // Hi-Z starts at HALF resolution (mip 0 = half of depth buffer)
    // This is the standard approach - we don't need a full-res copy
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
    hizDesc.isTransient = true;  // Transient - will be aliased/reused

    VirtualResourceHandle hizHandle = fg.CreateTexture("rt_HiZPyramid", hizDesc);

    auto& passData = fg.addCallbackPass<HiZBuildData>(
        "Hi-Z Build",

        [&, width, height, hizWidth, hizHeight, hizMipLevels](FrameGraph& builder, PassHandle passHandle, HiZBuildData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.width = hizWidth;
            data.height = hizHeight;
            data.mipLevels = hizMipLevels;
            data.passState = &hizState;

            data.depthInput = passBuilder.read(depthInput, ResourceState::ShaderResource);
            data.hizPyramid = passBuilder.write(hizHandle, ResourceState::UnorderedAccess);
        },

        // Execute lambda
        [](const HiZBuildData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            if (!data.passState->computeEnabled || !data.passState->pipeline) {
                return;
            }

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

            // Get depth texture from framegraph
            nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(data.depthInput);
            // Get Hi-Z pyramid texture from framegraph
            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);

            if (!depthTexture) {
                Msg("! [HiZBuild] Depth texture not available");
                return;
            }

            if (!hizTexture) {
                Msg("! [HiZBuild] Hi-Z pyramid texture not available");
                return;
            }

            // ─────────────────────────────────────────────────────
            // BUILD MIP CHAIN
            // ─────────────────────────────────────────────────────
            // For each mip level:
            //   - Mip 0: Read from full-res depth buffer
            //   - Mip N: Read from Hi-Z mip N-1
            //   - Take MAX of 2x2 block (conservative depth)

            // Hi-Z pyramid mip layout (HALF-RES BASE):
            // - Mip 0: Half resolution (first 2x2 downsample from depth)
            // - Mip 1: Quarter resolution
            // - Mip N: 1/(2^(N+1)) of original depth resolution
            //
            // For mip 0, we read from full-res depth and downsample 2x2 → 1
            // For mip N (N>0), we read from Hi-Z mip N-1 and downsample 2x2 → 1

            auto hizCB = framegraph::GetPassResourceCache().GetOrCreateVolatileCB("HiZBuild", "HiZCB", sizeof(HiZCB), 64, nvDevice);
            if (!hizCB) {
                Msg("! [HiZBuild] Constant buffer is NULL at execute time!");
                return;
            }

            for (u32 mip = 0; mip < data.mipLevels; mip++) {
                // Output dimensions for this mip level
                u32 outWidth = std::max(1u, data.width >> mip);
                u32 outHeight = std::max(1u, data.height >> mip);

                // Skip if output is 0 (shouldn't happen)
                if (outWidth == 0 || outHeight == 0) break;

                // Update constant buffer
                HiZCB cb;
                cb.outputWidth = outWidth;
                cb.outputHeight = outHeight;
                // NOTE: When binding a specific mip subresource, the shader sees it as mip 0
                // So inputMipLevel should always be 0 for the bound texture view
                cb.inputMipLevel = 0;  // Always 0 - we bind specific mip as subresource
                cb.isFirstMip = (mip == 0) ? 1 : 0;

                cmdList->writeBuffer(hizCB, &cb, sizeof(cb));

                // Create binding set for this mip level
                nvrhi::BindingSetDesc bindDesc;

                // Input texture
                if (mip == 0) {
                    // First mip reads from full-res depth buffer
                    // NOTE: D32 depth textures must be read as R32_FLOAT when used as SRV
                    bindDesc.bindings.push_back(
                        nvrhi::BindingSetItem::Texture_SRV(0, depthTexture, nvrhi::Format::R32_FLOAT));
                } else {
                    // Subsequent mips read from previous Hi-Z mip
                    nvrhi::TextureSubresourceSet inputSubres;
                    inputSubres.baseMipLevel = mip - 1;
                    inputSubres.numMipLevels = 1;
                    inputSubres.baseArraySlice = 0;
                    inputSubres.numArraySlices = 1;

                    bindDesc.bindings.push_back(
                        nvrhi::BindingSetItem::Texture_SRV(0, hizTexture, nvrhi::Format::R32_FLOAT, inputSubres));
                }

                // Output mip level as UAV
                nvrhi::TextureSubresourceSet outputSubres;
                outputSubres.baseMipLevel = mip;
                outputSubres.numMipLevels = 1;
                outputSubres.baseArraySlice = 0;
                outputSubres.numArraySlices = 1;

                bindDesc.bindings.push_back(
                    nvrhi::BindingSetItem::Texture_UAV(0, hizTexture, nvrhi::Format::R32_FLOAT, outputSubres));

                // Constant buffer and sampler
                bindDesc.bindings.push_back(
                    nvrhi::BindingSetItem::ConstantBuffer(5, hizCB));
                bindDesc.bindings.push_back(
                    nvrhi::BindingSetItem::Sampler(0, data.passState->pointSampler));

                nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, data.passState->layout);

                if (!bindingSet) {
                    Msg("! [HiZBuild] Failed to create binding set for mip %d", mip);
                    continue;
                }

                // Set compute state and dispatch
                nvrhi::ComputeState state;
                state.pipeline = data.passState->pipeline;
                state.bindings = { bindingSet };
                cmdList->setComputeState(state);

                u32 groupsX = (outWidth + 7) / 8;
                u32 groupsY = (outHeight + 7) / 8;
                cmdList->dispatch(groupsX, groupsY, 1);

                // Insert UAV barrier to ensure writes complete before next mip reads this one
                // This is sufficient - no need to also transition state since we use UAV barrier
                if (mip < data.mipLevels - 1) {
                    // Only need barrier if there's another mip that will read from this one
                    nvrhi::utils::TextureUavBarrier(cmdList, hizTexture);
                }
            }

            // Final state: transition entire pyramid to ShaderResource for culling pass
            cmdList->setTextureState(hizTexture, nvrhi::AllSubresources,
                nvrhi::ResourceStates::ShaderResource);
        }
    );

    HiZPyramidOutput output;
    output.pyramid = passData.hizPyramid;
    output.mipLevels = hizMipLevels;
    output.width = hizWidth;    // Hi-Z base resolution (half of depth)
    output.height = hizHeight;
    return output;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
