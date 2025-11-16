// xrRender/ConstantSystem/FGConstantSystem.cpp
#include "stdafx.h"
#include "FGConstantSystem.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"

namespace xray::render::fgconstants {

FGConstantSystem::FGConstantSystem(const MaterialPSO* pso)
    : m_pso(pso)
    , m_dirtyEngine(0)
    , m_dirtyPass(0)
    , m_dirtyMaterial(0)
    , m_dirtyInstance(0)
    , m_dirtyStatic(0)
    , m_staticInitialized(false)
{
    VERIFY(pso != nullptr);

    // Zero volatile staging buffers
    ZeroMemory(m_engineConstants, sizeof(m_engineConstants));
    ZeroMemory(m_passConstants, sizeof(m_passConstants));
    ZeroMemory(m_materialConstants, sizeof(m_materialConstants));
    ZeroMemory(m_instanceConstants, sizeof(m_instanceConstants));

    // Zero static staging buffer
    ZeroMemory(m_staticConstants, sizeof(m_staticConstants));
}

// ═══════════════════════════════════════════════════
//  TYPE-SAFE SETTERS
// ═══════════════════════════════════════════════════

void FGConstantSystem::Set(const char* name, float value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem] Constant '%s' not found in shader", name);
        return;
    }

    if (constant->size != sizeof(float)) {
        Msg("! [FGConstantSystem] Size mismatch for '%s': expected %u, got %u",
            name, constant->size, sizeof(float));
        return;
    }

    WriteConstant(constant, &value, sizeof(float));
}

void FGConstantSystem::Set(const char* name, const Fvector2& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem] Constant '%s' not found in shader", name);
        return;
    }

    // Fvector2 is 8 bytes, but shader expects aligned float2 (16 bytes due to packing)
    // Write as float4 with z,w = 0
    alignas(16) float data[4] = { value.x, value.y, 0.0f, 0.0f };
    WriteConstant(constant, data, 16);
}

void FGConstantSystem::Set(const char* name, const Fvector& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem] Constant '%s' not found in shader", name);
        return;
    }

    // Fvector is 12 bytes, but shader expects aligned float3 (16 bytes)
    alignas(16) float data[4] = { value.x, value.y, value.z, 0.0f };
    WriteConstant(constant, data, 16);
}

void FGConstantSystem::Set(const char* name, const Fvector4& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem] Constant '%s' not found in shader", name);
        return;
    }

    if (constant->size != 16) {
        Msg("! [FGConstantSystem] Size mismatch for '%s': expected %u, got 16",
            name, constant->size);
        return;
    }

    WriteConstant(constant, &value, 16);
}

void FGConstantSystem::Set(const char* name, const Fmatrix& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem] Constant '%s' not found in shader", name);
        return;
    }

    if (constant->size != 64) {
        Msg("! [FGConstantSystem] Size mismatch for '%s': expected %u, got 64",
            name, constant->size);
        return;
    }

    WriteConstant(constant, &value, 64);
}

// ═══════════════════════════════════════════════════
//  STATIC CONSTANT SETTERS
// ═══════════════════════════════════════════════════

void FGConstantSystem::SetStatic(const char* name, float value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem::SetStatic] Constant '%s' not found in shader", name);
        return;
    }

    if (constant->size != sizeof(float)) {
        Msg("! [FGConstantSystem::SetStatic] Size mismatch for '%s': expected %u, got %u",
            name, constant->size, sizeof(float));
        return;
    }

    WriteStaticConstant(constant, &value, sizeof(float));
}

void FGConstantSystem::SetStatic(const char* name, const Fvector2& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem::SetStatic] Constant '%s' not found in shader", name);
        return;
    }

    // Fvector2 is 8 bytes, but shader expects aligned float2 (16 bytes due to packing)
    alignas(16) float data[4] = { value.x, value.y, 0.0f, 0.0f };
    WriteStaticConstant(constant, data, 16);
}

void FGConstantSystem::SetStatic(const char* name, const Fvector& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem::SetStatic] Constant '%s' not found in shader", name);
        return;
    }

    // Fvector is 12 bytes, but shader expects aligned float3 (16 bytes)
    alignas(16) float data[4] = { value.x, value.y, value.z, 0.0f };
    WriteStaticConstant(constant, data, 16);
}

void FGConstantSystem::SetStatic(const char* name, const Fvector4& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem::SetStatic] Constant '%s' not found in shader", name);
        return;
    }

    if (constant->size != 16) {
        Msg("! [FGConstantSystem::SetStatic] Size mismatch for '%s': expected %u, got 16",
            name, constant->size);
        return;
    }

    WriteStaticConstant(constant, &value, 16);
}

void FGConstantSystem::SetStatic(const char* name, const Fmatrix& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem::SetStatic] Constant '%s' not found in shader", name);
        return;
    }

    if (constant->size != 64) {
        Msg("! [FGConstantSystem::SetStatic] Size mismatch for '%s': expected %u, got 64",
            name, constant->size);
        return;
    }

    WriteStaticConstant(constant, &value, 64);
}

// ═══════════════════════════════════════════════════
//  FREQUENCY-BASED COMMIT (VOLATILE)
// ═══════════════════════════════════════════════════

void FGConstantSystem::CommitEngine(ng::RenderContext* ctx) {
    // Find static_globals CB
    const auto* engineCB = m_pso->constantLayout.constantBuffers.GetByName("static_globals");
    if (!engineCB) {
        return;  // No engine-level constants
    }

    if (m_dirtyEngine == 0) {
        return;  // No dirty constants
    }

    // Find NVRHI buffer for this CB
    nvrhi::IBuffer* engineBuffer = nullptr;
    for (const auto& vcbReq : m_pso->vcbRequirements) {
        if (vcbReq.name == "static_globals") {
            engineBuffer = ctx->GetDevice()->GetNativeBuffer(vcbReq.vcbHandle);
            break;
        }
    }

    if (!engineBuffer) {
        Msg("! [FGConstantSystem] No VCB found for static_globals");
        return;
    }

    // Upload to GPU
    ctx->WriteBuffer(engineBuffer, m_engineConstants, engineCB->size);

    // Clear dirty flags
    m_dirtyEngine = 0;
}

void FGConstantSystem::CommitPass(ng::RenderContext* ctx) {
    // Similar to CommitEngine, but for pass-level constants
    // (Currently no dedicated pass CB in your shaders - skip for now)
    m_dirtyPass = 0;
}

void FGConstantSystem::CommitMaterial(ng::RenderContext* ctx) {
    // Find dynamic_transforms CB
    const auto* materialCB = m_pso->constantLayout.constantBuffers.GetByName("dynamic_transforms");
    if (!materialCB) {
        return;
    }

    if (m_dirtyMaterial == 0) {
        return;
    }

    // Find NVRHI buffer
    nvrhi::IBuffer* materialBuffer = nullptr;
    for (const auto& vcbReq : m_pso->vcbRequirements) {
        if (vcbReq.name == "dynamic_transforms") {
            materialBuffer = ctx->GetDevice()->GetNativeBuffer(vcbReq.vcbHandle);
            break;
        }
    }

    if (!materialBuffer) {
        Msg("! [FGConstantSystem] No VCB found for dynamic_transforms");
        return;
    }

    ctx->WriteBuffer(materialBuffer, m_materialConstants, materialCB->size);
    m_dirtyMaterial = 0;
}

void FGConstantSystem::CommitInstance(ng::RenderContext* ctx) {
    // Find $Globals CB
    const auto* instanceCB = m_pso->constantLayout.constantBuffers.GetByName("$Globals");
    if (!instanceCB) {
        return;
    }

    if (m_dirtyInstance == 0) {
        return;
    }

    // Find NVRHI buffer
    nvrhi::IBuffer* instanceBuffer = nullptr;
    for (const auto& vcbReq : m_pso->vcbRequirements) {
        if (vcbReq.name == "$Globals") {
            instanceBuffer = ctx->GetDevice()->GetNativeBuffer(vcbReq.vcbHandle);
            break;
        }
    }

    if (!instanceBuffer) {
        Msg("! [FGConstantSystem] No VCB found for $Globals");
        return;
    }

    ctx->WriteBuffer(instanceBuffer, m_instanceConstants, instanceCB->size);
    m_dirtyInstance = 0;
}

void FGConstantSystem::CommitAll(ng::RenderContext* ctx) {
    CommitEngine(ctx);
    CommitPass(ctx);
    CommitMaterial(ctx);
    CommitInstance(ctx);
}

// ═══════════════════════════════════════════════════
//  STATIC CONSTANT COMMIT
// ═══════════════════════════════════════════════════

void FGConstantSystem::CommitStatic(ng::RenderContext* ctx) {
    // Upload static constants from MaterialPSO->constantBuffers (not vcbRequirements!)
    // Static CBs are persistent GPU buffers, uploaded once and reused across frames

    if (m_dirtyStatic == 0 && m_staticInitialized) {
        return;  // No dirty static constants and already initialized
    }

    // Iterate through all CB info from constantBuffers (static persistent CBs)
    for (const auto& cbInfo : m_pso->constantBuffers) {
        // Check if this CB contains static constants
        bool hasStaticConstants = false;
        for (const auto& constant : m_pso->constantLayout.constants) {
            if (constant.cbIndex < m_pso->constantLayout.constantBuffers.buffers.size()) {
                const auto& cb = m_pso->constantLayout.constantBuffers.buffers[constant.cbIndex];
                if (cb.name == cbInfo.name && constant.persistence == ConstantPersistence::Static) {
                    hasStaticConstants = true;
                    break;
                }
            }
        }

        if (!hasStaticConstants)
            continue;

        // Upload staging data to this static CB
        ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), m_staticConstants, cbInfo.size);
    }

    // Clear dirty flags
    m_dirtyStatic = 0;
    m_staticInitialized = true;
}

void FGConstantSystem::InvalidateStatic(const char* name) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant || constant->persistence != ConstantPersistence::Static) {
        return;
    }

    // Mark static constants as dirty for re-upload
    m_dirtyStatic = ~0ull;
}

void FGConstantSystem::InvalidateAllStatic() {
    m_dirtyStatic = ~0ull;
    m_staticInitialized = false;
}

bool FGConstantSystem::HasStaticConstants() const {
    // Check if any constant in layout has static persistence
    for (const auto& constant : m_pso->constantLayout.constants) {
        if (constant.persistence == ConstantPersistence::Static) {
            return true;
        }
    }
    return false;
}

// ═══════════════════════════════════════════════════
//  INTERNAL HELPERS
// ═══════════════════════════════════════════════════

void FGConstantSystem::WriteConstant(const ShaderConstant* constant, const void* data, u32 size) {
    VERIFY(constant != nullptr);

    u8* stagingBuffer = GetStagingBuffer(constant->frequency);
    memcpy(stagingBuffer + constant->offset, data, size);

    // Mark dirty (use constant index within frequency as bit index)
    // For now, just mark entire frequency dirty (TODO: per-constant bits)
    switch (constant->frequency) {
        case UpdateFrequency::Engine:   m_dirtyEngine = ~0ull; break;
        case UpdateFrequency::Pass:     m_dirtyPass = ~0ull; break;
        case UpdateFrequency::Material: m_dirtyMaterial = ~0ull; break;
        case UpdateFrequency::Instance: m_dirtyInstance = ~0ull; break;
    }
}

u8* FGConstantSystem::GetStagingBuffer(UpdateFrequency freq) {
    switch (freq) {
        case UpdateFrequency::Engine:   return m_engineConstants;
        case UpdateFrequency::Pass:     return m_passConstants;
        case UpdateFrequency::Material: return m_materialConstants;
        case UpdateFrequency::Instance: return m_instanceConstants;
        default:                        return m_instanceConstants;
    }
}

void FGConstantSystem::WriteStaticConstant(const ShaderConstant* constant, const void* data, u32 size) {
    VERIFY(constant != nullptr);
    VERIFY(constant->persistence == ConstantPersistence::Static);

    // Write to static staging buffer (persistent, not per-frame)
    memcpy(m_staticConstants + constant->offset, data, size);

    // Mark static as dirty
    m_dirtyStatic = ~0ull;
}

bool FGConstantSystem::ValidateBindings() const {
    // TODO: Check all required constants are set
    return true;
}

void FGConstantSystem::PrintUnboundConstants() const {
    // TODO: Log which constants are missing
}

} // namespace xray::render::fgconstants
