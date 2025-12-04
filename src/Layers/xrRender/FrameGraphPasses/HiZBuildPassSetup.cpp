#include "stdafx.h"
#include "HiZBuildPassSetup.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/PassResourceCache.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include <nvrhi/utils.h>

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/dx11HW.h"
#endif

namespace RENDER_NAMESPACE
{
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;

static ref_cs s_hiz_build_cs;

static nvrhi::BufferHandle s_hiz_cb;
static nvrhi::SamplerHandle s_point_sampler;

static nvrhi::ComputePipelineHandle s_hiz_pipeline;
static nvrhi::BindingLayoutHandle s_hiz_layout;

static bool s_initialized = false;
static bool s_compute_enabled = false;
static u32 s_current_width = 0;
static u32 s_current_height = 0;
static u32 s_mip_levels = 0;

struct HiZCB {
    u32 outputWidth;
    u32 outputHeight;
    u32 inputMipLevel;
    u32 isFirstMip;
};

static void InitializeHiZResources(ng::RenderDevice* device, u32 width, u32 height)
{
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice) {
        Msg("! [HiZBuild] NVRHI device not available");
        return;
    }

    bool needsReinit = !s_initialized ||
                       s_current_width != width ||
                       s_current_height != height;

    if (!needsReinit) {
        return;
    }

    if (!s_initialized) {
        s_hiz_build_cs.create("hiz_build");

        if (s_hiz_build_cs && s_hiz_build_cs->nvrhiShader) {
            Msg("* [HiZBuild] Loaded hiz_build compute shader: OK");
            s_compute_enabled = true;
        } else {
            Msg("! [HiZBuild] hiz_build.cs not found - Hi-Z culling disabled");
            s_compute_enabled = false;
        }

        nvrhi::BufferDesc cbDesc;
        cbDesc.debugName = "HiZCB";
        cbDesc.byteSize = sizeof(HiZCB);
        cbDesc.isConstantBuffer = true;
        cbDesc.isVolatile = true;
        cbDesc.maxVersions = 16;
        cbDesc.keepInitialState = true;
        cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
        s_hiz_cb = nvDevice->createBuffer(cbDesc);

        auto& cache = framegraph::GetPassResourceCache();

        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.minFilter = false;
        samplerDesc.magFilter = false;
        samplerDesc.mipFilter = false;
        samplerDesc.addressU = nvrhi::SamplerAddressMode::Clamp;
        samplerDesc.addressV = nvrhi::SamplerAddressMode::Clamp;
        samplerDesc.addressW = nvrhi::SamplerAddressMode::Clamp;
        s_point_sampler = cache.GetOrCreateSampler("HiZBuildPass", samplerDesc, nvDevice);

        if (s_compute_enabled) {
            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.visibility = nvrhi::ShaderType::Compute;
            layoutDesc.bindings = {
                nvrhi::BindingLayoutItem::ConstantBuffer(5),
                nvrhi::BindingLayoutItem::Texture_SRV(0),
                nvrhi::BindingLayoutItem::Texture_UAV(0),
                nvrhi::BindingLayoutItem::Sampler(0)
            };
            s_hiz_layout = cache.GetOrCreateBindingLayout("HiZBuildPass", layoutDesc, nvDevice);

            if (s_hiz_layout) {
                nvrhi::ComputePipelineDesc pipeDesc;
                pipeDesc.CS = s_hiz_build_cs->nvrhiShader;
                pipeDesc.bindingLayouts = { s_hiz_layout };
                s_hiz_pipeline = cache.GetOrCreateComputePipeline("HiZBuildPass", pipeDesc, nvDevice);

                if (s_hiz_pipeline) {
                    Msg("* [HiZBuild] Compute pipeline created successfully");
                } else {
                    Msg("! [HiZBuild] Failed to create compute pipeline");
                    s_compute_enabled = false;
                }
            } else {
                Msg("! [HiZBuild] Failed to create binding layout");
                s_compute_enabled = false;
            }
        }
    }

    s_current_width = width;
    s_current_height = height;
    s_mip_levels = CalculateHiZMipLevels(width, height);

    Msg("* [HiZBuild] Initialized: %dx%d, %d mips (texture managed by FrameGraph)", width, height, s_mip_levels);

    s_initialized = true;
}

HiZPyramidOutput setupHiZBuildPass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    VirtualResourceHandle depthInput,
    u32 width,
    u32 height)
{
    InitializeHiZResources(device, width, height);

    if (!s_compute_enabled) {
        Msg("! [HiZBuild] Compute disabled, returning invalid output");
        HiZPyramidOutput output;
        output.pyramid = VirtualResourceHandle();
        output.mipLevels = 0;
        output.width = width;
        output.height = height;
        return output;
    }

    struct HiZBuildData {
        VirtualResourceHandle depthInput;
        VirtualResourceHandle hizPyramid;

        ng::RenderDevice* device;
        u32 width;
        u32 height;
        u32 mipLevels;
    };

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

            data.device = device;
            data.width = hizWidth;
            data.height = hizHeight;
            data.mipLevels = hizMipLevels;

            data.depthInput = passBuilder.read(depthInput, ResourceState::ShaderResource);

            data.hizPyramid = passBuilder.write(hizHandle, ResourceState::UnorderedAccess);
        },

        [](const HiZBuildData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            if (!s_compute_enabled || !s_hiz_pipeline) {
                return;
            }

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->beginMarker("Hi-Z Build");

            nvrhi::IDevice* nvDevice = data.device->GetNVRHIDevice();

            nvrhi::ITexture* depthTexture = fg.GetPhysicalTexture(data.depthInput);
            nvrhi::ITexture* hizTexture = fg.GetPhysicalTexture(data.hizPyramid);

            if (!depthTexture) {
                Msg("! [HiZBuild] Depth texture not available");
                cmdList->endMarker();
                return;
            }

            if (!hizTexture) {
                Msg("! [HiZBuild] Hi-Z pyramid texture not available");
                cmdList->endMarker();
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

                cmdList->writeBuffer(s_hiz_cb, &cb, sizeof(cb));

                nvrhi::BindingSetDesc bindDesc;

                if (mip == 0) {
                    bindDesc.bindings.push_back(
                        nvrhi::BindingSetItem::Texture_SRV(0, depthTexture, nvrhi::Format::R32_FLOAT));
                } else {
                    nvrhi::TextureSubresourceSet inputSubres;
                    inputSubres.baseMipLevel = mip - 1;
                    inputSubres.numMipLevels = 1;
                    inputSubres.baseArraySlice = 0;
                    inputSubres.numArraySlices = 1;

                    bindDesc.bindings.push_back(
                        nvrhi::BindingSetItem::Texture_SRV(0, hizTexture, nvrhi::Format::R32_FLOAT, inputSubres));
                }

                nvrhi::TextureSubresourceSet outputSubres;
                outputSubres.baseMipLevel = mip;
                outputSubres.numMipLevels = 1;
                outputSubres.baseArraySlice = 0;
                outputSubres.numArraySlices = 1;

                bindDesc.bindings.push_back(
                    nvrhi::BindingSetItem::Texture_UAV(0, hizTexture, nvrhi::Format::R32_FLOAT, outputSubres));

                bindDesc.bindings.push_back(
                    nvrhi::BindingSetItem::ConstantBuffer(5, s_hiz_cb));
                bindDesc.bindings.push_back(
                    nvrhi::BindingSetItem::Sampler(0, s_point_sampler));

                nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, s_hiz_layout);

                if (!bindingSet) {
                    Msg("! [HiZBuild] Failed to create binding set for mip %d", mip);
                    continue;
                }

                nvrhi::ComputeState state;
                state.pipeline = s_hiz_pipeline;
                state.bindings = { bindingSet };
                cmdList->setComputeState(state);

                u32 groupsX = (outWidth + 7) / 8;
                u32 groupsY = (outHeight + 7) / 8;
                cmdList->dispatch(groupsX, groupsY, 1);

                nvrhi::utils::TextureUavBarrier(cmdList, hizTexture);

                cmdList->setTextureState(hizTexture, outputSubres,
                    nvrhi::ResourceStates::ShaderResource);
            }

            cmdList->setTextureState(hizTexture, nvrhi::AllSubresources,
                nvrhi::ResourceStates::ShaderResource);

            cmdList->endMarker();
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
