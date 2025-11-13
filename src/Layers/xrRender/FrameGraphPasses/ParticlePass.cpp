#include "stdafx.h"
#include "ParticlePass.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderStateConversion.h"
#include "Layers/xrRender/ParticleEffect.h"
#include "Layers/xrRender/ParticleEffectDef.h"
#include "Layers/xrRender/ParticleGroup.h"
#include "Layers/xrRender/PSLibrary.h"
#include "Layers/xrRender/SH_Texture.h"
#include "Layers/xrRender/ResourceManager.h"
#include "xrEngine/Render.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/StateManager/dx11State.h"
#include "../Externals/nvrhi/src/common/dxgi-format.h"
#endif

extern ENGINE_API float psHUD_FOV;

namespace xray::render::passes {

using namespace framegraph;
using RENDER_NAMESPACE::PS::CParticleEffect;
using RENDER_NAMESPACE::PS::CParticleGroup;

Fmatrix ParticlePass::ApplyHUDFOVAdjustment(const Fmatrix& worldMatrix)
{
    float fovScale = 1.0f / psHUD_FOV;

    Fmatrix viewMatrix = Device.mView;
    Fmatrix invView;
    invView.invert(viewMatrix);

    Fmatrix fovScaleMatrix;
    fovScaleMatrix.identity();
    fovScaleMatrix._11 = fovScale;
    fovScaleMatrix._22 = fovScale;
    fovScaleMatrix._33 = 1.0f;

    Fmatrix temp1, temp2, result;
    temp1.mul(viewMatrix, worldMatrix);
    temp2.mul(fovScaleMatrix, temp1);
    result.mul(invView, temp2);

    return result;
}

ParticlePass::ParticlePass(ng::RenderDevice* device, MaterialCache* materialCache, const ParticlePassConfig& config)
    : m_device(device)
    , m_materialCache(materialCache)
    , m_config(config)
{
    VERIFY(m_device != nullptr);
    VERIFY(m_materialCache != nullptr);

    if (config.width == 0 || config.height == 0) {
        Msg("! [ParticlePass] ERROR: Invalid resolution %ux%u", config.width, config.height);
    }
}

ParticlePass::~ParticlePass() {
}

void ParticlePass::Setup(FrameGraph& fg) {

    PassIO io;

    io.reads.push_back({m_outputs.depth, ResourceState::DepthStencilRead});

    io.writes.push_back({m_outputs.albedo, ResourceState::RenderTarget});
    io.writes.push_back({m_outputs.normal, ResourceState::RenderTarget});
    io.writes.push_back({m_outputs.material, ResourceState::RenderTarget});
    io.writes.push_back({m_outputs.depth, ResourceState::DepthStencilWrite});

    RegisterPass(fg, "Particles", io);

    Msg("  ✓ Particle pass configured");
}

void ParticlePass::EnsureQuadIndexBuffer(u32 maxQuads) {
    if (m_quadIB && m_maxQuads >= maxQuads) {
        return;
    }

    u32 numIndices = maxQuads * 6;
    xr_vector<u16> indices;
    indices.reserve(numIndices);

    for (u32 i = 0; i < maxQuads; i++) {
        u16 baseVertex = (u16)(i * 4);
        indices.push_back(baseVertex + 0);
        indices.push_back(baseVertex + 1);
        indices.push_back(baseVertex + 2);
        indices.push_back(baseVertex + 2);
        indices.push_back(baseVertex + 1);
        indices.push_back(baseVertex + 3);
    }

    nvrhi::BufferDesc ibDesc;
    ibDesc.byteSize = numIndices * sizeof(u16);
    ibDesc.isIndexBuffer = true;
    ibDesc.debugName = "ParticleQuadIB";
    ibDesc.initialState = nvrhi::ResourceStates::IndexBuffer;

    m_quadIB = m_device->GetNVRHIDevice()->createBuffer(ibDesc);
    if (!m_quadIB) {
        Msg("! [ParticlePass] ERROR: Failed to create quad index buffer");
        return;
    }

    nvrhi::CommandListHandle cmdList = m_device->GetNVRHIDevice()->createCommandList();
    cmdList->open();
    cmdList->writeBuffer(m_quadIB, indices.data(), indices.size() * sizeof(u16));
    cmdList->close();
    m_device->GetNVRHIDevice()->executeCommandList(cmdList);

    m_maxQuads = maxQuads;
}

void ParticlePass::EnsureParticleVertexBuffer(u32 sizeBytes) {
    if (m_particleVB && m_particleVBSize >= sizeBytes) {
        return;
    }

    u32 allocSize = ((sizeBytes + 65535) / 65536) * 65536;

    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = allocSize;
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "ParticleDynamicVB";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = false;

    m_particleVB = m_device->GetNVRHIDevice()->createBuffer(vbDesc);
    if (!m_particleVB) {
        Msg("! [ParticlePass] ERROR: Failed to create particle vertex buffer");
        return;
    }

    m_particleVBSize = allocSize;
}

void ParticlePass::FillSprite(
    ParticleVertex*& pv,
    const Fvector& T, const Fvector& R,
    const Fvector& pos,
    const Fvector2& lt, const Fvector2& rb,
    float r1, float r2,
    u32 clr,
    float sina, float cosa)
{
    Fvector Vr, Vt;

    Vr.x = T.x * r1 * sina + R.x * r1 * cosa;
    Vr.y = T.y * r1 * sina + R.y * r1 * cosa;
    Vr.z = T.z * r1 * sina + R.z * r1 * cosa;

    Vt.x = T.x * r2 * cosa - R.x * r2 * sina;
    Vt.y = T.y * r2 * cosa - R.y * r2 * sina;
    Vt.z = T.z * r2 * cosa - R.z * r2 * sina;

    Fvector a, b, c, d;

    a.sub(Vt, Vr);
    b.add(Vt, Vr);

    c.invert(a);
    d.invert(b);

    pv->p.set(d.x + pos.x, d.y + pos.y, d.z + pos.z);
    pv->color = clr;
    pv->t.set(lt.x, rb.y);
    pv++;

    pv->p.set(a.x + pos.x, a.y + pos.y, a.z + pos.z);
    pv->color = clr;
    pv->t.set(lt.x, lt.y);
    pv++;

    pv->p.set(c.x + pos.x, c.y + pos.y, c.z + pos.z);
    pv->color = clr;
    pv->t.set(rb.x, rb.y);
    pv++;

    pv->p.set(b.x + pos.x, b.y + pos.y, b.z + pos.z);
    pv->color = clr;
    pv->t.set(rb.x, lt.y);
    pv++;
}

void ParticlePass::FillSprite(
    ParticleVertex*& pv,
    const Fvector& pos, const Fvector& dir,
    const Fvector2& lt, const Fvector2& rb,
    float r1, float r2,
    u32 clr,
    float sina, float cosa)
{
    const Fvector& T = dir;

    Fvector R;
    R.crossproduct(T, Device.vCameraDirection).normalize_safe();

    FillSprite(pv, T, R, pos, lt, rb, r1, r2, clr, sina, cosa);
}

void ParticlePass::Execute(ng::RenderContext& ctx, const FrameGraph& fg) {
    auto execStart = std::chrono::high_resolution_clock::now();
    m_stats.numDrawCalls = 0;
    m_stats.numTriangles = 0;
    m_stats.numBatches = 0;

    u32 totalWorldParticles = m_worldParticleBatches ? (u32)m_worldParticleBatches->size() : 0;
    u32 totalHUDParticles = m_hudParticleBatches ? (u32)m_hudParticleBatches->size() : 0;

    if (totalWorldParticles == 0 && totalHUDParticles == 0) {
        m_stats.cpuTimeMs = 0.0f;
        return;
    }

    nvrhi::ITexture* normal = fg.GetPhysicalTexture(m_outputs.normal);
    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(m_outputs.albedo);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(m_outputs.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(m_outputs.depth);

    if (!normal || !albedo || !material || !depth) {
        Msg("! [ParticlePass] ERROR: Missing render targets!");
        return;
    }

    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = normal;
    passDesc.renderTargets[1] = albedo;
    passDesc.renderTargets[2] = material;
    passDesc.numRenderTargets = 3;
    passDesc.depthStencil = depth;
    passDesc.clearColor = false;
    passDesc.clearDepth = false;
    passDesc.clearStencil = false;

    ctx.BeginRenderPass(passDesc);

    ng::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)m_config.width;
    viewport.height = (float)m_config.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    ctx.SetViewport(viewport);

    ng::Rect scissor;
    scissor.x = 0;
    scissor.y = 0;
    scissor.width = m_config.width;
    scissor.height = m_config.height;
    ctx.SetScissor(scissor);

    if (m_worldParticleBatches && !m_worldParticleBatches->empty()) {

        for (const auto& batch : *m_worldParticleBatches) {
            if (RenderParticleSystem(ctx, batch, false, fg)) {
                m_stats.numBatches++;
                m_stats.numDrawCalls++;
            }
        }
    }

    if (m_hudParticleBatches && !m_hudParticleBatches->empty()) {

        for (const auto& batch : *m_hudParticleBatches) {
            if (RenderParticleSystem(ctx, batch, true, fg)) {
                m_stats.numBatches++;
                m_stats.numDrawCalls++;
            }
        }
    }

    ctx.EndRenderPass();

    auto execEnd = std::chrono::high_resolution_clock::now();
    m_stats.cpuTimeMs = std::chrono::duration<float, std::milli>(execEnd - execStart).count();
}

ng::PipelineState* ParticlePass::GetOrCreateParticlePSO(
    RENDER_NAMESPACE::SPass* pass,
    const RENDER_NAMESPACE::PS::CPEDef* pDef,
    const framegraph::FrameGraph& fg)
{
    if (!pass || !pDef)
        return nullptr;

    if (!pass->vs || !pass->ps)
        return nullptr;

    RENDER_NAMESPACE::SVS* vs = pass->vs._get();
    RENDER_NAMESPACE::SPS* ps = pass->ps._get();
    if (!vs || !ps)
        return nullptr;

    if (!vs->bytecode || !ps->bytecode)
        return nullptr;

    ng::PipelineStateDesc psoDesc;

    xr_string debugNameStr = xr_string(vs->cName.c_str()) + "_" + xr_string(ps->cName.c_str());
    shared_str debugName = debugNameStr.c_str();
    psoDesc.debugName = debugName.c_str();

    ng::ShaderHandle vsHandle = m_materialCache->GetOrCreateShaderVS(vs);
    if (!vsHandle.IsValid()) {
        Msg("! [ParticlePass] ERROR: Failed to get VS '%s'", vs->cName.c_str());
        return nullptr;
    }

    ng::ShaderHandle psHandle = m_materialCache->GetOrCreateShaderPS(ps);
    if (!psHandle.IsValid()) {
        Msg("! [ParticlePass] ERROR: Failed to get PS '%s'", ps->cName.c_str());
        return nullptr;
    }

    ng::RCShader* rcVS = m_device->GetShader(vsHandle);
    if (!rcVS) {
        Msg("! [ParticlePass] ERROR: rcVS is NULL for handle %u", vs->cName.c_str());
        m_device->DestroyShader(vsHandle);
        return nullptr;
    }

    ng::RCShader* rcPS = m_device->GetShader(psHandle);
    if (!rcPS) {
        Msg("! [ParticlePass] ERROR: rcPS is NULL for handle %u", ps->cName.c_str());
        m_device->DestroyShader(psHandle);
        return nullptr;
    }

    psoDesc.vertexShader = rcVS;
    psoDesc.pixelShader = rcPS;

    psoDesc.vertexAttributes.clear();

    ng::VertexAttribute posAttr;
    posAttr.semanticName = "POSITION";
    posAttr.semanticIndex = 0;
    posAttr.format = nvrhi::Format::RGB32_FLOAT;
    posAttr.offset = 0;
    posAttr.bufferIndex = 0;
    posAttr.elementStride = sizeof(ParticleVertex);
    psoDesc.vertexAttributes.push_back(posAttr);

    ng::VertexAttribute colorAttr;
    colorAttr.semanticName = "COLOR";
    colorAttr.semanticIndex = 0;
    colorAttr.format = nvrhi::Format::RGBA8_UNORM;
    colorAttr.offset = 12;
    colorAttr.bufferIndex = 0;
    colorAttr.elementStride = sizeof(ParticleVertex);
    psoDesc.vertexAttributes.push_back(colorAttr);

    ng::VertexAttribute texAttr;
    texAttr.semanticName = "TEXCOORD";
    texAttr.semanticIndex = 0;
    texAttr.format = nvrhi::Format::RG32_FLOAT;
    texAttr.offset = 16;
    texAttr.bufferIndex = 0;
    texAttr.elementStride = sizeof(ParticleVertex);
    psoDesc.vertexAttributes.push_back(texAttr);

    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(m_outputs.albedo);
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(m_outputs.normal);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(m_outputs.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(m_outputs.depth);

    psoDesc.renderTargetFormats[0] = albedo->getDesc().format;
    psoDesc.renderTargetFormats[1] = normal->getDesc().format;
    psoDesc.renderTargetFormats[2] = material->getDesc().format;
    psoDesc.depthStencilFormat = depth->getDesc().format;
    psoDesc.renderTargetCount = 3;

    m_materialCache->SetupRenderStates(pass, psoDesc);

    psoDesc.rasterizerState.cullMode = ng::CullMode::None;
    if (pDef->m_Flags.is(RENDER_NAMESPACE::PS::CPEDef::dfCulling)) {
        if (pDef->m_Flags.is(RENDER_NAMESPACE::PS::CPEDef::dfCullCCW)) {
            psoDesc.rasterizerState.cullMode = ng::CullMode::Front;
        } else {
            psoDesc.rasterizerState.cullMode = ng::CullMode::Back;
        }
    }

    psoDesc.blendState.renderTargets[0].writeMask = ng::ColorWriteMask::None;
    psoDesc.blendState.renderTargets[2].writeMask = ng::ColorWriteMask::None;
    psoDesc.rasterizerState.scissorEnable = false;
    psoDesc.rasterizerState.multisampleEnable = false;
    psoDesc.rasterizerState.antialiasedLineEnable = false;

    psoDesc.primitiveTopology = ng::PrimitiveTopology::TriangleList;

    ng::PipelineState* pso = m_device->GetPipelineCache()->GetOrCreate(psoDesc);
    if (!pso) {
        Msg("! [ParticlePass] ERROR: Failed to create PSO for '%s'", vs->cName.c_str());
        return nullptr;
    }


    return pso;
}

ParticlePass::ParticleBindingCache* ParticlePass::CreateParticleBindingSet(
    RENDER_NAMESPACE::SPass* pass)
{
    if (!pass)
        return nullptr;

    RENDER_NAMESPACE::SVS* vs = pass->vs._get();
    RENDER_NAMESPACE::SPS* ps = pass->ps._get();
    if (!vs || !ps)
        return nullptr;

    if (!vs->bytecode || !ps->bytecode)
        return nullptr;

    xr_string cacheKeyStr;
    cacheKeyStr.append(vs->cName.c_str());
    cacheKeyStr.append("|");
    cacheKeyStr.append(ps->cName.c_str());

    RENDER_NAMESPACE::STextureList* texList = pass->T._get();
    if (texList && !texList->empty()) {
        for (size_t i = 0; i < texList->size(); i++) {
            const auto& texPair = (*texList)[i];
            RENDER_NAMESPACE::CTexture* xrayTex = texPair.second._get();
            if (xrayTex) {
                cacheKeyStr.append("|");
                cacheKeyStr.append(xrayTex->cName.c_str());
            }
        }
    }

    shared_str cacheKey = cacheKeyStr.c_str();

    auto it = m_bindingCache.find(cacheKey);
    if (it != m_bindingCache.end()) {
        return &it->second;
    }

    ParticlePass::ParticleBindingCache cacheEntry;


    auto vsCBs = framegraph::ShaderReflector::AnalyzeConstantBuffers(
        vs->bytecode->GetBufferPointer(), vs->bytecode->GetBufferSize());
    for (const auto& cbInfo : vsCBs.buffers) {
        bool isPerObject = (cbInfo.slot == 0);

        nvrhi::BufferDesc bufDesc;
        bufDesc.byteSize = cbInfo.size;
        bufDesc.isConstantBuffer = true;
        bufDesc.debugName = cbInfo.name.c_str();
        bufDesc.keepInitialState = false;
        bufDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        nvrhi::BufferHandle buffer = m_device->GetNVRHIDevice()->createBuffer(bufDesc);
        if (buffer) {
            ParticlePass::ParticleBindingCache::CBInfo info;
            info.slot = cbInfo.slot;
            info.size = cbInfo.size;
            info.name = cbInfo.name.c_str();
            info.isPerObject = isPerObject;
            info.isVertexShader = true;
            info.buffer = buffer;
            cacheEntry.cbInfos.push_back(info);
            cacheEntry.constantBuffers.push_back(buffer);

            Msg("    VS CB[%u]: '%s' (%u bytes, %s)", cbInfo.slot, cbInfo.name.c_str(),
                cbInfo.size, isPerObject ? "per-object" : "global");
        }
    }

    auto psCBs = framegraph::ShaderReflector::AnalyzeConstantBuffers(
        ps->bytecode->GetBufferPointer(), ps->bytecode->GetBufferSize());
    for (const auto& cbInfo : psCBs.buffers) {
        bool isPerObject = (cbInfo.slot == 0);

        nvrhi::BufferDesc bufDesc;
        bufDesc.byteSize = cbInfo.size;
        bufDesc.isConstantBuffer = true;
        bufDesc.debugName = cbInfo.name.c_str();
        bufDesc.keepInitialState = false;
        bufDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        nvrhi::BufferHandle buffer = m_device->GetNVRHIDevice()->createBuffer(bufDesc);
        if (buffer) {
            ParticlePass::ParticleBindingCache::CBInfo info;
            info.slot = cbInfo.slot;
            info.size = cbInfo.size;
            info.name = cbInfo.name.c_str();
            info.isPerObject = isPerObject;
            info.isVertexShader = false;
            info.buffer = buffer;
            cacheEntry.cbInfos.push_back(info);
            cacheEntry.constantBuffers.push_back(buffer);

            Msg("    PS CB[%u]: '%s' (%u bytes, %s)", cbInfo.slot, cbInfo.name.c_str(),
                cbInfo.size, isPerObject ? "per-object" : "global");
        }
    }

    nvrhi::BindingLayoutDesc vsLayoutDesc;
    vsLayoutDesc.visibility = nvrhi::ShaderType::Vertex;
    vsLayoutDesc.registerSpace = 0;

    for (const auto& cbInfo : cacheEntry.cbInfos) {
        if (cbInfo.isVertexShader) {
            if (cbInfo.isPerObject) {
                vsLayoutDesc.bindings.push_back(
                    nvrhi::BindingLayoutItem::VolatileConstantBuffer(cbInfo.slot));
            } else {
                vsLayoutDesc.bindings.push_back(
                    nvrhi::BindingLayoutItem::ConstantBuffer(cbInfo.slot));
            }
        }
    }

    cacheEntry.vsBindingLayout = m_device->GetNVRHIDevice()->createBindingLayout(vsLayoutDesc);
    if (!cacheEntry.vsBindingLayout) {
        Msg("! [ParticlePass] ERROR: Failed to create VS binding layout");
        return nullptr;
    }

    nvrhi::BindingLayoutDesc psLayoutDesc;
    psLayoutDesc.visibility = nvrhi::ShaderType::Pixel;
    psLayoutDesc.registerSpace = 0;

    for (const auto& cbInfo : cacheEntry.cbInfos) {
        if (!cbInfo.isVertexShader) {
            if (cbInfo.isPerObject) {
                psLayoutDesc.bindings.push_back(
                    nvrhi::BindingLayoutItem::VolatileConstantBuffer(cbInfo.slot));
            } else {
                psLayoutDesc.bindings.push_back(
                    nvrhi::BindingLayoutItem::ConstantBuffer(cbInfo.slot));
            }
        }
    }

    u32 numTextures = 0;
    if (texList && !texList->empty()) {
        numTextures = (u32)texList->size();
        for (u32 i = 0; i < numTextures; i++) {
            psLayoutDesc.bindings.push_back(
                nvrhi::BindingLayoutItem::Texture_SRV(i));
        }
    }

    for (u32 i = 0; i < numTextures; i++) {
        psLayoutDesc.bindings.push_back(
            nvrhi::BindingLayoutItem::Sampler(i));
    }

    cacheEntry.psBindingLayout = m_device->GetNVRHIDevice()->createBindingLayout(psLayoutDesc);
    if (!cacheEntry.psBindingLayout) {
        Msg("! [ParticlePass] ERROR: Failed to create PS binding layout");
        return nullptr;
    }

    nvrhi::BindingSetDesc vsBindingDesc;

    for (const auto& cbInfo : cacheEntry.cbInfos) {
        if (cbInfo.isVertexShader) {
            vsBindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.buffer));
        }
    }

    cacheEntry.vsBindingSet = m_device->GetNVRHIDevice()->createBindingSet(
        vsBindingDesc, cacheEntry.vsBindingLayout);
    if (!cacheEntry.vsBindingSet) {
        Msg("! [ParticlePass] ERROR: Failed to create VS binding set");
        return nullptr;
    }

    nvrhi::BindingSetDesc psBindingDesc;

    for (const auto& cbInfo : cacheEntry.cbInfos) {
        if (!cbInfo.isVertexShader) {
            psBindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.buffer));
        }
    }

    if (texList && !texList->empty()) {
        for (size_t i = 0; i < texList->size(); i++) {
            const auto& texPair = (*texList)[i];
            u32 slot = texPair.first;
            RENDER_NAMESPACE::CTexture* xrayTex = texPair.second._get();

            if (xrayTex) {
                auto* d3dTex = xrayTex->surface_get();
                if (d3dTex) {
                    ID3D11Resource* resource = nullptr;
                    d3dTex->QueryInterface(__uuidof(ID3D11Resource), (void**)&resource);

                    if (resource) {
                        D3D11_RESOURCE_DIMENSION dimension;
                        resource->GetType(&dimension);

                        ng::RenderDevice::TextureDesc texDesc;
                        texDesc.debugName = xrayTex->cName.c_str();

                        if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
                            ID3D11Texture2D* tex2d = nullptr;
                            resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex2d);
                            if (tex2d) {
                                D3D11_TEXTURE2D_DESC desc;
                                tex2d->GetDesc(&desc);

                                texDesc.width = desc.Width;
                                texDesc.height = desc.Height;
                                texDesc.mipLevels = desc.MipLevels;
                                texDesc.arraySize = desc.ArraySize;

                                if (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) {
                                    texDesc.dimension = ng::RenderDevice::TextureDesc::TextureCube;
                                } else if (desc.ArraySize > 1) {
                                    texDesc.dimension = ng::RenderDevice::TextureDesc::Texture2DArray;
                                } else {
                                    texDesc.dimension = ng::RenderDevice::TextureDesc::Texture2D;
                                }

                                texDesc.format = ConvertDxgiFormatToNvrhi(desc.Format);
                                texDesc.isRenderTarget = false;
                                texDesc.isUAV = false;

                                tex2d->Release();
                            }
                        }

                        ng::TextureHandle nvrhiTex = m_device->CreateTextureFromD3D11(resource, texDesc);
                        resource->Release();

                        if (nvrhiTex.IsValid()) {
                            cacheEntry.textures.push_back(nvrhiTex);

                            nvrhi::ITexture* nativeTex = m_device->GetNativeTexture(nvrhiTex);
                            if (nativeTex) {
                                nvrhi::TextureDimension nvrhiDim = nvrhi::TextureDimension::Texture2D;
                                if (texDesc.dimension == ng::RenderDevice::TextureDesc::TextureCube) {
                                    nvrhiDim = nvrhi::TextureDimension::TextureCube;
                                } else if (texDesc.dimension == ng::RenderDevice::TextureDesc::Texture2DArray) {
                                    nvrhiDim = nvrhi::TextureDimension::Texture2DArray;
                                }

                                psBindingDesc.bindings.push_back(
                                    nvrhi::BindingSetItem::Texture_SRV(
                                        slot, nativeTex,
                                        nvrhi::Format::UNKNOWN,
                                        nvrhi::AllSubresources,
                                        nvrhiDim));
                            }
                        }
                    }
                }
            }
        }
    }

    for (u32 i = 0; i < numTextures; i++) {
        nvrhi::SamplerDesc samplerDesc;
        samplerDesc.setAllFilters(true);
        samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
        samplerDesc.setMaxAnisotropy(8.0f);

        nvrhi::SamplerHandle sampler = m_device->GetNVRHIDevice()->createSampler(samplerDesc);
        if (sampler) {
            cacheEntry.samplers.push_back(sampler);
            psBindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::Sampler(i, sampler));
        }
    }

    cacheEntry.psBindingSet = m_device->GetNVRHIDevice()->createBindingSet(
        psBindingDesc, cacheEntry.psBindingLayout);
    if (!cacheEntry.psBindingSet) {
        Msg("! [ParticlePass] ERROR: Failed to create PS binding set");
        return nullptr;
    }

    m_bindingCache[cacheKey] = std::move(cacheEntry);
    return &m_bindingCache[cacheKey];
}

bool ParticlePass::RenderParticleSystem(
    ng::RenderContext& ctx,
    const ParticleBatch& batch,
    bool applyFOV,
    const framegraph::FrameGraph& fg)
{
    if (!batch.visual)
        return false;

    u32 vType = batch.visual->getType();

    if (vType == MT_PARTICLE_EFFECT) {
        CParticleEffect* pEffect = static_cast<CParticleEffect*>(batch.visual);

        auto* pDef = pEffect->GetDefinition();
        if (!pDef || !pDef->m_Flags.is(RENDER_NAMESPACE::PS::CPEDef::dfSprite)) {
            return false;
        }

        PAPI::Particle* particles = nullptr;
        u32 particleCount = 0;
        PAPI::ParticleManager()->GetParticles(pEffect->GetHandleEffect(), particles, particleCount);

        if (particleCount == 0 || !particles) {
            return false;
        }

        u32 requiredVBSize = particleCount * 4 * sizeof(ParticleVertex);
        EnsureParticleVertexBuffer(requiredVBSize);
        EnsureQuadIndexBuffer(particleCount);

        if (!m_particleVB || !m_quadIB) {
            Msg("! [ParticlePass] ERROR: Failed to create particle buffers");
            return false;
        }

        xr_vector<ParticleVertex> vertices;
        vertices.resize(particleCount * 4);
        ParticleVertex* pv = vertices.data();

        float sina = 0.0f, cosa = 0.0f;
        float angle = float(0xFFFFFFFF);

        for (u32 i = 0; i < particleCount; i++) {
            auto& m = particles[i];

            if (angle != m.rot.x) {
                angle = m.rot.x;
                sina = _sin(angle);
                cosa = _cos(angle);
            }

            Fvector2 lt, rb;
            lt.set(0.f, 0.f);
            rb.set(1.f, 1.f);

            if (pDef->m_Flags.is(RENDER_NAMESPACE::PS::CPEDef::dfFramed)) {
                pDef->m_Frame.CalculateTC(iFloor(float(m.frame) / 255.f), lt, rb);
            }

            float r_x = m.size.x * 0.5f;
            float r_y = m.size.y * 0.5f;

            if (pDef->m_Flags.is(RENDER_NAMESPACE::PS::CPEDef::dfVelocityScale)) {
                float speed = m.vel.magnitude();
                r_x += speed * pDef->m_VelocityScale.x;
                r_y += speed * pDef->m_VelocityScale.y;
            }

            FillSprite(pv, Device.vCameraTop, Device.vCameraRight, m.pos, lt, rb, r_x, r_y, m.color, sina, cosa);
        }

        ctx.WriteBuffer(m_particleVB.Get(), vertices.data(), vertices.size() * sizeof(ParticleVertex));

        RENDER_NAMESPACE::Shader* particleShader = pDef->m_CachedShader._get();

        if (!particleShader) {
            Msg("! [ParticlePass] WARNING: No shader for particle effect '%s'", pDef->m_Name.c_str());
            return false;
        }

        RENDER_NAMESPACE::ShaderElement* element = particleShader->E[0]._get();
        if (!element) {
            Msg("! [ParticlePass] WARNING: Shader has no elements for particle effect '%s'", pDef->m_Name.c_str());
            return false;
        }

        if (element->passes.empty()) {
            Msg("! [ParticlePass] WARNING: Shader element has no passes for particle effect '%s'", pDef->m_Name.c_str());
            return false;
        }

        RENDER_NAMESPACE::SPass* pass = element->passes[0]._get();
        if (!pass) {
            Msg("! [ParticlePass] WARNING: Invalid pass for particle effect '%s'", pDef->m_Name.c_str());
            return false;
        }

        ng::PipelineState* particlePSO = GetOrCreateParticlePSO(pass, pDef, fg);

        if (!particlePSO) {
            return false;
        }


        ParticlePass::ParticleBindingCache* bindingCache = CreateParticleBindingSet(pass);
        if (!bindingCache) {
            Msg("! [ParticlePass] WARNING: Failed to create binding sets for effect '%s'", pDef->m_Name.c_str());
            return false;
        }

        nvrhi::IGraphicsPipeline* nativePipeline = particlePSO->GetNativePipeline();
        ctx.SetPipeline(nativePipeline);

        StaticGlobals staticGlobalsCB = {};
        FillGlobalConstants(staticGlobalsCB);

        DynamicTransforms dynamicTransformsCB = {};
        FillDynamicTransforms(dynamicTransformsCB);

        struct ParticlePerFrame {
            Fmatrix mVPTexgen;
        };
        ParticlePerFrame perFrameCB = {};

        Fmatrix mVP;
        mVP.mul(Device.mProject, Device.mView);

        perFrameCB.mVPTexgen.transpose(mVP);

        for (const auto& cbInfo : bindingCache->cbInfos) {
            if (cbInfo.name == "static_globals") {
                u32 sizeToWrite = std::min<u32>(sizeof(StaticGlobals), cbInfo.size);
                ctx.WriteBuffer(cbInfo.buffer.Get(), &staticGlobalsCB, sizeToWrite);
            }
            else if (cbInfo.name == "dynamic_transforms") {
                u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                ctx.WriteBuffer(cbInfo.buffer.Get(), &dynamicTransformsCB, sizeToWrite);
            }
            else if (cbInfo.isPerObject && cbInfo.isVertexShader) {
                u32 sizeToWrite = std::min<u32>(sizeof(ParticlePerFrame), cbInfo.size);
                ctx.WriteBuffer(cbInfo.buffer.Get(), &perFrameCB, sizeToWrite);
            }
        }

        ctx.SetBindingSet(0, bindingCache->vsBindingSet.Get());
        ctx.SetBindingSet(1, bindingCache->psBindingSet.Get());

        ctx.SetVertexBuffer(0, m_particleVB.Get(), 0);

        ctx.SetIndexBuffer(m_quadIB.Get(), nvrhi::Format::R16_UINT, 0);

        u32 indexCount = particleCount * 6;
        ctx.DrawIndexed(indexCount, 0, 0);

        return true;
    }

    else if (vType == MT_PARTICLE_GROUP) {
        CParticleGroup* pGroup = static_cast<CParticleGroup*>(batch.visual);

        u32 particleCount = pGroup->ParticlesCount();
        if (particleCount == 0) {
            return false;
        }

        return false;
    }

    return false;
}

}
