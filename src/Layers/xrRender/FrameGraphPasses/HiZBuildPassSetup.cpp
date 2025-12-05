// xrRender/FrameGraphPasses/HiZBuildPassSetup.cpp
#include "stdafx.h"
#include "HiZBuildPassSetup.h"
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

// ═══════════════════════════════════════════════════════
//  STATIC STATE
// ═══════════════════════════════════════════════════════

// Compute shader (X-Ray format)
static ref_cs s_hiz_build_cs;

// NVRHI Resources (persistent across frames)
static nvrhi::BufferHandle s_hiz_cb;              // Constant buffer
static nvrhi::SamplerHandle s_point_sampler;      // Point sampler for exact texel reads
// NOTE: Hi-Z pyramid texture is managed by FrameGraph, not static

// Compute pipeline
static nvrhi::ComputePipelineHandle s_hiz_pipeline;
static nvrhi::BindingLayoutHandle s_hiz_layout;

static bool s_initialized = false;
static bool s_compute_enabled = false;
static u32 s_current_width = 0;
static u32 s_current_height = 0;
static u32 s_mip_levels = 0;

// ═══════════════════════════════════════════════════════
//  CONSTANT BUFFER STRUCTURE (must match HLSL)
// ═══════════════════════════════════════════════════════

struct HiZCB {
    u32 outputWidth;
    u32 outputHeight;
    u32 inputMipLevel;
    u32 isFirstMip;
};

// ═══════════════════════════════════════════════════════
//  INITIALIZATION
// ═══════════════════════════════════════════════════════

static void InitializeHiZResources(ng::RenderDevice* device, u32 width, u32 height)
{
    nvrhi::IDevice* nvDevice = device->GetNVRHIDevice();
    if (!nvDevice) {
        Msg("! [HiZBuild] NVRHI device not available");
        return;
    }

    // Check if we need to reinitialize (resolution changed)
    bool needsReinit = !s_initialized ||
                       s_current_width != width ||
                       s_current_height != height;

    if (!needsReinit) {
        return;
    }

    // ─────────────────────────────────────────────────────
    // Load compute shader (only once)
    // ─────────────────────────────────────────────────────
    if (!s_initialized) {
        s_hiz_build_cs.create("hiz_build");

        if (s_hiz_build_cs && s_hiz_build_cs->nvrhiShader) {
            Msg("* [HiZBuild] Loaded hiz_build compute shader: OK");
            s_compute_enabled = true;
        } else {
            Msg("! [HiZBuild] hiz_build.cs not found - Hi-Z culling disabled");
            s_compute_enabled = false;
        }

        // Create constant buffer (persistent)
        nvrhi::BufferDesc cbDesc;
        cbDesc.debugName = "HiZCB";
        cbDesc.byteSize = sizeof(HiZCB);
        cbDesc.isConstantBuffer = true;
        cbDesc.isVolatile = true;
        cbDesc.maxVersions = 16;
        cbDesc.keepInitialState = true;
        cbDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;
        s_hiz_cb = nvDevice->createBuffer(cbDesc);

        if (s_hiz_cb) {
            Msg("* [HiZBuild] Constant buffer created: %p, size=%d", s_hiz_cb.Get(), sizeof(HiZCB));
        } else {
            Msg("! [HiZBuild] Failed to create constant buffer!");
        }

        // Create point sampler (cached)
        auto& cache = framegraph::GetPassResourceCache();

        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.minFilter = false;  // Point filtering
        samplerDesc.magFilter = false;  // Point filtering
        samplerDesc.mipFilter = false;  // Point filtering
        samplerDesc.addressU = nvrhi::SamplerAddressMode::Clamp;
        samplerDesc.addressV = nvrhi::SamplerAddressMode::Clamp;
        samplerDesc.addressW = nvrhi::SamplerAddressMode::Clamp;
        s_point_sampler = cache.GetOrCreateSampler("HiZBuildPass", samplerDesc, nvDevice);

        // Create binding layout (cached)
        if (s_compute_enabled) {
            nvrhi::BindingLayoutDesc layoutDesc;
            layoutDesc.visibility = nvrhi::ShaderType::Compute;
            // NOTE: Order must match binding set push order (SRV, UAV, CB, Sampler)
            // Use VolatileConstantBuffer since the buffer is created with isVolatile=true
            layoutDesc.bindings = {
                nvrhi::BindingLayoutItem::Texture_SRV(0),              // t0: g_input_depth
                nvrhi::BindingLayoutItem::Texture_UAV(0),              // u0: g_output_hiz
                nvrhi::BindingLayoutItem::VolatileConstantBuffer(5),   // b5: HiZParams (volatile)
                nvrhi::BindingLayoutItem::Sampler(0)                   // s0: g_point_sampler
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

    // ─────────────────────────────────────────────────────
    // Store resolution info (texture managed by FrameGraph)
    // ─────────────────────────────────────────────────────
    s_current_width = width;
    s_current_height = height;
    s_mip_levels = CalculateHiZMipLevels(width, height);

    Msg("* [HiZBuild] Initialized: %dx%d, %d mips (texture managed by FrameGraph)", width, height, s_mip_levels);

    s_initialized = true;
}

// ═══════════════════════════════════════════════════════
//  SETUP HI-Z BUILD PASS
// ═══════════════════════════════════════════════════════

HiZPyramidOutput setupHiZBuildPass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    VirtualResourceHandle depthInput,
    u32 width,
    u32 height)
{
    // Initialize/update resources
    InitializeHiZResources(device, width, height);

    // Early out if compute not available
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

        // Setup lambda
        [&, width, height, hizWidth, hizHeight, hizMipLevels](FrameGraph& builder, PassHandle passHandle, HiZBuildData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.device = device;
            data.width = hizWidth;      // Hi-Z base resolution (half of depth)
            data.height = hizHeight;
            data.mipLevels = hizMipLevels;

            // Read depth buffer from depth prepass
            data.depthInput = passBuilder.read(depthInput, ResourceState::ShaderResource);

            // Write Hi-Z pyramid (all mip levels)
            data.hizPyramid = passBuilder.write(hizHandle, ResourceState::UnorderedAccess);
        },

        // Execute lambda
        [](const HiZBuildData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            if (!s_compute_enabled || !s_hiz_pipeline) {
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

            // Verify constant buffer is valid before dispatch loop
            if (!s_hiz_cb) {
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

                cmdList->writeBuffer(s_hiz_cb, &cb, sizeof(cb));

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
                    nvrhi::BindingSetItem::ConstantBuffer(5, s_hiz_cb));
                bindDesc.bindings.push_back(
                    nvrhi::BindingSetItem::Sampler(0, s_point_sampler));

                nvrhi::BindingSetHandle bindingSet = nvDevice->createBindingSet(bindDesc, s_hiz_layout);

                if (!bindingSet) {
                    Msg("! [HiZBuild] Failed to create binding set for mip %d", mip);
                    continue;
                }

                // Set compute state and dispatch
                nvrhi::ComputeState state;
                state.pipeline = s_hiz_pipeline;
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
