// xrRender/FrameGraphPasses/ParticlePass.cpp
#include "stdafx.h"
#include "ParticlePass.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/FrameGraph/FrameGraph.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/ParticleEffect.h"
#include "Layers/xrRender/ParticleEffectDef.h"
#include "Layers/xrRender/ParticleGroup.h"
#include "Layers/xrRender/PSLibrary.h"
#include "xrEngine/Render.h"

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

ParticlePass::ParticlePass(ng::RenderDevice* device, const ParticlePassConfig& config)
    : m_device(device)
    , m_config(config)
{
    VERIFY(m_device != nullptr);

    if (config.width == 0 || config.height == 0) {
        Msg("! [ParticlePass] ERROR: Invalid resolution %ux%u", config.width, config.height);
    }
}

ParticlePass::~ParticlePass() {
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
    Msg("  [ParticlePass] Created quad IB for %u quads (%u indices)", maxQuads, numIndices);
}

void ParticlePass::EnsureParticleVertexBuffer(u32 sizeBytes) {
    if (m_particleVB && m_particleVBSize >= sizeBytes) {
        return;  // Already large enough
    }

    // Round up to 64KB chunks for efficiency
    u32 allocSize = ((sizeBytes + 65535) / 65536) * 65536;

    // Create dynamic vertex buffer
    nvrhi::BufferDesc vbDesc;
    vbDesc.byteSize = allocSize;
    vbDesc.isVertexBuffer = true;
    vbDesc.debugName = "ParticleDynamicVB";
    vbDesc.initialState = nvrhi::ResourceStates::VertexBuffer;
    vbDesc.cpuAccess = nvrhi::CpuAccessMode::Write;  // CPU writable for dynamic updates

    m_particleVB = m_device->GetNVRHIDevice()->createBuffer(vbDesc);
    if (!m_particleVB) {
        Msg("! [ParticlePass] ERROR: Failed to create particle vertex buffer");
        return;
    }

    m_particleVBSize = allocSize;
    Msg("  [ParticlePass] Created particle VB: %u bytes", allocSize);
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

    Msg("! [ParticlePass] Rendering %u world + %u HUD particles",
        totalWorldParticles, totalHUDParticles);

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
        Msg("! [ParticlePass] TODO: Render %u world particles", (u32)m_worldParticleBatches->size());

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
        Msg("! [ParticlePass] TODO: Render %u HUD particles with FOV adjustment", (u32)m_hudParticleBatches->size());

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

    Msg("! [ParticlePass] Complete: %u draws, %u batches, %.2f ms",
        m_stats.numDrawCalls, m_stats.numBatches, m_stats.cpuTimeMs);
}

ng::PipelineState* ParticlePass::GetOrCreateParticlePSO(
    RENDER_NAMESPACE::Shader* shader,
    const RENDER_NAMESPACE::PS::CPEDef* pDef,
    const framegraph::FrameGraph& fg)
{
    if (!shader || !pDef)
        return nullptr;

    // Get first pass from shader
    if (shader->passes.empty())
        return nullptr;

    RENDER_NAMESPACE::SPass* pass = shader->passes.front()._get();
    if (!pass || !pass->vs || !pass->ps)
        return nullptr;

    // Check cache first
    shared_str shaderKey = pass->vs->cName;
    auto it = m_particlePSOCache.find(shaderKey);
    if (it != m_particlePSOCache.end()) {
        return it->second;
    }

    // ═══════════════════════════════════════════════════════
    //  CREATE PARTICLE PSO
    // ═══════════════════════════════════════════════════════

    Msg("  [ParticlePass] Creating PSO for particle shader '%s'", shaderKey.c_str());

    ng::PipelineStateDesc psoDesc;
    psoDesc.debugName = shaderKey.c_str();

    // ─── Vertex Shader ───
    psoDesc.vs = framegraph::LoadShader(
        m_device->GetNVRHIDevice(),
        pass->vs->vs,
        nvrhi::ShaderType::Vertex);

    if (!psoDesc.vs) {
        Msg("! [ParticlePass] ERROR: Failed to load VS '%s'", pass->vs->cName.c_str());
        return nullptr;
    }

    // ─── Pixel Shader ───
    psoDesc.ps = framegraph::LoadShader(
        m_device->GetNVRHIDevice(),
        pass->ps->ps,
        nvrhi::ShaderType::Pixel);

    if (!psoDesc.ps) {
        Msg("! [ParticlePass] ERROR: Failed to load PS '%s'", pass->ps->cName.c_str());
        return nullptr;
    }

    // ─── Input Layout (FVF::LIT format) ───
    // ParticleVertex: position (float3), color (uint32), texcoord (float2)
    psoDesc.vertexAttributes.clear();

    // Position (POSITION semantic)
    ng::VertexAttributeDesc posAttr;
    posAttr.name = "POSITION";
    posAttr.format = nvrhi::Format::RGB32_FLOAT;
    posAttr.offset = 0;
    posAttr.bufferIndex = 0;
    posAttr.elementStride = sizeof(ParticleVertex);
    psoDesc.vertexAttributes.push_back(posAttr);

    // Color (COLOR semantic)
    ng::VertexAttributeDesc colorAttr;
    colorAttr.name = "COLOR";
    colorAttr.format = nvrhi::Format::RGBA8_UNORM;  // u32 color -> RGBA8
    colorAttr.offset = 12;  // After position (3 floats)
    colorAttr.bufferIndex = 0;
    colorAttr.elementStride = sizeof(ParticleVertex);
    psoDesc.vertexAttributes.push_back(colorAttr);

    // Texcoord (TEXCOORD semantic)
    ng::VertexAttributeDesc texAttr;
    texAttr.name = "TEXCOORD";
    texAttr.format = nvrhi::Format::RG32_FLOAT;
    texAttr.offset = 16;  // After position + color
    texAttr.bufferIndex = 0;
    texAttr.elementStride = sizeof(ParticleVertex);
    psoDesc.vertexAttributes.push_back(texAttr);

    // ─── Render Targets ───
    nvrhi::ITexture* albedo = fg.GetPhysicalTexture(m_outputs.albedo);
    nvrhi::ITexture* normal = fg.GetPhysicalTexture(m_outputs.normal);
    nvrhi::ITexture* material = fg.GetPhysicalTexture(m_outputs.material);
    nvrhi::ITexture* depth = fg.GetPhysicalTexture(m_outputs.depth);

    psoDesc.renderTargets.clear();
    psoDesc.renderTargets.push_back(albedo);
    psoDesc.renderTargets.push_back(normal);
    psoDesc.renderTargets.push_back(material);
    psoDesc.depthStencil = depth;

    // ─── Blend State (Alpha Blending) ───
    psoDesc.blendDesc.alphaToCoverageEnable = false;
    psoDesc.blendDesc.independentBlendEnable = false;

    // RT0: Albedo - enable alpha blending
    auto& rt0 = psoDesc.blendDesc.targets[0];
    rt0.blendEnable = true;
    rt0.srcBlend = nvrhi::BlendFactor::SrcAlpha;
    rt0.destBlend = nvrhi::BlendFactor::InvSrcAlpha;
    rt0.blendOp = nvrhi::BlendOp::Add;
    rt0.srcBlendAlpha = nvrhi::BlendFactor::One;
    rt0.destBlendAlpha = nvrhi::BlendFactor::InvSrcAlpha;
    rt0.blendOpAlpha = nvrhi::BlendOp::Add;
    rt0.colorWriteMask = nvrhi::ColorMask::All;

    // RT1, RT2: Disable for particles (they don't write normals/material)
    psoDesc.blendDesc.targets[1].colorWriteMask = nvrhi::ColorMask::None;
    psoDesc.blendDesc.targets[2].colorWriteMask = nvrhi::ColorMask::None;

    // ─── Depth/Stencil State (Test but no write) ───
    psoDesc.depthStencilDesc.depthTestEnable = true;
    psoDesc.depthStencilDesc.depthWriteEnable = false;  // Transparent particles don't write depth
    psoDesc.depthStencilDesc.depthFunc = nvrhi::ComparisonFunc::LessOrEqual;
    psoDesc.depthStencilDesc.stencilEnable = false;

    // ─── Rasterizer State ───
    psoDesc.rasterizerDesc.fillMode = nvrhi::RasterFillMode::Solid;
    psoDesc.rasterizerDesc.cullMode = nvrhi::RasterCullMode::None;  // Particles are double-sided by default

    // Check particle definition for culling
    if (pDef->m_Flags.is(RENDER_NAMESPACE::PS::CPEDef::dfCulling)) {
        if (pDef->m_Flags.is(RENDER_NAMESPACE::PS::CPEDef::dfCullCCW)) {
            psoDesc.rasterizerDesc.cullMode = nvrhi::RasterCullMode::Front;
        } else {
            psoDesc.rasterizerDesc.cullMode = nvrhi::RasterCullMode::Back;
        }
    }

    psoDesc.rasterizerDesc.frontCounterClockwise = false;
    psoDesc.rasterizerDesc.depthClipEnable = true;
    psoDesc.rasterizerDesc.scissorEnable = false;
    psoDesc.rasterizerDesc.multisampleEnable = false;
    psoDesc.rasterizerDesc.antialiasedLineEnable = false;

    // ─── Topology ───
    psoDesc.primType = nvrhi::PrimitiveType::TriangleList;

    // ─── Create PSO ───
    ng::PipelineState* pso = m_device->CreatePipelineState(psoDesc);
    if (!pso) {
        Msg("! [ParticlePass] ERROR: Failed to create PSO for '%s'", shaderKey.c_str());
        return nullptr;
    }

    // Cache and return
    m_particlePSOCache[shaderKey] = pso;

    Msg("  ✓ Created particle PSO for shader '%s'", shaderKey.c_str());

    return pso;
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

        // Particles store their shader in the geometry or effect definition
        RENDER_NAMESPACE::Shader* particleShader = nullptr;

        if (pEffect->geom && pEffect->geom->ps) {
            // Get shader from geometry (preferred - set during OnDeviceCreate)
            particleShader = pEffect->geom->ps;
        } else if (pDef->m_CachedShader) {
            // Fallback: get cached shader from definition
            particleShader = pDef->m_CachedShader;
        }

        if (!particleShader) {
            Msg("! [ParticlePass] WARNING: No shader for particle effect '%s'", pDef->m_Name.c_str());
            return false;
        }

        // ═══════════════════════════════════════════════════════
        //  GET OR CREATE PSO
        // ═══════════════════════════════════════════════════════

        ng::PipelineState* particlePSO = GetOrCreateParticlePSO(particleShader, pDef, fg);

        if (!particlePSO) {
            // PSO system not implemented yet - log and skip
            Msg("! [ParticlePass] Generated %u billboard quads for effect '%s' (shader=%s, HUD=%d) - NO PSO",
                particleCount, pDef->m_Name.c_str(),
                particleShader->passes.front().vs->cName.c_str(), batch.isHUDMode);
            return false;
        }

        // ═══════════════════════════════════════════════════════
        //  RENDER PARTICLES
        // ═══════════════════════════════════════════════════════

        // Bind pipeline
        ctx.SetPipeline(particlePSO->GetNativePipeline());

        // Bind vertex buffer
        ctx.SetVertexBuffer(0, m_particleVB.Get(), 0);

        // Bind index buffer
        ctx.SetIndexBuffer(m_quadIB.Get(), nvrhi::Format::R16_UINT, 0);

        // Draw particles (each particle = 2 triangles = 6 indices)
        u32 indexCount = particleCount * 6;
        ctx.DrawIndexed(indexCount, 0, 0);

        Msg("! [ParticlePass] Rendered %u billboard quads for effect '%s' (shader=%s, HUD=%d)",
            particleCount, pDef->m_Name.c_str(),
            particleShader->passes.front().vs->cName.c_str(), batch.isHUDMode);

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

        Msg("! [ParticlePass] TODO: Render particle group with %u particles (HUD=%d)",
            particleCount, batch.isHUDMode);

        return false;
    }

    return false;
}

} // namespace xray::render::passes
