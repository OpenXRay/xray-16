#include "stdafx.h"
#include "GBufferPass.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"

namespace xray::render::passes {

using namespace framegraph;

GBufferPass::GBufferPass(ng::RenderDevice* device, const GBufferPassConfig& config)
    : m_device(device)
    , m_config(config)
{
    VERIFY(m_device != nullptr);

    if (config.width == 0 || config.height == 0) {
        Msg("! [GBufferPass] ERROR: Invalid resolution %ux%u! Must set config.width/height to Device.dwWidth/dwHeight!",
            config.width, config.height);
        FATAL("GBufferPass created with invalid resolution");
    }

    Msg("* [GBufferPass] Created (%ux%u)", config.width, config.height);

    m_materialCache = xr_make_unique<MaterialCache>(device);

    if (!LoadShaders())
    {
        Msg("! [GBufferPass] Failed to load shaders");
    }
}

GBufferPass::~GBufferPass() {
    Msg("* [GBufferPass] Destroyed");
}

bool GBufferPass::LoadShaders()
{
    ShaderLoader loader(m_device);

    m_vertexShaderNative = loader.LoadVertexShader("gbuffer");
    if (!m_vertexShaderNative)
    {
        Msg("! [GBufferPass] Failed to load gbuffer vertex shader");
        return false;
    }

    m_pixelShaderNative = loader.LoadPixelShader("gbuffer");
    if (!m_pixelShaderNative)
    {
        Msg("! [GBufferPass] Failed to load gbuffer pixel shader");
        return false;
    }

    m_vertexShader = xr_make_unique<ng::RCShader>(
        ng::ShaderStage::Vertex,
        m_vertexShaderNative,
        "gbuffer.vs"
    );

    m_pixelShader = xr_make_unique<ng::RCShader>(
        ng::ShaderStage::Pixel,
        m_pixelShaderNative,
        "gbuffer.ps"
    );

    struct PerObjectConstants {
        Fmatrix worldViewProj;
        Fmatrix world;
        Fmatrix worldIT;
        u8 padding[64];
    };

    ng::RenderDevice::BufferDesc cbDesc;
    cbDesc.byteSize = 256;
    cbDesc.isConstantBuffer = true;
    cbDesc.isVolatile = true;
    cbDesc.debugName = "PerObjectCB";

    m_perObjectCB = m_device->CreateBuffer(cbDesc);
    if (!m_perObjectCB.IsValid()) {
        Msg("! [GBufferPass] Failed to create per-object constant buffer");
        return false;
    }

    Msg("  ✓ G-Buffer shaders loaded successfully");
    return true;
}

bool GBufferPass::CreatePipeline(const GBufferOutputs& outputs, const FrameGraph& fg)
{
    VERIFY(m_vertexShader != nullptr);
    VERIFY(m_pixelShader != nullptr);

    ng::PipelineStateDesc psoDesc;

    psoDesc.vertexShader = m_vertexShader.get();
    psoDesc.pixelShader = m_pixelShader.get();

    psoDesc.vertexAttributes = {
        { "POSITION", 0, nvrhi::Format::RGB32_FLOAT,  0, 0, false },
        { "NORMAL",   0, nvrhi::Format::RGB32_FLOAT, 12, 0, false },
        { "TEXCOORD", 0, nvrhi::Format::RG32_FLOAT,  24, 0, false },
        { "TANGENT",  0, nvrhi::Format::RGB32_FLOAT, 32, 0, false },
        { "BINORMAL", 0, nvrhi::Format::RGB32_FLOAT, 44, 0, false },
    };

    psoDesc.primitiveTopology = ng::PrimitiveTopology::TriangleList;

    psoDesc.rasterizerState.cullMode = ng::CullMode::Back;
    psoDesc.rasterizerState.fillMode = ng::FillMode::Solid;
    psoDesc.rasterizerState.frontCounterClockwise = false;

    for (int i = 0; i < 3; ++i)
    {
        psoDesc.blendState.renderTargets[i].blendEnable = false;
        psoDesc.blendState.renderTargets[i].writeMask = ng::ColorWriteMask::All;
    }

    psoDesc.depthStencilState.depthTestEnable = true;
    psoDesc.depthStencilState.depthWriteEnable = true;
    psoDesc.depthStencilState.depthFunc = ng::ComparisonFunc::Less;
    psoDesc.depthStencilState.stencilEnable = false;

    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(outputs.albedo);
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(outputs.normal);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(outputs.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(outputs.depth);

    psoDesc.renderTargetFormats[0] = albedo->getDesc().format;
    psoDesc.renderTargetFormats[1] = normal->getDesc().format;
    psoDesc.renderTargetFormats[2] = material->getDesc().format;
    psoDesc.depthStencilFormat = depth->getDesc().format;
    psoDesc.renderTargetCount = 3;

    psoDesc.debugName = "GBufferPass";

    m_pipeline = m_device->GetPipelineCache()->GetOrCreate(psoDesc);

    if (!m_pipeline)
    {
        Msg("! [GBufferPass] Failed to create graphics pipeline");
        return false;
    }

    Msg("  ✓ GBufferPass pipeline created successfully (MRT: 3 targets + depth)");
    return true;
}

GBufferOutputs GBufferPass::Setup(FrameGraph& fg) {
    Msg("~ [GBufferPass] Setting up in FrameGraph");

    GBufferOutputs outputs;

    ResourceDesc albedoDesc;
    albedoDesc.type = ResourceDesc::Type::Texture2D;
    albedoDesc.width = m_config.width;
    albedoDesc.height = m_config.height;
    albedoDesc.format = m_config.albedoFormat;
    albedoDesc.isRenderTarget = true;
    albedoDesc.isTransient = true;
    albedoDesc.debugName = "GBuffer.Albedo";

    outputs.albedo = fg.CreateTexture("GBuffer.Albedo", albedoDesc);

    ResourceDesc normalDesc;
    normalDesc.type = ResourceDesc::Type::Texture2D;
    normalDesc.width = m_config.width;
    normalDesc.height = m_config.height;
    normalDesc.format = m_config.normalFormat;
    normalDesc.isRenderTarget = true;
    normalDesc.isTransient = true;
    normalDesc.debugName = "GBuffer.Normal";

    outputs.normal = fg.CreateTexture("GBuffer.Normal", normalDesc);

    ResourceDesc materialDesc;
    materialDesc.type = ResourceDesc::Type::Texture2D;
    materialDesc.width = m_config.width;
    materialDesc.height = m_config.height;
    materialDesc.format = m_config.materialFormat;
    materialDesc.isRenderTarget = true;
    materialDesc.isTransient = true;
    materialDesc.debugName = "GBuffer.Material";

    outputs.material = fg.CreateTexture("GBuffer.Material", materialDesc);

    ResourceDesc depthDesc;
    depthDesc.type = ResourceDesc::Type::Texture2D;
    depthDesc.width = m_config.width;
    depthDesc.height = m_config.height;
    depthDesc.format = m_config.depthFormat;
    depthDesc.isDepthStencil = true;
    depthDesc.isTransient = true;
    depthDesc.debugName = "GBuffer.Depth";

    outputs.depth = fg.CreateTexture("GBuffer.Depth", depthDesc);

    PassHandle gbufferPass = fg.AddPass("GBuffer");

    fg.PassWrite(gbufferPass, outputs.albedo, ResourceState::RenderTarget);
    fg.PassWrite(gbufferPass, outputs.normal, ResourceState::RenderTarget);
    fg.PassWrite(gbufferPass, outputs.material, ResourceState::RenderTarget);
    fg.PassWrite(gbufferPass, outputs.depth, ResourceState::DepthStencilWrite);

    fg.SetPassCallback(gbufferPass, [this, outputs](ng::RenderContext& ctx, const FrameGraph& fg) {
        this->Execute(ctx, fg, outputs);
    });

    Msg("  ✓ G-Buffer pass configured");

    return outputs;
}

void GBufferPass::Execute(
    ng::RenderContext& ctx,
    const FrameGraph& fg,
    const GBufferOutputs& outputs
) {
    Msg("~ [GBufferPass] Executing");

    auto executeStart = std::chrono::high_resolution_clock::now();

    m_stats = Stats{};

    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(outputs.albedo);
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(outputs.normal);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(outputs.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(outputs.depth);

    if (!m_pipeline)
    {
        if (!CreatePipeline(outputs, fg))
        {
            Msg("! [GBufferPass] Failed to create pipeline");
            return;
        }
    }

    GlobalConstants globalCB = {};
    FillGlobalConstants(globalCB);

    ID3D11Device* d3dDevice = static_cast<ID3D11Device*>(
        m_device->GetNativeDevice()->getNativeObject(nvrhi::ObjectTypes::D3D11_Device).pointer);
    VERIFY(d3dDevice);

    ID3D11DeviceContext* d3dContext = nullptr;
    d3dDevice->GetImmediateContext(&d3dContext);
    VERIFY(d3dContext);

    xr_set<ID3D11Buffer*> updatedBuffers;

    Msg("  [GBufferPass] Global CB matrices:");
    Msg("    m_V:  [%.3f, %.3f, %.3f, %.3f]", globalCB.m_V[0], globalCB.m_V[1], globalCB.m_V[2], globalCB.m_V[3]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_V[4], globalCB.m_V[5], globalCB.m_V[6], globalCB.m_V[7]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_V[8], globalCB.m_V[9], globalCB.m_V[10], globalCB.m_V[11]);
    Msg("    m_P:  [%.3f, %.3f, %.3f, %.3f]", globalCB.m_P[0], globalCB.m_P[1], globalCB.m_P[2], globalCB.m_P[3]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_P[4], globalCB.m_P[5], globalCB.m_P[6], globalCB.m_P[7]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_P[8], globalCB.m_P[9], globalCB.m_P[10], globalCB.m_P[11]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_P[12], globalCB.m_P[13], globalCB.m_P[14], globalCB.m_P[15]);
    Msg("    m_VP: [%.3f, %.3f, %.3f, %.3f]", globalCB.m_VP[0], globalCB.m_VP[1], globalCB.m_VP[2], globalCB.m_VP[3]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_VP[4], globalCB.m_VP[5], globalCB.m_VP[6], globalCB.m_VP[7]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_VP[8], globalCB.m_VP[9], globalCB.m_VP[10], globalCB.m_VP[11]);
    Msg("          [%.3f, %.3f, %.3f, %.3f]", globalCB.m_VP[12], globalCB.m_VP[13], globalCB.m_VP[14], globalCB.m_VP[15]);
    Msg("    eye:  (%.1f, %.1f, %.1f)", globalCB.eye_position.x, globalCB.eye_position.y, globalCB.eye_position.z);

    if (g_geometryCollector != nullptr) {
        const auto& batches = g_geometryCollector->GetBatches();
        xr_set<nvrhi::IBuffer*> updatedGlobalBuffers;

        for (const auto& batch : batches) {
            if (batch.visual && m_materialCache) {
                MaterialPSO* matPSO = m_materialCache->GetOrCreatePSO(batch.visual, outputs, fg);
                if (matPSO) {
                    for (const auto& cbInfo : matPSO->constantBuffers) {
                        if (!cbInfo.isPerObject && cbInfo.nvrhiBuffer) {
                            if (updatedGlobalBuffers.find(cbInfo.nvrhiBuffer.Get()) == updatedGlobalBuffers.end()) {
                                u32 sizeToWrite = std::min<u32>(sizeof(GlobalConstants), cbInfo.size);
                                Msg("  [GBufferPass] Pre-updating global CB slot %u (%u bytes) BEFORE render pass",
                                    cbInfo.slot, sizeToWrite);
                                ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &globalCB, sizeToWrite);
                                updatedGlobalBuffers.insert(cbInfo.nvrhiBuffer.Get());
                            }
                        }
                    }
                }
            }
        }
        Msg("  [GBufferPass] Pre-updated %u unique global CB buffers", (u32)updatedGlobalBuffers.size());
    }

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = albedo;
    passDesc.renderTargets[1] = normal;
    passDesc.renderTargets[2] = material;
    passDesc.numRenderTargets = 3;
    passDesc.depthStencil = depth;
    passDesc.clearValue.color[0] = m_config.clearColor[0];
    passDesc.clearValue.color[1] = m_config.clearColor[1];
    passDesc.clearValue.color[2] = m_config.clearColor[2];
    passDesc.clearValue.color[3] = m_config.clearColor[3];
    passDesc.clearValue.depth = m_config.clearDepth;
    passDesc.clearValue.stencil = m_config.clearStencil;
    passDesc.clearColor = true;
    passDesc.clearDepth = true;
    passDesc.clearStencil = true;

    ctx.BeginRenderPass(passDesc);

    ctx.SetViewport(0, 0,
        static_cast<float>(m_config.width),
        static_cast<float>(m_config.height));

    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = m_config.width;
    scissor.height = m_config.height;
    ctx.SetScissor(scissor);

    if (g_geometryCollector != nullptr) {
        const auto& batches = g_geometryCollector->GetBatches();
        m_stats.numObjects = static_cast<u32>(batches.size());

        Msg("  Rendering %u geometry batches", m_stats.numObjects);

        ng::PipelineState* currentPipeline = nullptr;
        nvrhi::IBindingSet* currentBindingSet = nullptr;

        for (const auto& batch : batches) {
            ng::PipelineState* pipelineToUse = m_pipeline;
            MaterialPSO* matPSO = nullptr;

            if (batch.visual && m_materialCache) {
                matPSO = m_materialCache->GetOrCreatePSO(
                    batch.visual,
                    outputs,
                    fg);

                if (matPSO && matPSO->pso) {
                    pipelineToUse = matPSO->pso;
                }
            }

            if (pipelineToUse != currentPipeline) {
                ctx.SetPipeline(pipelineToUse->GetNativePipeline());
                currentPipeline = pipelineToUse;
            }

            if (matPSO) {

                constexpr u32 VCB_SIZE = 256;

                u8 cbData[VCB_SIZE] = {};

                auto CopyMatrix3x4 = [](u8* dest, const Fmatrix& src) {
                    Fmatrix transposed;
                    transposed.transpose(src);

                    float* destF = reinterpret_cast<float*>(dest);
                    destF[0]  = transposed._11; destF[1]  = transposed._12; destF[2]  = transposed._13; destF[3]  = transposed._14;
                    destF[4]  = transposed._21; destF[5]  = transposed._22; destF[6]  = transposed._23; destF[7]  = transposed._24;
                    destF[8]  = transposed._31; destF[9]  = transposed._32; destF[10] = transposed._33; destF[11] = transposed._34;
                };

                Fmatrix xform = batch.worldMatrix;
                Fmatrix xform_v;
                xform_v.mul(batch.worldMatrix, Device.mView);

                CopyMatrix3x4(cbData + 0, xform);
                CopyMatrix3x4(cbData + 48, xform_v);

                VERIFY(m_perObjectCB.IsValid());
                nvrhi::IBuffer* vcbBuffer = m_device->GetNativeBuffer(m_perObjectCB);

                ctx.WriteBuffer(vcbBuffer, cbData, VCB_SIZE);

                nvrhi::BindingSetHandle fullBindingSet =
                    m_materialCache->GetOrCreateBindingSet(matPSO, vcbBuffer, matPSO->pass);

                if (fullBindingSet) {
                    ctx.SetBindingSet(0, fullBindingSet.Get());
                    currentBindingSet = fullBindingSet.Get();
                }
            } else {
                UpdatePerObjectConstants(ctx, batch);

                if (batch.bindingSet != currentBindingSet) {
                    ctx.SetBindingSet(0, batch.bindingSet);
                    currentBindingSet = batch.bindingSet;
                }
            }

            ctx.SetVertexBuffer(0, batch.vertexBuffer.Get(), 0);
            ctx.SetIndexBuffer(batch.indexBuffer.Get(), nvrhi::Format::R16_UINT, 0);

            ctx.DrawIndexed(batch.indexCount, batch.startIndex, batch.baseVertex);

            m_stats.numDrawCalls++;
            m_stats.numTriangles += batch.indexCount / 3;
        }
    } else {
        Msg("  (No geometry collector available)");
    }

    ctx.EndRenderPass();

    if (d3dContext) {
        d3dContext->Release();
    }

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_stats.cpuTimeMs = std::chrono::duration<float, std::milli>(
        executeEnd - executeStart
    ).count();

    const auto& ctxStats = ctx.GetStats();
    m_stats.numDrawCalls = ctxStats.numDrawCalls;
    m_stats.numTriangles = 0;

    Msg("  ✓ G-Buffer pass complete: %u draws, %u tris, %.2f ms",
        m_stats.numDrawCalls,
        m_stats.numTriangles,
        m_stats.cpuTimeMs);
}

void GBufferPass::UpdatePerObjectConstantsData(const GeometryBatch& batch)
{
    struct PerObjectConstants {
        Fmatrix worldViewProj;
        Fmatrix world;
        Fmatrix worldIT;
    };

    PerObjectConstants constants;

    Fmatrix viewProj;
    viewProj.mul(Device.mView, Device.mProject);

    constants.worldViewProj.mul(batch.worldMatrix, viewProj);

    constants.world = batch.worldMatrix;

    Fmatrix temp;
    temp.invert(batch.worldMatrix);
    constants.worldIT.transpose(temp);

    VERIFY(m_perObjectCB.IsValid());
    m_device->UpdateBuffer(m_perObjectCB, &constants, sizeof(constants));
}

void GBufferPass::UpdatePerObjectConstants(
    ng::RenderContext& ctx,
    const GeometryBatch& batch
) {
    UpdatePerObjectConstantsData(batch);

    ctx.SetConstantBuffer(0, m_perObjectCB);
}

}
