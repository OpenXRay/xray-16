#include "stdafx.h"
#include "GBufferPass.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Geometry/GeometryBatch.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/TextureDescrManager.h"
#include "Layers/xrRender/SkeletonCustom.h"
#include "Layers/xrRender/FSkinned.h"
#include "Layers/xrRender/FTreeVisual.h"
#include "xrEngine/Render.h"

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

    m_vcbPool = xr_make_unique<framegraph::VolatileConstantBufferPool>(device);

    m_materialCache = xr_make_unique<MaterialCache>(device, m_vcbPool.get());

    if (!LoadShaders())
    {
        Msg("! [GBufferPass] Failed to load shaders");
    }
}

GBufferPass::~GBufferPass() {
    if (m_vertexShaderBytecode) {
        m_vertexShaderBytecode->Release();
        m_vertexShaderBytecode = nullptr;
    }
    if (m_pixelShaderBytecode) {
        m_pixelShaderBytecode->Release();
        m_pixelShaderBytecode = nullptr;
    }
    Msg("* [GBufferPass] Destroyed");
}

bool GBufferPass::LoadShaders()
{
    ShaderLoader loader(m_device);

    m_vertexShaderNative = loader.LoadVertexShader("gbuffer", "main", &m_vertexShaderBytecode);
    if (!m_vertexShaderNative)
    {
        Msg("! [GBufferPass] Failed to load gbuffer vertex shader");
        return false;
    }

    m_pixelShaderNative = loader.LoadPixelShader("gbuffer", "main", &m_pixelShaderBytecode);
    if (!m_pixelShaderNative)
    {
        Msg("! [GBufferPass] Failed to load gbuffer pixel shader");
        return false;
    }

    Msg("* [GBufferPass] Analyzing constant buffer requirements...");

    if (m_vertexShaderBytecode) {
        auto vsCBs = ShaderReflector::AnalyzeConstantBuffers(m_vertexShaderBytecode);
        Msg("  → Vertex shader has %u constant buffers", vsCBs.buffers.size());

        for (const auto& cbInfo : vsCBs.buffers) {
            VolatileConstantBufferPool::CBLayout layout(cbInfo.name.c_str(), cbInfo.slot, cbInfo.size);
            m_vcbPool->GetOrCreateVCB(layout);

            if (xr_strcmp(cbInfo.name.c_str(), "dynamic_transforms") == 0) {
                m_dynamicTransformsLayout = layout;
            }
        }
    }

    if (m_pixelShaderBytecode) {
        auto psCBs = ShaderReflector::AnalyzeConstantBuffers(m_pixelShaderBytecode);
        Msg("  → Pixel shader has %u constant buffers", psCBs.buffers.size());

        for (const auto& cbInfo : psCBs.buffers) {
            VolatileConstantBufferPool::CBLayout layout(cbInfo.name.c_str(), cbInfo.slot, cbInfo.size);
            m_vcbPool->GetOrCreateVCB(layout);

            if (xr_strcmp(cbInfo.name.c_str(), "Material") == 0) {
                m_materialLayout = layout;
            }
        }
    }

    m_vcbPool->LogStats();

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

    Msg("  ✓ G-Buffer shaders loaded successfully");
    return true;
}

bool GBufferPass::CreatePipeline(const framegraph::DefaultOutputLayout& outputs, const FrameGraph& fg)
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

void GBufferPass::Setup(FrameGraph& fg) {

    m_outputs = framegraph::DefaultOutputLayout{};

    ResourceDesc albedoDesc;
    albedoDesc.type = ResourceDesc::Type::Texture2D;
    albedoDesc.width = m_config.width;
    albedoDesc.height = m_config.height;
    albedoDesc.format = m_config.albedoFormat;
    albedoDesc.isRenderTarget = true;
    albedoDesc.isTransient = true;
    albedoDesc.debugName = "GBuffer.Albedo";

    m_outputs.albedo = fg.CreateTexture("GBuffer.Albedo", albedoDesc);

    ResourceDesc normalDesc;
    normalDesc.type = ResourceDesc::Type::Texture2D;
    normalDesc.width = m_config.width;
    normalDesc.height = m_config.height;
    normalDesc.format = m_config.normalFormat;
    normalDesc.isRenderTarget = true;
    normalDesc.isTransient = true;
    normalDesc.debugName = "GBuffer.Normal";

    m_outputs.normal = fg.CreateTexture("GBuffer.Normal", normalDesc);

    ResourceDesc materialDesc;
    materialDesc.type = ResourceDesc::Type::Texture2D;
    materialDesc.width = m_config.width;
    materialDesc.height = m_config.height;
    materialDesc.format = m_config.materialFormat;
    materialDesc.isRenderTarget = true;
    materialDesc.isTransient = true;
    materialDesc.debugName = "GBuffer.Material";

    m_outputs.material = fg.CreateTexture("GBuffer.Material", materialDesc);

    ResourceDesc depthDesc;
    depthDesc.type = ResourceDesc::Type::Texture2D;
    depthDesc.width = m_config.width;
    depthDesc.height = m_config.height;
    depthDesc.format = m_config.depthFormat;
    depthDesc.isDepthStencil = true;
    depthDesc.isTransient = true;
    depthDesc.debugName = "GBuffer.Depth";

    m_outputs.depth = fg.CreateTexture("GBuffer.Depth", depthDesc);

    PassIO io;
    io.writes.push_back({m_outputs.albedo, ResourceState::RenderTarget});
    io.writes.push_back({m_outputs.normal, ResourceState::RenderTarget});
    io.writes.push_back({m_outputs.material, ResourceState::RenderTarget});
    io.writes.push_back({m_outputs.depth, ResourceState::DepthStencilWrite});

    RegisterPass(fg, "GBuffer", io);

    Msg("  ✓ G-Buffer pass configured");
}

void GBufferPass::Execute(ng::RenderContext& ctx, const FrameGraph& fg) {

    auto executeStart = std::chrono::high_resolution_clock::now();

    m_gbufferStats = Stats{};

    u32 nullVBCount = 0, nullIBCount = 0;
    for (u32 i = 0; i < m_batches.size(); ++i) {
        const auto* batch = m_batches[i];
        if (!batch->vertexBuffer) {
            nullVBCount++;
            if (i < 5) Msg("!   Batch %u: VB is NULL! (debugName='%s')", i, batch->debugName.c_str());
        }
        if (!batch->indexBuffer) {
            nullIBCount++;
            if (i < 5) Msg("!   Batch %u: IB is NULL! (debugName='%s')", i, batch->debugName.c_str());
        }
    }
    if (nullVBCount > 0 || nullIBCount > 0) {
        Msg("! [GBufferPass] ERROR: Found %u batches with null VB, %u with null IB (total %u batches)",
            nullVBCount, nullIBCount, m_batches.size());
    } else {
    }

    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(m_outputs.albedo);
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(m_outputs.normal);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(m_outputs.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(m_outputs.depth);

    if (!m_pipeline)
    {
        if (!CreatePipeline(m_outputs, fg))
        {
            Msg("! [GBufferPass] Failed to create pipeline");
            return;
        }
    }

    StaticGlobals staticGlobalsCB = {};
    FillGlobalConstants(staticGlobalsCB);

    DynamicTransforms dynamicTransformsCB = {};
    FillDynamicTransforms(dynamicTransformsCB);

    Msg("    m_WVP[0-3]: %.3f, %.3f, %.3f, %.3f",
        dynamicTransformsCB.m_WVP[0], dynamicTransformsCB.m_WVP[1],
        dynamicTransformsCB.m_WVP[2], dynamicTransformsCB.m_WVP[3]);
    Msg("    m_W[0-3]:   %.3f, %.3f, %.3f, %.3f",
        dynamicTransformsCB.m_W[0], dynamicTransformsCB.m_W[1],
        dynamicTransformsCB.m_W[2], dynamicTransformsCB.m_W[3]);

    ID3D11Device* d3dDevice = static_cast<ID3D11Device*>(
        m_device->GetNativeDevice()->getNativeObject(nvrhi::ObjectTypes::D3D11_Device).pointer);
    VERIFY(d3dDevice);

    xr_set<ID3D11Buffer*> updatedBuffers;

    Msg("    static_globals: m_V, m_P, m_VP, lighting, fog, etc.");
    Msg("    dynamic_transforms: m_W, m_WV, m_WVP from RCache.xforms");

    if (!m_batches.empty()) {
        xr_set<nvrhi::IBuffer*> updatedGlobalBuffers;

        for (const auto* batch : m_batches) {
            if (batch->visual && m_materialCache) {
                MaterialPSO* matPSO = m_materialCache->GetOrCreatePSO(batch->visual, m_outputs, fg);
                if (matPSO) {
                    for (const auto& cbInfo : matPSO->constantBuffers) {
                        if (!cbInfo.isPerObject && cbInfo.nvrhiBuffer) {
                            if (updatedGlobalBuffers.find(cbInfo.nvrhiBuffer.Get()) == updatedGlobalBuffers.end()) {
                                if (cbInfo.name == "static_globals") {
                                    u32 sizeToWrite = std::min<u32>(sizeof(StaticGlobals), cbInfo.size);
                                    ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &staticGlobalsCB, sizeToWrite);
                                    updatedGlobalBuffers.insert(cbInfo.nvrhiBuffer.Get());
                                } else if (cbInfo.name == "dynamic_transforms") {
                                    u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                                    ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicTransformsCB, sizeToWrite);
                                    updatedGlobalBuffers.insert(cbInfo.nvrhiBuffer.Get());
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = normal;
    passDesc.renderTargets[1] = albedo;
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

    Msg("! [GBufferPass] Framebuffer binding (X-Ray convention):");
    Msg("!   Slot 0 → Normal   (texture %p)", normal);
    Msg("!   Slot 1 → Albedo   (texture %p)", albedo);
    Msg("!   Slot 2 → Material (texture %p)", material);

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

    m_gbufferStats.numObjects = static_cast<u32>(m_batches.size());
    Msg("  Rendering %u geometry batches (from IPass routing)", m_gbufferStats.numObjects);

    if (!m_batches.empty()) {

        ng::PipelineState* currentPipeline = nullptr;
        nvrhi::IBindingSet* currentBindingSet = nullptr;

        float lastDetailScale = -1.0f;

        for (const auto* batch : m_batches) {
            ng::PipelineState* pipelineToUse = m_pipeline;
            MaterialPSO* matPSO = nullptr;

            if (batch->visual && m_materialCache) {
                matPSO = m_materialCache->GetOrCreatePSO(
                    batch->visual,
                    m_outputs,
                    fg);

                if (matPSO && matPSO->pso) {
                    pipelineToUse = matPSO->pso;
                }
            }

            if (pipelineToUse != currentPipeline) {
                ctx.SetPipeline(pipelineToUse->GetNativePipeline());
                currentPipeline = pipelineToUse;
            }

            if (matPSO && matPSO->detail_scale != lastDetailScale) {
                DynamicTransforms dynamicCB = {};
                FillDynamicTransforms(dynamicCB);

                extern float r__dtex_range;
                dynamicCB.dt_params.set(matPSO->detail_scale, matPSO->detail_scale, matPSO->detail_scale,
                                       1.0f / xray::render::RENDER_NAMESPACE::r__dtex_range);

                for (const auto& cbInfo : matPSO->constantBuffers) {
                    if (cbInfo.name == "dynamic_transforms") {
                        u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                        ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                        break;
                    }
                }

                lastDetailScale = matPSO->detail_scale;
            }

            if (matPSO) {
                ng::BufferHandle vcbHandle;
                u32 vcbSize = 0;
                if (!matPSO->vcbRequirements.empty()) {
                    vcbHandle = matPSO->vcbRequirements[0].vcbHandle;
                    vcbSize = matPSO->vcbRequirements[0].size;
                }

                if (!vcbHandle.IsValid()) {
                    Msg("! [GBufferPass] Material has no valid VCB");
                    continue;
                }

                constexpr u32 MAX_CB_SIZE = 4096;
                if (vcbSize > MAX_CB_SIZE) {
                    Msg("! [GBufferPass] VCB size %u exceeds maximum %u", vcbSize, MAX_CB_SIZE);
                    continue;
                }

                u8 cbData[MAX_CB_SIZE];
                ZeroMemory(cbData, vcbSize);

                auto CopyBoneMatrix = [](u8* dest, const Fmatrix& M) {
                    float* destF = reinterpret_cast<float*>(dest);
                    destF[0]  = M._11; destF[1]  = M._21; destF[2]  = M._31; destF[3]  = M._41;
                    destF[4]  = M._12; destF[5]  = M._22; destF[6]  = M._32; destF[7]  = M._42;
                    destF[8]  = M._13; destF[9]  = M._23; destF[10] = M._33; destF[11] = M._43;
                };

                auto CopyMatrix3x4 = [](u8* dest, const Fmatrix& src) {
                    Fmatrix transposed;
                    transposed.transpose(src);
                    float* destF = reinterpret_cast<float*>(dest);
                    destF[0]  = transposed._11; destF[1]  = transposed._12; destF[2]  = transposed._13; destF[3]  = transposed._14;
                    destF[4]  = transposed._21; destF[5]  = transposed._22; destF[6]  = transposed._23; destF[7]  = transposed._24;
                    destF[8]  = transposed._31; destF[9]  = transposed._32; destF[10] = transposed._33; destF[11] = transposed._34;
                };

                bool isSkeleton = false;
                if (batch->visual && batch->renderable) {
                    auto visualType = batch->visual->getType();
                    isSkeleton = (visualType == MT_SKELETON_GEOMDEF_ST || visualType == MT_SKELETON_GEOMDEF_PM);
                }

                if (isSkeleton) {

                    DynamicTransforms dynamicCB = {};
                    FillDynamicTransforms(dynamicCB, batch->worldMatrix, 0.f);
                    for (const auto& cbInfo : matPSO->constantBuffers) {
                        if (cbInfo.name == "dynamic_transforms") {
                            u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                            ctx.WriteBuffer(cbInfo.nvrhiBuffer.Get(), &dynamicCB, sizeToWrite);
                            break;
                        }
                    }

                    RENDER_NAMESPACE::CKinematics* Parent = nullptr;

                    auto visualType = batch->visual->getType();
                    if (visualType == MT_SKELETON_GEOMDEF_ST) {
                        auto* skeletonMesh = static_cast<RENDER_NAMESPACE::CSkeletonX_ST*>(batch->visual);
                        Parent = skeletonMesh->GetParent();
                    } else if (visualType == MT_SKELETON_GEOMDEF_PM) {
                        auto* skeletonMesh = static_cast<RENDER_NAMESPACE::CSkeletonX_PM*>(batch->visual);
                        Parent = skeletonMesh->GetParent();
                    }

                    if (Parent) {
                        Parent->CalculateBones(TRUE);

                        u32 count = Parent->LL_BoneCount();
                        u32 maxBones = vcbSize / 48;
                        count = std::min(count, maxBones);

                        float* array = reinterpret_cast<float*>(cbData);
                        for (u32 mid = 0; mid < count; mid++)
                        {
                            Fmatrix& M = Parent->LL_GetTransform_R(u16(mid));
                            u32 id = mid * 3;
                            array[(id + 0) * 4 + 0] = M._11;
                            array[(id + 0) * 4 + 1] = M._21;
                            array[(id + 0) * 4 + 2] = M._31;
                            array[(id + 0) * 4 + 3] = M._41;
                            array[(id + 1) * 4 + 0] = M._12;
                            array[(id + 1) * 4 + 1] = M._22;
                            array[(id + 1) * 4 + 2] = M._32;
                            array[(id + 1) * 4 + 3] = M._42;
                            array[(id + 2) * 4 + 0] = M._13;
                            array[(id + 2) * 4 + 1] = M._23;
                            array[(id + 2) * 4 + 2] = M._33;
                            array[(id + 2) * 4 + 3] = M._43;
                        }
                    }
                } else {

                    Fmatrix xform = batch->worldMatrix;
                    Fmatrix xform_v;
                    xform_v.mul(batch->worldMatrix, Device.mView);

                    CopyMatrix3x4(cbData + 0, xform);
                    CopyMatrix3x4(cbData + 48, xform_v);

                    using namespace xray::render::RENDER_NAMESPACE;
                    float tree_scale = 1.0f / float(FTreeVisual_quant);

                    float* constsPtr = reinterpret_cast<float*>(cbData + 96);
                    constsPtr[0] = tree_scale;
                    constsPtr[1] = tree_scale;
                    constsPtr[2] = 0.0f;
                    constsPtr[3] = 0.0f;
                }

                nvrhi::IBuffer* vcbBuffer = m_device->GetNativeBuffer(vcbHandle);

                ctx.WriteBuffer(vcbBuffer, cbData, vcbSize);

                m_materialCache->GetOrCreateBindingSet(matPSO, vcbBuffer, matPSO->pass);

                ctx.SetBindingSet(0, matPSO->vsBindingSet.Get());
                ctx.SetBindingSet(1, matPSO->psBindingSet.Get());
                currentBindingSet = matPSO->vsBindingSet.Get();
            }

            nvrhi::IBuffer* vb = batch->vertexBuffer.Get();
            nvrhi::IBuffer* ib = batch->indexBuffer.Get();

            if (!vb || !ib) {
                Msg("! [GBufferPass] ERROR: Batch %u has null buffers! VB=%p, IB=%p, visual=%p, debugName='%s'",
                    m_gbufferStats.numDrawCalls,
                    vb, ib, batch->visual, batch->debugName.c_str());
                Msg("!   vertexBuffer handle valid: %s", batch->vertexBuffer ? "YES" : "NO");
                Msg("!   indexBuffer handle valid: %s", batch->indexBuffer ? "YES" : "NO");
                continue;
            }

            ctx.SetVertexBuffer(0, vb, 0);
            ctx.SetIndexBuffer(ib, nvrhi::Format::R16_UINT, 0);

            ctx.DrawIndexed(batch->indexCount, batch->startIndex, batch->baseVertex);

            m_gbufferStats.numDrawCalls++;
            m_gbufferStats.numTriangles += batch->indexCount / 3;
        }
    }

    ctx.EndRenderPass();

    auto executeEnd = std::chrono::high_resolution_clock::now();
    m_gbufferStats.cpuTimeMs = std::chrono::duration<float, std::milli>(
        executeEnd - executeStart
    ).count();

    Msg("  ✓ G-Buffer pass complete: %u draws, %u tris, %.2f ms",
        m_gbufferStats.numDrawCalls,
        m_gbufferStats.numTriangles,
        m_gbufferStats.cpuTimeMs);
}

}
