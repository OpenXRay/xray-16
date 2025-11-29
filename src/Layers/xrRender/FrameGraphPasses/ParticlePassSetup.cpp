// xrRender/FrameGraphPasses/ParticlePassSetup.cpp
#include "stdafx.h"
#include "ParticlePassSetup.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/IPass.h"
#include "Layers/xrRender/FrameGraph/RenderPassBuilder.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderStateConversion.h"
#include "Layers/xrRender/ParticleEffect.h"
#include "Layers/xrRender/ParticleEffectDef.h"
#include "Layers/xrRender/ParticleGroup.h"
#include "Layers/xrRender/PSLibrary.h"
#include "Layers/xrRender/SH_Texture.h"
#include "Layers/xrRender/ResourceManager.h"
#include "xrParticles/psystem.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRenderDX11/StateManager/dx11State.h"

extern ENGINE_API float psHUD_FOV;

namespace RENDER_NAMESPACE
{
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace framegraph;
using xray::render::ConvertDxgiFormatToNvrhi;
using RENDER_NAMESPACE::PS::CParticleEffect;
using RENDER_NAMESPACE::PS::CParticleGroup;
using RENDER_NAMESPACE::PS::CPEDef;

// ═══════════════════════════════════════════════════════
//  STATIC STATE (persists across frames)
// ═══════════════════════════════════════════════════════
// These are managed at module scope since the pass is lambda-based

static nvrhi::BufferHandle s_particleVB;
static u32 s_particleVBSize = 0;
static nvrhi::BufferHandle s_quadIB;
static u32 s_maxQuads = 0;

// Binding cache - stores binding sets per shader
struct ParticleBindingCache {
    nvrhi::BindingSetHandle vsBindingSet;
    nvrhi::BindingSetHandle psBindingSet;
    nvrhi::BindingLayoutHandle vsBindingLayout;
    nvrhi::BindingLayoutHandle psBindingLayout;
    xr_vector<nvrhi::SamplerHandle> samplers;
    xr_vector<ng::TextureHandle> textures;
    xr_vector<nvrhi::BufferHandle> constantBuffers;

    struct CBInfo {
        u32 slot;
        u32 size;
        xr_string name;
        bool isPerObject;
        bool isVertexShader;
        nvrhi::BufferHandle buffer;
    };
    xr_vector<CBInfo> cbInfos;
};
static xr_map<shared_str, ParticleBindingCache> s_bindingCache;

// ═══════════════════════════════════════════════════════
//  HELPER FUNCTIONS
// ═══════════════════════════════════════════════════════

static Fmatrix ApplyHUDFOVAdjustment(const Fmatrix& worldMatrix)
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

static void EnsureQuadIndexBuffer(ng::RenderDevice* device, u32 maxQuads) {
    if (s_quadIB && s_maxQuads >= maxQuads) {
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

    s_quadIB = device->GetNVRHIDevice()->createBuffer(ibDesc);
    if (!s_quadIB) {
        Msg("! [ParticlePass] ERROR: Failed to create quad index buffer");
        return;
    }

    nvrhi::CommandListHandle cmdList = device->GetNVRHIDevice()->createCommandList();
    cmdList->open();
    cmdList->writeBuffer(s_quadIB, indices.data(), indices.size() * sizeof(u16));
    cmdList->close();
    device->GetNVRHIDevice()->executeCommandList(cmdList);

    s_maxQuads = maxQuads;
}

static void EnsureParticleVertexBuffer(ng::RenderDevice* device, u32 sizeBytes) {
    if (s_particleVB && s_particleVBSize >= sizeBytes) {
        return;
    }

    u32 allocSize = ((sizeBytes + 65535) / 65536) * 65536;

    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = allocSize;
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "ParticleDynamicVB";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = false;

    s_particleVB = device->GetNVRHIDevice()->createBuffer(vbDesc);
    if (!s_particleVB) {
        Msg("! [ParticlePass] ERROR: Failed to create particle vertex buffer");
        return;
    }

    s_particleVBSize = allocSize;
}

// ═══════════════════════════════════════════════════════
//  BILLBOARD GENERATION
// ═══════════════════════════════════════════════════════

static void FillSprite(
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

// ═══════════════════════════════════════════════════════
//  PSO CREATION
// ═══════════════════════════════════════════════════════

static ng::PipelineState* GetOrCreateParticlePSO(
    RENDER_NAMESPACE::SPass* pass,
    const CPEDef* pDef,
    MaterialCache* materialCache,
    const DefaultOutputLayout& outputs,
    const FrameGraph& fg)
{
    if (!pass || !pDef)
        return nullptr;

    if (!pass->vs || !pass->ps)
        return nullptr;

    RENDER_NAMESPACE::SVS* vs = pass->vs._get();
    RENDER_NAMESPACE::SPS* ps = pass->ps._get();
    if (!vs || !ps)
        return nullptr;

    Msg("* [ParticlePass] Creating PSO for VS='%s' PS='%s'", vs->cName.c_str(), ps->cName.c_str());

    // Get or create NVRHI shader handles via MaterialCache
    nvrhi::ShaderHandle vsHandle = materialCache->GetOrCreateShaderVS(vs);
    if (!vsHandle) {
        Msg("! [ParticlePass] ERROR: Failed to get VS '%s'", vs->cName.c_str());
        return nullptr;
    }

    nvrhi::ShaderHandle psHandle = materialCache->GetOrCreateShaderPS(ps);
    if (!psHandle) {
        Msg("! [ParticlePass] ERROR: Failed to get PS '%s'", ps->cName.c_str());
        return nullptr;
    }

    ng::PipelineStateDesc psoDesc;

    xr_string debugNameStr = xr_string(vs->cName.c_str()) + "_" + xr_string(ps->cName.c_str());
    shared_str debugName = debugNameStr.c_str();
    psoDesc.debugName = debugName.c_str();

    psoDesc.vertexShader = vsHandle.Get();
    psoDesc.pixelShader = psHandle.Get();

    // Input Layout (ParticleVertex format)
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

    // Render Target Formats (Forward+ single-RT)
    nvrhi::ITexture* colorRT = fg.GetPhysicalTexture(outputs.albedo);
    nvrhi::ITexture* depthRT = fg.GetPhysicalTexture(outputs.depth);

    if (!colorRT || !depthRT) {
        Msg("! [ParticlePass] ERROR: Missing render targets for PSO creation");
        return nullptr;
    }

    psoDesc.renderTargetFormats[0] = colorRT->getDesc().format;
    psoDesc.depthStencilFormat = depthRT->getDesc().format;
    psoDesc.renderTargetCount = 1;

    // Extract render states from shader pass
    materialCache->SetupRenderStates(pass, psoDesc);

    // Override culling for particles
    psoDesc.rasterizerState.cullMode = ng::CullMode::None;
    if (pDef->m_Flags.is(CPEDef::dfCulling)) {
        if (pDef->m_Flags.is(CPEDef::dfCullCCW)) {
            psoDesc.rasterizerState.cullMode = ng::CullMode::Front;
        } else {
            psoDesc.rasterizerState.cullMode = ng::CullMode::Back;
        }
    }

    psoDesc.rasterizerState.scissorEnable = false;
    psoDesc.rasterizerState.multisampleEnable = false;
    psoDesc.rasterizerState.antialiasedLineEnable = false;
    psoDesc.primitiveTopology = ng::PrimitiveTopology::TriangleList;

    ng::PipelineState* pso = GEnv.FrameGraphRenderer->GetRenderDevice()->GetPipelineCache()->GetOrCreate(psoDesc);
    if (!pso) {
        Msg("! [ParticlePass] ERROR: Failed to create PSO for '%s'", vs->cName.c_str());
        return nullptr;
    }

    return pso;
}

// ═══════════════════════════════════════════════════════
//  BINDING SET CREATION
// ═══════════════════════════════════════════════════════

static ParticleBindingCache* CreateParticleBindingSet(
    RENDER_NAMESPACE::SPass* pass,
    ng::RenderDevice* device)
{
    if (!pass)
        return nullptr;

    RENDER_NAMESPACE::SVS* vs = pass->vs._get();
    RENDER_NAMESPACE::SPS* ps = pass->ps._get();
    if (!vs || !ps)
        return nullptr;

    Msg("  Shader reflection: VS=%p PS=%p", vs->reflection, ps->reflection);
    if (!vs->reflection || !ps->reflection) {
        Msg("  WARNING: Missing shader reflection!");
    }

    // Build cache key
    xr_string cacheKeyStr;
    cacheKeyStr.append(vs->cName.c_str());
    cacheKeyStr.append("|");
    cacheKeyStr.append(ps->cName.c_str());

    RENDER_NAMESPACE::STextureList* texList = pass->T._get();

    // Debug: Log texture list contents
    Msg("* [ParticlePass] CreateBindingSet for VS='%s' PS='%s'", vs->cName.c_str(), ps->cName.c_str());
    if (texList && !texList->empty()) {
        Msg("  Texture list has %zu entries:", texList->size());
        for (size_t i = 0; i < texList->size(); i++) {
            const auto& texPair = (*texList)[i];
            const shared_str& textureName = texPair.second;
            Msg("    [%u] slot=%u name='%s'", (u32)i, texPair.first, textureName.c_str() ? textureName.c_str() : "(null)");
            if (textureName.c_str() && textureName[0]) {
                cacheKeyStr.append("|");
                cacheKeyStr.append(textureName.c_str());
            }
        }
    } else {
        Msg("  WARNING: Texture list is EMPTY or NULL!");
    }

    shared_str cacheKey = cacheKeyStr.c_str();

    auto it = s_bindingCache.find(cacheKey);
    if (it != s_bindingCache.end()) {
        Msg("  [CACHE HIT] Returning cached binding set");
        return &it->second;
    }

    Msg("  [CACHE MISS] Creating new binding set...");
    ParticleBindingCache cacheEntry;

    // Analyze VS constant buffers
    const auto& vsCBs = ShaderReflector::GetConstantBuffers(vs->reflection);
    for (const auto& cbInfo : vsCBs.buffers) {
        bool isPerObject = (cbInfo.slot == 0);

        nvrhi::BufferDesc bufDesc;
        bufDesc.byteSize = cbInfo.size;
        bufDesc.isConstantBuffer = true;
        bufDesc.debugName = cbInfo.name.c_str();
        bufDesc.keepInitialState = false;
        bufDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        nvrhi::BufferHandle buffer = device->GetNVRHIDevice()->createBuffer(bufDesc);
        if (buffer) {
            ParticleBindingCache::CBInfo info;
            info.slot = cbInfo.slot;
            info.size = cbInfo.size;
            info.name = cbInfo.name.c_str();
            info.isPerObject = isPerObject;
            info.isVertexShader = true;
            info.buffer = buffer;
            cacheEntry.cbInfos.push_back(info);
            cacheEntry.constantBuffers.push_back(buffer);
        }
    }

    // Analyze PS constant buffers
    const auto& psCBs = ShaderReflector::GetConstantBuffers(ps->reflection);
    for (const auto& cbInfo : psCBs.buffers) {
        bool isPerObject = (cbInfo.slot == 0);

        nvrhi::BufferDesc bufDesc;
        bufDesc.byteSize = cbInfo.size;
        bufDesc.isConstantBuffer = true;
        bufDesc.debugName = cbInfo.name.c_str();
        bufDesc.keepInitialState = false;
        bufDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        nvrhi::BufferHandle buffer = device->GetNVRHIDevice()->createBuffer(bufDesc);
        if (buffer) {
            ParticleBindingCache::CBInfo info;
            info.slot = cbInfo.slot;
            info.size = cbInfo.size;
            info.name = cbInfo.name.c_str();
            info.isPerObject = isPerObject;
            info.isVertexShader = false;
            info.buffer = buffer;
            cacheEntry.cbInfos.push_back(info);
            cacheEntry.constantBuffers.push_back(buffer);
        }
    }

    // Create VS binding layout
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

    cacheEntry.vsBindingLayout = device->GetNVRHIDevice()->createBindingLayout(vsLayoutDesc);
    if (!cacheEntry.vsBindingLayout) {
        Msg("! [ParticlePass] ERROR: Failed to create VS binding layout");
        return nullptr;
    }

    // Create PS binding layout
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

    // For particle shaders, we only need s_base (slot 0) and smp_base (slot 0)
    // The soft particles use s_position at higher slots, but we skip those for forward rendering
    psLayoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::Texture_SRV(0));  // s_base at t0
    psLayoutDesc.bindings.push_back(nvrhi::BindingLayoutItem::Sampler(0));       // smp_base at s0

    cacheEntry.psBindingLayout = device->GetNVRHIDevice()->createBindingLayout(psLayoutDesc);
    if (!cacheEntry.psBindingLayout) {
        Msg("! [ParticlePass] ERROR: Failed to create PS binding layout");
        return nullptr;
    }

    // Create VS binding set
    nvrhi::BindingSetDesc vsBindingDesc;
    for (const auto& cbInfo : cacheEntry.cbInfos) {
        if (cbInfo.isVertexShader) {
            vsBindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.buffer));
        }
    }

    cacheEntry.vsBindingSet = device->GetNVRHIDevice()->createBindingSet(
        vsBindingDesc, cacheEntry.vsBindingLayout);
    if (!cacheEntry.vsBindingSet) {
        Msg("! [ParticlePass] ERROR: Failed to create VS binding set");
        return nullptr;
    }

    // Create PS binding set
    nvrhi::BindingSetDesc psBindingDesc;

    for (const auto& cbInfo : cacheEntry.cbInfos) {
        if (!cbInfo.isVertexShader) {
            psBindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.buffer));
        }
    }

    // Bind s_base texture at slot 0 (the first texture in the list is the particle texture)
    Msg("  Binding particle texture...");
    if (texList && !texList->empty()) {
        // Find the slot 0 texture (s_base - the particle texture)
        for (size_t i = 0; i < texList->size(); i++) {
            const auto& texPair = (*texList)[i];
            u32 slot = texPair.first;

            // Only bind slot 0 (s_base) for particle rendering
            if (slot != 0) continue;

            const shared_str& textureName = texPair.second;
            Msg("    Binding s_base texture: '%s'", textureName.c_str() ? textureName.c_str() : "(null)");

            RENDER_NAMESPACE::CTexture* xrayTex = (textureName.c_str() && textureName[0])
                ? RENDER_NAMESPACE::RImplementation.Resources->_CreateTexture(textureName.c_str())
                : nullptr;

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
                                texDesc.dimension = ng::RenderDevice::TextureDesc::Texture2D;
                                texDesc.format = ConvertDxgiFormatToNvrhi(desc.Format);
                                texDesc.isRenderTarget = false;
                                texDesc.isUAV = false;

                                tex2d->Release();
                            }
                        }

                        ng::TextureHandle nvrhiTex = device->CreateTextureFromD3D11(resource, texDesc);
                        resource->Release();

                        if (nvrhiTex.IsValid()) {
                            cacheEntry.textures.push_back(nvrhiTex);

                            nvrhi::ITexture* nativeTex = device->GetNativeTexture(nvrhiTex);
                            if (nativeTex) {
                                psBindingDesc.bindings.push_back(
                                    nvrhi::BindingSetItem::Texture_SRV(0, nativeTex));
                                Msg("      SUCCESS: Bound texture to slot 0");
                            } else {
                                Msg("      ERROR: GetNativeTexture returned null!");
                            }
                        } else {
                            Msg("      ERROR: CreateTextureFromD3D11 failed!");
                        }
                    } else {
                        Msg("      ERROR: Failed to get ID3D11Resource!");
                    }
                } else {
                    Msg("      ERROR: surface_get() returned null!");
                }
            } else {
                Msg("      ERROR: _CreateTexture returned null!");
            }
            break;  // Only bind slot 0
        }
    } else {
        Msg("  WARNING: No textures to bind!");
    }

    // Create sampler for s_base (slot 0)
    nvrhi::SamplerDesc samplerDesc;
    samplerDesc.setAllFilters(true);
    samplerDesc.setAllAddressModes(nvrhi::SamplerAddressMode::Wrap);
    samplerDesc.setMaxAnisotropy(8.0f);

    nvrhi::SamplerHandle sampler = device->GetNVRHIDevice()->createSampler(samplerDesc);
    if (sampler) {
        cacheEntry.samplers.push_back(sampler);
        psBindingDesc.bindings.push_back(nvrhi::BindingSetItem::Sampler(0, sampler));
        Msg("      SUCCESS: Created sampler at slot 0");
    }

    cacheEntry.psBindingSet = device->GetNVRHIDevice()->createBindingSet(
        psBindingDesc, cacheEntry.psBindingLayout);
    if (!cacheEntry.psBindingSet) {
        Msg("! [ParticlePass] ERROR: Failed to create PS binding set");
        return nullptr;
    }

    s_bindingCache[cacheKey] = std::move(cacheEntry);
    return &s_bindingCache[cacheKey];
}

// ═══════════════════════════════════════════════════════
//  RENDER PARTICLE EFFECT
// ═══════════════════════════════════════════════════════

static bool RenderParticleEffect(
    ng::RenderContext* ctx,
    ng::RenderDevice* device,
    MaterialCache* materialCache,
    const ParticleBatch& batch,
    bool applyFOV,
    const DefaultOutputLayout& outputs,
    const FrameGraph& fg)
{
    CParticleEffect* pEffect = static_cast<CParticleEffect*>(batch.visual);

    auto* pDef = pEffect->GetDefinition();
    if (!pDef || !pDef->m_Flags.is(CPEDef::dfSprite)) {
        Msg("* [ParticlePass] Skipping non-sprite particle effect");
        return false;
    }

    Msg("* [ParticlePass] Rendering particle effect: shader='%s' texture='%s'",
        pDef->m_ShaderName.c_str() ? pDef->m_ShaderName.c_str() : "(null)",
        pDef->m_TextureName.c_str() ? pDef->m_TextureName.c_str() : "(null)");

    PAPI::Particle* particles = nullptr;
    u32 particleCount = 0;
    PAPI::ParticleManager()->GetParticles(pEffect->GetHandleEffect(), particles, particleCount);

    if (particleCount == 0 || !particles) {
        return false;
    }

    // Allocate buffers
    u32 requiredVBSize = particleCount * 4 * sizeof(ParticleVertex);
    EnsureParticleVertexBuffer(device, requiredVBSize);
    EnsureQuadIndexBuffer(device, particleCount);

    if (!s_particleVB || !s_quadIB) {
        return false;
    }

    // Generate billboard geometry
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

        if (pDef->m_Flags.is(CPEDef::dfFramed)) {
            pDef->m_Frame.CalculateTC(iFloor(float(m.frame) / 255.f), lt, rb);
        }

        float r_x = m.size.x * 0.5f;
        float r_y = m.size.y * 0.5f;

        if (pDef->m_Flags.is(CPEDef::dfVelocityScale)) {
            float speed = m.vel.magnitude();
            r_x += speed * pDef->m_VelocityScale.x;
            r_y += speed * pDef->m_VelocityScale.y;
        }

        FillSprite(pv, Device.vCameraTop, Device.vCameraRight, m.pos, lt, rb, r_x, r_y, m.color, sina, cosa);
    }

    ctx->WriteBuffer(s_particleVB.Get(), vertices.data(), vertices.size() * sizeof(ParticleVertex));

    // Get shader
    RENDER_NAMESPACE::Shader* particleShader = pDef->m_CachedShader._get();
    if (!particleShader) {
        return false;
    }

    RENDER_NAMESPACE::ShaderElement* element = particleShader->E[0]._get();
    if (!element || element->passes.empty()) {
        return false;
    }

    RENDER_NAMESPACE::SPass* pass = element->passes[0]._get();
    if (!pass) {
        return false;
    }

    // Get PSO
    ng::PipelineState* particlePSO = GetOrCreateParticlePSO(pass, pDef, materialCache, outputs, fg);
    if (!particlePSO) {
        return false;
    }

    // Get binding sets
    ParticleBindingCache* bindingCache = CreateParticleBindingSet(pass, device);
    if (!bindingCache) {
        return false;
    }

    // Bind pipeline
    ctx->SetPipeline(particlePSO->GetNativePipeline());

    // Update constant buffers
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
            ctx->WriteBuffer(cbInfo.buffer.Get(), &staticGlobalsCB, sizeToWrite);
        }
        else if (cbInfo.name == "dynamic_transforms") {
            u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
            ctx->WriteBuffer(cbInfo.buffer.Get(), &dynamicTransformsCB, sizeToWrite);
        }
        else if (cbInfo.isPerObject && cbInfo.isVertexShader) {
            u32 sizeToWrite = std::min<u32>(sizeof(ParticlePerFrame), cbInfo.size);
            ctx->WriteBuffer(cbInfo.buffer.Get(), &perFrameCB, sizeToWrite);
        }
    }

    // Bind resources
    ctx->SetBindingSet(0, bindingCache->vsBindingSet.Get());
    ctx->SetBindingSet(1, bindingCache->psBindingSet.Get());

    ctx->SetVertexBuffer(0, s_particleVB.Get(), 0);
    ctx->SetIndexBuffer(s_quadIB.Get(), nvrhi::Format::R16_UINT, 0);

    // Draw
    u32 indexCount = particleCount * 6;
    ctx->DrawIndexed(indexCount, 0, 0);

    return true;
}

// ═══════════════════════════════════════════════════════
//  SETUP PARTICLE PASS
// ═══════════════════════════════════════════════════════

DefaultOutputLayout setupParticlePass(
    FrameGraph& fg,
    ng::RenderDevice* device,
    const DefaultOutputLayout& forwardInputs,
    const xr_vector<ParticleBatch>* worldParticleBatches,
    const xr_vector<ParticleBatch>* hudParticleBatches,
    MaterialCache* materialCache,
    u32 width,
    u32 height)
{
    struct ParticlePassData {
        VirtualResourceHandle inputColor;
        VirtualResourceHandle depth;
        VirtualResourceHandle outputColor;

        ng::RenderDevice* device;
        const xr_vector<ParticleBatch>* worldParticleBatches;
        const xr_vector<ParticleBatch>* hudParticleBatches;
        MaterialCache* materialCache;
        DefaultOutputLayout outputs;
        u32 width;
        u32 height;
    };

    auto& passData = fg.addCallbackPass<ParticlePassData>(
        "Particles",

        // Setup lambda
        [&, width, height](FrameGraph& builder, PassHandle passHandle, ParticlePassData& data) {
            RenderPassBuilder passBuilder(builder, passHandle);

            data.width = width;
            data.height = height;
            data.device = device;
            data.worldParticleBatches = worldParticleBatches;
            data.hudParticleBatches = hudParticleBatches;
            data.materialCache = materialCache;

            // Read color input (from Forward+/HUD)
            data.inputColor = passBuilder.read(forwardInputs.albedo);

            // Write to same target (particles render on top)
            data.outputColor = passBuilder.write(forwardInputs.albedo, ResourceState::RenderTarget);

            // Read-write depth (for depth testing)
            data.depth = passBuilder.readWrite(forwardInputs.depth, ResourceState::DepthStencilWrite);

            data.outputs.albedo = data.outputColor;
            data.outputs.depth = data.depth;
        },

        // Execute lambda
        [](const ParticlePassData& data,
           const FrameGraph& fg,
           ng::RenderContext* ctx) {

            nvrhi::ICommandList* cmdList = ctx->GetCommandList();
            cmdList->beginMarker("Particle Pass");

            u32 totalWorld = data.worldParticleBatches ? (u32)data.worldParticleBatches->size() : 0;
            u32 totalHUD = data.hudParticleBatches ? (u32)data.hudParticleBatches->size() : 0;

            Msg("* [ParticlePass] Execute: %u world batches, %u HUD batches", totalWorld, totalHUD);

            if (totalWorld == 0 && totalHUD == 0) {
                cmdList->endMarker();
                return;
            }

            auto* colorRT = fg.GetPhysicalTexture(data.outputColor);
            auto* depthRT = fg.GetPhysicalTexture(data.depth);

            if (!colorRT || !depthRT) {
                Msg("! [ParticlePass] Failed to get physical textures");
                cmdList->endMarker();
                return;
            }

            // Setup render pass (no clear - particles render on top)
            ng::RenderPassDesc passDesc;
            passDesc.passName = "Particle Pass";
            passDesc.renderTargets[0] = colorRT;
            passDesc.numRenderTargets = 1;
            passDesc.depthStencil = depthRT;
            passDesc.clearColor = false;
            passDesc.clearDepth = false;
            passDesc.clearStencil = false;

            ctx->BeginRenderPass(passDesc);

            ng::Viewport viewport;
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)data.width;
            viewport.height = (float)data.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            ctx->SetViewport(viewport);

            ng::Rect scissor;
            scissor.x = 0;
            scissor.y = 0;
            scissor.width = data.width;
            scissor.height = data.height;
            ctx->SetScissor(scissor);

            u32 numDraws = 0;

            // Render world particles
            if (data.worldParticleBatches && !data.worldParticleBatches->empty()) {
                cmdList->beginMarker("World Particles");
                for (const auto& batch : *data.worldParticleBatches) {
                    if (batch.visual && batch.visual->getType() == MT_PARTICLE_EFFECT) {
                        if (RenderParticleEffect(ctx, data.device, data.materialCache,
                                                 batch, false, data.outputs, fg)) {
                            numDraws++;
                        }
                    }
                }
                cmdList->endMarker();
            }

            // Render HUD particles (with FOV adjustment)
            if (data.hudParticleBatches && !data.hudParticleBatches->empty()) {
                cmdList->beginMarker("HUD Particles");
                for (const auto& batch : *data.hudParticleBatches) {
                    if (batch.visual && batch.visual->getType() == MT_PARTICLE_EFFECT) {
                        if (RenderParticleEffect(ctx, data.device, data.materialCache,
                                                 batch, true, data.outputs, fg)) {
                            numDraws++;
                        }
                    }
                }
                cmdList->endMarker();
            }

            ctx->EndRenderPass();

            if (numDraws > 0) {
                Msg("* [ParticlePass] Rendered %u particle effects", numDraws);
            }

            cmdList->endMarker();
        }
    );

    DefaultOutputLayout outputs;
    outputs.albedo = passData.outputColor;
    outputs.depth = passData.depth;
    return outputs;
}

} // namespace xray::render::RENDER_NAMESPACE::passes
