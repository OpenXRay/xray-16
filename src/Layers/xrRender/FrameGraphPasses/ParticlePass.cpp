// xrRender/FrameGraphPasses/ParticlePass.cpp
#include "stdafx.h"
#include "ParticlePass.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"  // For CB analysis
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/RenderContext/RenderStateConversion.h"  // State conversion helpers
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
#include "../Externals/nvrhi/src/common/dxgi-format.h"  // For DXGI <-> NVRHI format conversion
#endif

extern ENGINE_API float psHUD_FOV;

namespace xray::render::passes {

using namespace framegraph;
using RENDER_NAMESPACE::PS::CParticleEffect;
using RENDER_NAMESPACE::PS::CParticleGroup;

// Apply HUD FOV adjustment to world matrix (same as HUDPass)
// Based on Unreal Engine's viewmodel FOV technique:
// AdjustedWorld = View^-1 * FOVScale * View * World
Fmatrix ParticlePass::ApplyHUDFOVAdjustment(const Fmatrix& worldMatrix)
{
    // FOV scale factor (psHUD_FOV = 0.45 default, range 0.1 to 1.0)
    // Lower psHUD_FOV = narrower FOV = weapon appears larger/closer
    // So we INVERT: scale = 1.0 / psHUD_FOV
    float fovScale = 1.0f / psHUD_FOV;

    // Get view and inverse view matrices
    Fmatrix viewMatrix = Device.mView;
    Fmatrix invView;
    invView.invert(viewMatrix);

    // Create FOV scale matrix in view space
    // Scale X and Y (perspective components), but not Z (depth) or translation
    Fmatrix fovScaleMatrix;
    fovScaleMatrix.identity();
    fovScaleMatrix._11 = fovScale;  // Scale X
    fovScaleMatrix._22 = fovScale;  // Scale Y
    fovScaleMatrix._33 = 1.0f;      // Don't scale Z (depth)

    // Apply transformation: World -> View -> Scale -> World
    // Result = V^-1 * S * V * W
    Fmatrix temp1, temp2, result;
    temp1.mul(viewMatrix, worldMatrix);       // V * W
    temp2.mul(fovScaleMatrix, temp1);         // S * V * W
    result.mul(invView, temp2);               // V^-1 * S * V * W

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
    // Shader handles are now managed by MaterialCache, no cleanup needed here
    // Buffers will be automatically released by nvrhi smart pointers
}

void ParticlePass::Setup(FrameGraph& fg) {
    // Particles render to the same G-Buffer targets as world geometry + HUD
    // This allows particles to participate in deferred lighting

    // Register pass with framegraph
    PassIO io;

    // Particles read from depth to do depth testing against world+HUD geometry
    // This establishes dependency: Particles must execute AFTER HUD
    io.reads.push_back({m_outputs.depth, ResourceState::DepthStencilRead});

    // Particles write to all G-Buffer targets (render on top of world+HUD)
    io.writes.push_back({m_outputs.albedo, ResourceState::RenderTarget});
    io.writes.push_back({m_outputs.normal, ResourceState::RenderTarget});
    io.writes.push_back({m_outputs.material, ResourceState::RenderTarget});
    io.writes.push_back({m_outputs.depth, ResourceState::DepthStencilWrite});

    RegisterPass(fg, "Particles", io);

    Msg("  ✓ Particle pass configured");
}

// ═══════════════════════════════════════════════════════
//  BUFFER MANAGEMENT
// ═══════════════════════════════════════════════════════

void ParticlePass::EnsureQuadIndexBuffer(u32 maxQuads) {
    if (m_quadIB && m_maxQuads >= maxQuads) {
        return;  // Already large enough
    }

    // Generate quad indices: each quad = 6 indices (2 triangles)
    // Pattern: 0,1,2, 2,1,3 for each quad
    u32 numIndices = maxQuads * 6;
    xr_vector<u16> indices;
    indices.reserve(numIndices);

    for (u32 i = 0; i < maxQuads; i++) {
        u16 baseVertex = (u16)(i * 4);
        // Triangle 1: 0,1,2
        indices.push_back(baseVertex + 0);
        indices.push_back(baseVertex + 1);
        indices.push_back(baseVertex + 2);
        // Triangle 2: 2,1,3
        indices.push_back(baseVertex + 2);
        indices.push_back(baseVertex + 1);
        indices.push_back(baseVertex + 3);
    }

    // Create NVRHI index buffer
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

    // Upload index data
    nvrhi::CommandListHandle cmdList = m_device->GetNVRHIDevice()->createCommandList();
    cmdList->open();
    cmdList->writeBuffer(m_quadIB, indices.data(), indices.size() * sizeof(u16));
    cmdList->close();
    m_device->GetNVRHIDevice()->executeCommandList(cmdList);

    m_maxQuads = maxQuads;
}

void ParticlePass::EnsureParticleVertexBuffer(u32 sizeBytes) {
    if (m_particleVB && m_particleVBSize >= sizeBytes) {
        return;  // Already large enough
    }

    // Round up to 64KB chunks for efficiency
    u32 allocSize = ((sizeBytes + 65535) / 65536) * 65536;

    // Create vertex buffer (NVRHI will handle uploads via staging internally)
    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = allocSize;
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "ParticleDynamicVB";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.keepInitialState = false;  // Allow state transitions
    // Note: Don't set cpuAccess - let NVRHI use staging buffers via writeBuffer()

    m_particleVB = m_device->GetNVRHIDevice()->createBuffer(vbDesc);
    if (!m_particleVB) {
        Msg("! [ParticlePass] ERROR: Failed to create particle vertex buffer");
        return;
    }

    m_particleVBSize = allocSize;
}

// ═══════════════════════════════════════════════════════
//  BILLBOARD GENERATION (ported from ParticleEffect.cpp)
// ═══════════════════════════════════════════════════════

void ParticlePass::FillSprite(
    ParticleVertex*& pv,
    const Fvector& T, const Fvector& R,
    const Fvector& pos,
    const Fvector2& lt, const Fvector2& rb,
    float r1, float r2,
    u32 clr,
    float sina, float cosa)
{
    // Calculate rotated basis vectors
    Fvector Vr, Vt;

    Vr.x = T.x * r1 * sina + R.x * r1 * cosa;
    Vr.y = T.y * r1 * sina + R.y * r1 * cosa;
    Vr.z = T.z * r1 * sina + R.z * r1 * cosa;

    Vt.x = T.x * r2 * cosa - R.x * r2 * sina;
    Vt.y = T.y * r2 * cosa - R.y * r2 * sina;
    Vt.z = T.z * r2 * cosa - R.z * r2 * sina;

    // Calculate quad corners
    Fvector a, b, c, d;

    a.sub(Vt, Vr);
    b.add(Vt, Vr);

    c.invert(a);
    d.invert(b);

    // Vertex 0: bottom-left
    pv->p.set(d.x + pos.x, d.y + pos.y, d.z + pos.z);
    pv->color = clr;
    pv->t.set(lt.x, rb.y);
    pv++;

    // Vertex 1: top-left
    pv->p.set(a.x + pos.x, a.y + pos.y, a.z + pos.z);
    pv->color = clr;
    pv->t.set(lt.x, lt.y);
    pv++;

    // Vertex 2: bottom-right
    pv->p.set(c.x + pos.x, c.y + pos.y, c.z + pos.z);
    pv->color = clr;
    pv->t.set(rb.x, rb.y);
    pv++;

    // Vertex 3: top-right
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
    // T = direction, R = perpendicular to direction and camera
    const Fvector& T = dir;

    Fvector R;
    R.crossproduct(T, Device.vCameraDirection).normalize_safe();

    // Call the full version
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

    // Get physical render targets
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(m_outputs.normal);
    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(m_outputs.albedo);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(m_outputs.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(m_outputs.depth);

    if (!normal || !albedo || !material || !depth) {
        Msg("! [ParticlePass] ERROR: Missing render targets!");
        return;
    }

    // Setup render pass (no clear - particles render on top of world+HUD)
    ng::RenderPassDesc passDesc;
    passDesc.renderTargets[0] = normal;
    passDesc.renderTargets[1] = albedo;
    passDesc.renderTargets[2] = material;
    passDesc.numRenderTargets = 3;
    passDesc.depthStencil = depth;
    passDesc.clearColor = false;  // Don't clear - render on top
    passDesc.clearDepth = false;
    passDesc.clearStencil = false;

    ctx.BeginRenderPass(passDesc);

    // Set viewport (full screen)
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

    // ═══════════════════════════════════════════════════════
    //  RENDER WORLD PARTICLES
    // ═══════════════════════════════════════════════════════

    if (m_worldParticleBatches && !m_worldParticleBatches->empty()) {

        // TODO: Port particle rendering from legacy CBackend to NVRHI
        // Original implementation in ParticleEffect::Render() uses:
        //   - RImplementation.Vertex.Lock() for dynamic vertex buffers
        //   - cmd_list.set_xform_world(Fidentity)
        //   - cmd_list.set_Geometry(geom)
        //   - cmd_list.Render(D3DPT_TRIANGLELIST, ...)
        //
        // For framegraph, we need to:
        //   1. Create dynamic vertex buffer via ctx.WriteBuffer()
        //   2. Generate billboard quads in ParticleRenderStream()
        //   3. Bind particle shader PSO
        //   4. ctx.DrawIndexed() for each batch

        for (const auto& batch : *m_worldParticleBatches) {
            if (RenderParticleSystem(ctx, batch, false, fg)) {
                m_stats.numBatches++;
                m_stats.numDrawCalls++;
            }
        }
    }

    // ═══════════════════════════════════════════════════════
    //  RENDER HUD PARTICLES (with FOV adjustment)
    // ═══════════════════════════════════════════════════════

    if (m_hudParticleBatches && !m_hudParticleBatches->empty()) {

        // TODO: Apply hud_transform_helper FOV adjustment
        // Original engine temporarily modifies Device.mProject:
        //   Device.mProject.build_projection(deg2rad(psHUD_FOV * Device.fFOV), ...)
        //   cmd_list.set_xform_project(Device.mProject)
        //
        // For framegraph, we should:
        //   1. Use ApplyHUDFOVAdjustment() on world matrix (like HUDPass)
        //   2. Update per-draw constant buffers with adjusted matrix
        //   3. Render with same technique as world particles

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

    // Validate shaders
    if (!pass->vs || !pass->ps)
        return nullptr;

    RENDER_NAMESPACE::SVS* vs = pass->vs._get();
    RENDER_NAMESPACE::SPS* ps = pass->ps._get();
    if (!vs || !ps)
        return nullptr;

    // Check if we have bytecode
    if (!vs->bytecode || !ps->bytecode)
        return nullptr;

    // ═══════════════════════════════════════════════════════
    //  CREATE PARTICLE PSO
    // ═══════════════════════════════════════════════════════

    ng::PipelineStateDesc psoDesc;

    // Build unique debug name from BOTH shaders to avoid cache collisions
    // Store in shared_str so it persists beyond this scope
    xr_string debugNameStr = xr_string(vs->cName.c_str()) + "_" + xr_string(ps->cName.c_str());
    shared_str debugName = debugNameStr.c_str();
    psoDesc.debugName = debugName.c_str();

    // ─── Get or Create Cached Shaders (via MaterialCache) ───
    nvrhi::ShaderHandle vsHandle = m_materialCache->GetOrCreateShaderVS(vs);
    if (!vsHandle) {
        Msg("! [ParticlePass] ERROR: Failed to get VS '%s'", vs->cName.c_str());
        return nullptr;
    }

    nvrhi::ShaderHandle psHandle = m_materialCache->GetOrCreateShaderPS(ps);
    if (!psHandle) {
        Msg("! [ParticlePass] ERROR: Failed to get PS '%s'", ps->cName.c_str());
        return nullptr;
    }



    // Assign direct NVRHI shader pointers (no wrappers!)
    psoDesc.vertexShader = vsHandle.Get();
    psoDesc.pixelShader = psHandle.Get();

    // ─── Input Layout (FVF::LIT format) ───
    // ParticleVertex: position (float3), color (uint32), texcoord (float2)
    psoDesc.vertexAttributes.clear();

    // Position (POSITION semantic)
    ng::VertexAttribute posAttr;
    posAttr.semanticName = "POSITION";
    posAttr.semanticIndex = 0;
    posAttr.format = nvrhi::Format::RGB32_FLOAT;
    posAttr.offset = 0;
    posAttr.bufferIndex = 0;
    posAttr.elementStride = sizeof(ParticleVertex);
    psoDesc.vertexAttributes.push_back(posAttr);

    // Color (COLOR semantic)
    ng::VertexAttribute colorAttr;
    colorAttr.semanticName = "COLOR";
    colorAttr.semanticIndex = 0;
    colorAttr.format = nvrhi::Format::RGBA8_UNORM;  // u32 color -> RGBA8
    colorAttr.offset = 12;  // After position (3 floats)
    colorAttr.bufferIndex = 0;
    colorAttr.elementStride = sizeof(ParticleVertex);
    psoDesc.vertexAttributes.push_back(colorAttr);

    // Texcoord (TEXCOORD semantic)
    ng::VertexAttribute texAttr;
    texAttr.semanticName = "TEXCOORD";
    texAttr.semanticIndex = 0;
    texAttr.format = nvrhi::Format::RG32_FLOAT;
    texAttr.offset = 16;  // After position + color
    texAttr.bufferIndex = 0;
    texAttr.elementStride = sizeof(ParticleVertex);
    psoDesc.vertexAttributes.push_back(texAttr);

    // ─── Render Target Formats ───
    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(m_outputs.albedo);
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(m_outputs.normal);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(m_outputs.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(m_outputs.depth);

    psoDesc.renderTargetFormats[0] = albedo->getDesc().format;
    psoDesc.renderTargetFormats[1] = normal->getDesc().format;
    psoDesc.renderTargetFormats[2] = material->getDesc().format;
    psoDesc.depthStencilFormat = depth->getDesc().format;
    psoDesc.renderTargetCount = 3;

    // ─── Extract Render States from Shader Pass ───
    // Use MaterialCache to extract blend, depth, and rasterizer state from pass->state
    // This fixes the blend state bug (was hardcoded to alpha blending, should be additive from shader)
    m_materialCache->SetupRenderStates(pass, psoDesc);

    // ─── Override Culling for Particles ───
    // Particles have special culling requirements based on CPEDef flags, override after extraction
    psoDesc.rasterizerState.cullMode = ng::CullMode::None;  // Default: double-sided
    if (pDef->m_Flags.is(RENDER_NAMESPACE::PS::CPEDef::dfCulling)) {
        if (pDef->m_Flags.is(RENDER_NAMESPACE::PS::CPEDef::dfCullCCW)) {
            psoDesc.rasterizerState.cullMode = ng::CullMode::Front;
        } else {
            psoDesc.rasterizerState.cullMode = ng::CullMode::Back;
        }
    }

    // ─── Configure RT Write Masks for Particles ───
    psoDesc.blendState.renderTargets[0].writeMask = ng::ColorWriteMask::None;  // Disable normal
    psoDesc.blendState.renderTargets[2].writeMask = ng::ColorWriteMask::None;  // Disable material
    psoDesc.rasterizerState.scissorEnable = false;
    psoDesc.rasterizerState.multisampleEnable = false;
    psoDesc.rasterizerState.antialiasedLineEnable = false;

    // ─── Topology ───
    psoDesc.primitiveTopology = ng::PrimitiveTopology::TriangleList;

    // ─── Create PSO via Pipeline Cache ───
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

    // Generate cache key from BOTH shaders + texture list (like MaterialCache does)
    RENDER_NAMESPACE::SVS* vs = pass->vs._get();
    RENDER_NAMESPACE::SPS* ps = pass->ps._get();
    if (!vs || !ps)
        return nullptr;

    // Validate extracted reflection exists
    if (!vs->reflection || !ps->reflection)
        return nullptr;

    // Build cache key: "VS_name|PS_name|tex0|tex1|tex2..."
    xr_string cacheKeyStr;
    cacheKeyStr.append(vs->cName.c_str());
    cacheKeyStr.append("|");
    cacheKeyStr.append(ps->cName.c_str());

    // Add texture names to key (so different textures = different cache entry)
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

    // ═══════════════════════════════════════════════════════
    //  CHECK CACHE - Return existing binding cache if found
    // ═══════════════════════════════════════════════════════

    auto it = m_bindingCache.find(cacheKey);
    if (it != m_bindingCache.end()) {
        return &it->second;  // Return pointer to cached entry
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE NEW BINDING CACHE ENTRY
    // ═══════════════════════════════════════════════════════

    ParticlePass::ParticleBindingCache cacheEntry;

    // ═══════════════════════════════════════════════════════
    //  ANALYZE SHADER CONSTANT BUFFERS (like MaterialCache)
    // ═══════════════════════════════════════════════════════


    // Get VS constant buffers from extracted reflection
    const auto& vsCBs = framegraph::ShaderReflector::GetConstantBuffers(vs->reflection);
    for (const auto& cbInfo : vsCBs.buffers) {
        // Determine if this is a per-object CB (slot 0) or global (slot 1+)
        bool isPerObject = (cbInfo.slot == 0);

        // Create NVRHI buffer
        nvrhi::BufferDesc bufDesc;
        bufDesc.byteSize = cbInfo.size;
        bufDesc.isConstantBuffer = true;
        bufDesc.debugName = cbInfo.name.c_str();
        bufDesc.keepInitialState = false;
        bufDesc.initialState = nvrhi::ResourceStates::ConstantBuffer;

        nvrhi::BufferHandle buffer = m_device->GetNVRHIDevice()->createBuffer(bufDesc);
        if (buffer) {
            // Store CB info
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

    // Get PS constant buffers from extracted reflection
    const auto& psCBs = framegraph::ShaderReflector::GetConstantBuffers(ps->reflection);
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

    // ═══════════════════════════════════════════════════════
    //  CREATE VS BINDING LAYOUT (CBs only)
    // ═══════════════════════════════════════════════════════

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

    // ═══════════════════════════════════════════════════════
    //  CREATE PS BINDING LAYOUT (CBs + Textures + Samplers)
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingLayoutDesc psLayoutDesc;
    psLayoutDesc.visibility = nvrhi::ShaderType::Pixel;
    psLayoutDesc.registerSpace = 0;

    // Add PS constant buffers
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

    // Add texture slots (t0, t1, t2, etc.)
    u32 numTextures = 0;
    if (texList && !texList->empty()) {
        numTextures = (u32)texList->size();
        for (u32 i = 0; i < numTextures; i++) {
            psLayoutDesc.bindings.push_back(
                nvrhi::BindingLayoutItem::Texture_SRV(i));
        }
    }

    // Add sampler slots (s0, s1, s2, etc.)
    for (u32 i = 0; i < numTextures; i++) {
        psLayoutDesc.bindings.push_back(
            nvrhi::BindingLayoutItem::Sampler(i));
    }

    cacheEntry.psBindingLayout = m_device->GetNVRHIDevice()->createBindingLayout(psLayoutDesc);
    if (!cacheEntry.psBindingLayout) {
        Msg("! [ParticlePass] ERROR: Failed to create PS binding layout");
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE VS BINDING SET (CBs only)
    // ═══════════════════════════════════════════════════════

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

    // ═══════════════════════════════════════════════════════
    //  CREATE PS BINDING SET (CBs + Textures + Samplers)
    // ═══════════════════════════════════════════════════════

    nvrhi::BindingSetDesc psBindingDesc;

    // Bind PS constant buffers
    for (const auto& cbInfo : cacheEntry.cbInfos) {
        if (!cbInfo.isVertexShader) {
            psBindingDesc.bindings.push_back(
                nvrhi::BindingSetItem::ConstantBuffer(cbInfo.slot, cbInfo.buffer));
        }
    }

    // Bind textures (same as before, but now in PS binding set)
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

    // Create samplers
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

    // Store cache entry and return pointer to it
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

    // ═══════════════════════════════════════════════════════
    //  PARTICLE EFFECTS (CParticleEffect)
    // ═══════════════════════════════════════════════════════
    if (vType == MT_PARTICLE_EFFECT) {
        CParticleEffect* pEffect = static_cast<CParticleEffect*>(batch.visual);

        // Check if this is a sprite-based particle (billboards)
        auto* pDef = pEffect->GetDefinition();
        if (!pDef || !pDef->m_Flags.is(RENDER_NAMESPACE::PS::CPEDef::dfSprite)) {
            return false;  // Not a sprite particle
        }

        // Get particle data from ParticleManager
        PAPI::Particle* particles = nullptr;
        u32 particleCount = 0;
        PAPI::ParticleManager()->GetParticles(pEffect->GetHandleEffect(), particles, particleCount);

        if (particleCount == 0 || !particles) {
            return false;  // No particles to render
        }

        // ═══════════════════════════════════════════════════════
        //  ALLOCATE VERTEX BUFFER SPACE
        // ═══════════════════════════════════════════════════════

        // Each particle = 4 vertices * 24 bytes = 96 bytes
        u32 requiredVBSize = particleCount * 4 * sizeof(ParticleVertex);
        EnsureParticleVertexBuffer(requiredVBSize);
        EnsureQuadIndexBuffer(particleCount);

        if (!m_particleVB || !m_quadIB) {
            Msg("! [ParticlePass] ERROR: Failed to create particle buffers");
            return false;
        }

        // ═══════════════════════════════════════════════════════
        //  GENERATE BILLBOARD GEOMETRY
        // ═══════════════════════════════════════════════════════

        // Allocate CPU-side buffer for billboards
        xr_vector<ParticleVertex> vertices;
        vertices.resize(particleCount * 4);
        ParticleVertex* pv = vertices.data();

        // Generate billboards (simplified version of ParticleRenderStream)
        float sina = 0.0f, cosa = 0.0f;
        float angle = float(0xFFFFFFFF);

        for (u32 i = 0; i < particleCount; i++) {
            auto& m = particles[i];

            // Calculate sin/cos if angle changed
            if (angle != m.rot.x) {
                angle = m.rot.x;
                sina = _sin(angle);
                cosa = _cos(angle);
            }

            // Texture coordinates
            Fvector2 lt, rb;
            lt.set(0.f, 0.f);
            rb.set(1.f, 1.f);

            if (pDef->m_Flags.is(RENDER_NAMESPACE::PS::CPEDef::dfFramed)) {
                pDef->m_Frame.CalculateTC(iFloor(float(m.frame) / 255.f), lt, rb);
            }

            // Billboard size
            float r_x = m.size.x * 0.5f;
            float r_y = m.size.y * 0.5f;

            // Velocity scaling (optional)
            if (pDef->m_Flags.is(RENDER_NAMESPACE::PS::CPEDef::dfVelocityScale)) {
                float speed = m.vel.magnitude();
                r_x += speed * pDef->m_VelocityScale.x;
                r_y += speed * pDef->m_VelocityScale.y;
            }

            // Generate billboard quad
            // For now, use simple camera-facing billboards
            // TODO: Add support for dfAlignToPath and other alignment modes
            FillSprite(pv, Device.vCameraTop, Device.vCameraRight, m.pos, lt, rb, r_x, r_y, m.color, sina, cosa);
        }

        // ═══════════════════════════════════════════════════════
        //  UPLOAD VERTEX DATA
        // ═══════════════════════════════════════════════════════

        ctx.WriteBuffer(m_particleVB.Get(), vertices.data(), vertices.size() * sizeof(ParticleVertex));

        // ═══════════════════════════════════════════════════════
        //  GET PARTICLE SHADER
        // ═══════════════════════════════════════════════════════

        // Particles store their shader in the effect definition
        RENDER_NAMESPACE::Shader* particleShader = pDef->m_CachedShader._get();

        if (!particleShader) {
            Msg("! [ParticlePass] WARNING: No shader for particle effect '%s'", pDef->m_Name.c_str());
            return false;
        }

        // Get first element and first pass
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

        // ═══════════════════════════════════════════════════════
        //  GET OR CREATE PSO
        // ═══════════════════════════════════════════════════════

        ng::PipelineState* particlePSO = GetOrCreateParticlePSO(pass, pDef, fg);

        if (!particlePSO) {
            return false;
        }


        // ═══════════════════════════════════════════════════════
        //  CREATE BINDING SETS (VS + PS with CBs, textures, samplers)
        // ═══════════════════════════════════════════════════════

        ParticlePass::ParticleBindingCache* bindingCache = CreateParticleBindingSet(pass);
        if (!bindingCache) {
            Msg("! [ParticlePass] WARNING: Failed to create binding sets for effect '%s'", pDef->m_Name.c_str());
            return false;
        }

        // ═══════════════════════════════════════════════════════
        //  BIND PIPELINE FIRST (before any resource bindings!)
        // ═══════════════════════════════════════════════════════
        // CRITICAL: SetPipeline() must come BEFORE resource bindings!
        // Setting a pipeline clears previously bound resources, so we must:
        // 1. SetPipeline() FIRST
        // 2. Update constant buffers
        // 3. Bind binding sets
        // 4. Bind vertex/index buffers
        // 5. Draw

        nvrhi::IGraphicsPipeline* nativePipeline = particlePSO->GetNativePipeline();
        ctx.SetPipeline(nativePipeline);

        // ═══════════════════════════════════════════════════════
        //  UPDATE CONSTANT BUFFERS (after pipeline is set!)
        // ═══════════════════════════════════════════════════════

        // Fill global constant data
        StaticGlobals staticGlobalsCB = {};
        FillGlobalConstants(staticGlobalsCB);

        // Fill per-object transform data
        DynamicTransforms dynamicTransformsCB = {};
        FillDynamicTransforms(dynamicTransformsCB);

        // Fill per-object CB slot 0 for VS (mVPTexgen matrix)
        // Particle shaders use mVPTexgen to transform world-space positions to clip space
        // This is the View-Projection matrix
        struct ParticlePerFrame {
            Fmatrix mVPTexgen;  // VP matrix for transforming particles to clip space
        };
        ParticlePerFrame perFrameCB = {};

        // Compute View-Projection matrix (same as vanilla X-Ray does for particles)
        Fmatrix mVP;
        mVP.mul(Device.mProject, Device.mView);

        // HLSL expects row-major, X-Ray stores column-major, so transpose
        perFrameCB.mVPTexgen.transpose(mVP);

        // Update all constant buffers (pipeline is already set, so these will stick)
        for (const auto& cbInfo : bindingCache->cbInfos) {
            // Determine which data to write based on CB name
            if (cbInfo.name == "static_globals") {
                u32 sizeToWrite = std::min<u32>(sizeof(StaticGlobals), cbInfo.size);
                ctx.WriteBuffer(cbInfo.buffer.Get(), &staticGlobalsCB, sizeToWrite);
            }
            else if (cbInfo.name == "dynamic_transforms") {
                u32 sizeToWrite = std::min<u32>(sizeof(DynamicTransforms), cbInfo.size);
                ctx.WriteBuffer(cbInfo.buffer.Get(), &dynamicTransformsCB, sizeToWrite);
            }
            else if (cbInfo.isPerObject && cbInfo.isVertexShader) {
                // Per-object VS CB (slot 0) - likely "per_frame" or similar
                u32 sizeToWrite = std::min<u32>(sizeof(ParticlePerFrame), cbInfo.size);
                ctx.WriteBuffer(cbInfo.buffer.Get(), &perFrameCB, sizeToWrite);
            }
        }

        // ═══════════════════════════════════════════════════════
        //  BIND RESOURCES (after pipeline + CB updates)
        // ═══════════════════════════════════════════════════════

        // Bind BOTH binding sets (like GBufferPass does)
        // Slot 0: VS binding set (VS constant buffers)
        // Slot 1: PS binding set (PS constant buffers + textures + samplers)
        ctx.SetBindingSet(0, bindingCache->vsBindingSet.Get());
        ctx.SetBindingSet(1, bindingCache->psBindingSet.Get());

        // Bind vertex buffer
        ctx.SetVertexBuffer(0, m_particleVB.Get(), 0);

        // Bind index buffer
        ctx.SetIndexBuffer(m_quadIB.Get(), nvrhi::Format::R16_UINT, 0);

        // Draw particles (each particle = 2 triangles = 6 indices)
        u32 indexCount = particleCount * 6;
        ctx.DrawIndexed(indexCount, 0, 0);

        return true;
    }

    // ═══════════════════════════════════════════════════════
    //  PARTICLE GROUPS (CParticleGroup)
    // ═══════════════════════════════════════════════════════
    else if (vType == MT_PARTICLE_GROUP) {
        CParticleGroup* pGroup = static_cast<CParticleGroup*>(batch.visual);

        // Particle groups contain multiple child effects
        // We would need to recursively render each child effect
        u32 particleCount = pGroup->ParticlesCount();
        if (particleCount == 0) {
            return false;
        }

        return false;
    }

    return false;
}

} // namespace xray::render::passes
