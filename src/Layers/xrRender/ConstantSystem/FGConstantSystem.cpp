// xrRender/ConstantSystem/FGConstantSystem.cpp
#include "stdafx.h"
#include "FGConstantSystem.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/FrameGraph/VolatileConstantBufferPool.h"

namespace xray::render::fgconstants {

FGConstantSystem::FGConstantSystem(
    const MaterialPSO* pso,
    framegraph::VolatileConstantBufferPool* vcbPool)
    : m_pso(pso)
    , m_vcbPool(vcbPool)
    , m_dirtyStatic(0)
    , m_staticInitialized(false)
{
    VERIFY(pso != nullptr);

    // Per-CB staging buffers initialized on-demand in GetStagingBuffer()
    // Zero static staging buffer
    ZeroMemory(m_staticConstants, sizeof(m_staticConstants));
}

// ═══════════════════════════════════════════════════
//  TYPE-SAFE SETTERS
// ═══════════════════════════════════════════════════

void FGConstantSystem::Set(const char* name, float value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        return;
    }

    if (constant->size != sizeof(float)) {
        return;
    }

    WriteConstant(constant, &value, sizeof(float));
}

void FGConstantSystem::Set(const char* name, const Fvector2& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
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
        return;
    }

    // Fvector is 12 bytes, but shader expects aligned float3 (16 bytes)
    alignas(16) float data[4] = { value.x, value.y, value.z, 0.0f };
    WriteConstant(constant, data, 16);
}

void FGConstantSystem::Set(const char* name, const Fvector4& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        return;
    }

    if (constant->size != 16) {
        return;
    }

    WriteConstant(constant, &value, 16);
}

void FGConstantSystem::Set(const char* name, const Fmatrix& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        return;
    }

    if (!constant->IsMatrix()) {
        return;
    }

    // REFLECTION-DRIVEN ROUTING
    u16 cbIndex = constant->cbIndex;
    const auto& cbInfo = m_pso->constantLayout.constantBuffers.buffers[cbIndex];

    u8* stagingBuffer = GetStagingBuffer(constant->frequency, cbIndex);

    // Automatic format detection based on shader reflection
    if (constant->IsMatrix3x4()) {
        WriteMatrix3x4(stagingBuffer + constant->offset, value);
    } else if (constant->IsMatrix4x4()) {
        WriteMatrix4x4(stagingBuffer + constant->offset, value);
    } else {
        return;
    }

    // Mark dirty
    SetDirtyBit(constant->frequency, cbIndex);
}

// ═══════════════════════════════════════════════════
//  STATIC CONSTANT SETTERS
// ═══════════════════════════════════════════════════

void FGConstantSystem::SetStatic(const char* name, float value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        return;
    }

    if (constant->size != sizeof(float)) {
        return;
    }

    WriteStaticConstant(constant, &value, sizeof(float));
}

void FGConstantSystem::SetStatic(const char* name, const Fvector2& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        return;
    }

    // Fvector2 is 8 bytes, but shader expects aligned float2 (16 bytes due to packing)
    alignas(16) float data[4] = { value.x, value.y, 0.0f, 0.0f };
    WriteStaticConstant(constant, data, 16);
}

void FGConstantSystem::SetStatic(const char* name, const Fvector& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        return;
    }

    // Fvector is 12 bytes, but shader expects aligned float3 (16 bytes)
    alignas(16) float data[4] = { value.x, value.y, value.z, 0.0f };
    WriteStaticConstant(constant, data, 16);
}

void FGConstantSystem::SetStatic(const char* name, const Fvector4& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        return;
    }

    if (constant->size != 16) {
        return;
    }

    WriteStaticConstant(constant, &value, 16);
}

void FGConstantSystem::SetStatic(const char* name, const Fmatrix& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        return;
    }

    if (!constant->IsMatrix()) {
        return;
    }

    // Write to static staging buffer
    if (constant->IsMatrix3x4()) {
        WriteMatrix3x4(m_staticConstants + constant->offset, value);
    } else if (constant->IsMatrix4x4()) {
        WriteMatrix4x4(m_staticConstants + constant->offset, value);
    }

    m_dirtyStatic = ~0ull;
}

// ═══════════════════════════════════════════════════
//  ARRAY SETTERS (FOR BONE TRANSFORMS)
// ═══════════════════════════════════════════════════

void FGConstantSystem::SetArray(const char* name, const Fmatrix* values, u32 count) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        return;
    }

    if (!constant->IsArray()) {
        return;
    }

    if (count > constant->arrayCount) {
        count = constant->arrayCount;  // Clamp to shader size
    }

    // REFLECTION-DRIVEN ROUTING: Find which CB contains this array
    u16 cbIndex = constant->cbIndex;
    u8* stagingBuffer = GetStagingBuffer(constant->frequency, cbIndex);
    u8* dest = stagingBuffer + constant->offset;

    // Detect element type from constant metadata
    // For bone transforms, shader declares: row_major float3x4 m_xform[64]

    if (constant->elementSize == 48) {
        // float3x4 array (bone transforms)
        for (u32 i = 0; i < count; ++i) {
            WriteMatrix3x4(dest, values[i]);
            dest += 48;
        }
    } else if (constant->elementSize == 64) {
        // float4x4 array
        for (u32 i = 0; i < count; ++i) {
            WriteMatrix4x4(dest, values[i]);
            dest += 64;
        }
    } else {
        return;
    }

    // Mark dirty
    SetDirtyBit(constant->frequency, cbIndex);
}

void FGConstantSystem::SetStaticArray(const char* name, const Fmatrix* values, u32 count) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        return;
    }

    if (!constant->IsArray()) {
        return;
    }

    if (count > constant->arrayCount) {
        count = constant->arrayCount;  // Clamp to shader size
    }

    u8* dest = m_staticConstants + constant->offset;

    // Detect element type from constant metadata
    if (constant->elementSize == 48) {
        // float3x4 array (bone transforms)
        for (u32 i = 0; i < count; ++i) {
            WriteMatrix3x4(dest, values[i]);
            dest += 48;
        }
    } else if (constant->elementSize == 64) {
        // float4x4 array
        for (u32 i = 0; i < count; ++i) {
            WriteMatrix4x4(dest, values[i]);
            dest += 64;
        }
    } else {
        return;
    }

    m_dirtyStatic = ~0ull;
}

// ═══════════════════════════════════════════════════
//  FREQUENCY-BASED COMMIT (VOLATILE)
// ═══════════════════════════════════════════════════

void FGConstantSystem::CommitEngine(ng::RenderContext* ctx) {
    // Engine-frequency CBs (uploaded once per frame)
    // Uses same reflection-driven approach as CommitInstance
    if (!m_vcbPool) return;

    for (auto& [cbIndex, staging] : m_engineStaging) {
        if (!staging.dirty) continue;

        const auto& cbInfo = m_pso->constantLayout.constantBuffers.buffers[cbIndex];
        ng::BufferHandle vcbHandle = m_vcbPool->GetOrCreateVCB(
            framegraph::VolatileConstantBufferPool::CBLayout(
                cbInfo.name.c_str(), cbInfo.slot, cbInfo.size
            )
        );

        if (!vcbHandle.IsValid()) continue;

        nvrhi::IBuffer* nvrhiBuffer = ctx->GetDevice()->GetNativeBuffer(vcbHandle);
        if (nvrhiBuffer) {
            ctx->WriteBuffer(nvrhiBuffer, staging.data, staging.size);
            staging.dirty = false;
        }
    }
}

void FGConstantSystem::CommitPass(ng::RenderContext* ctx) {
    // Pass-frequency CBs (uploaded once per render pass)
    if (!m_vcbPool) return;

    for (auto& [cbIndex, staging] : m_passStaging) {
        if (!staging.dirty) continue;

        const auto& cbInfo = m_pso->constantLayout.constantBuffers.buffers[cbIndex];
        ng::BufferHandle vcbHandle = m_vcbPool->GetOrCreateVCB(
            framegraph::VolatileConstantBufferPool::CBLayout(
                cbInfo.name.c_str(), cbInfo.slot, cbInfo.size
            )
        );

        if (!vcbHandle.IsValid()) continue;

        nvrhi::IBuffer* nvrhiBuffer = ctx->GetDevice()->GetNativeBuffer(vcbHandle);
        if (nvrhiBuffer) {
            ctx->WriteBuffer(nvrhiBuffer, staging.data, staging.size);
            staging.dirty = false;
        }
    }
}

void FGConstantSystem::CommitMaterial(ng::RenderContext* ctx) {
    // Material-frequency CBs (uploaded once per material)
    if (!m_vcbPool) return;

    for (auto& [cbIndex, staging] : m_materialStaging) {
        if (!staging.dirty) continue;

        const auto& cbInfo = m_pso->constantLayout.constantBuffers.buffers[cbIndex];
        ng::BufferHandle vcbHandle = m_vcbPool->GetOrCreateVCB(
            framegraph::VolatileConstantBufferPool::CBLayout(
                cbInfo.name.c_str(), cbInfo.slot, cbInfo.size
            )
        );

        if (!vcbHandle.IsValid()) continue;

        nvrhi::IBuffer* nvrhiBuffer = ctx->GetDevice()->GetNativeBuffer(vcbHandle);
        if (nvrhiBuffer) {
            ctx->WriteBuffer(nvrhiBuffer, staging.data, staging.size);
            staging.dirty = false;
        }
    }
}

void FGConstantSystem::CommitInstance(ng::RenderContext* ctx) {
    // ═══════════════════════════════════════════════════
    //  REFLECTION-DRIVEN: Upload ONLY dirty CBs
    // ═══════════════════════════════════════════════════
    // No hardcoded CB names! Iterate through all Instance-frequency CBs
    // and upload only the ones marked dirty.

    if (!m_vcbPool) {
        return;
    }

    // Upload each dirty Instance-frequency CB
    for (auto& [cbIndex, staging] : m_instanceStaging) {
        if (!staging.dirty) {
            continue;  // Skip non-dirty CBs
        }

        // Get CB metadata from reflection
        if (cbIndex >= m_pso->constantLayout.constantBuffers.buffers.size()) {
            continue;
        }

        const auto& cbInfo = m_pso->constantLayout.constantBuffers.buffers[cbIndex];

        // Get VCB from pool (based on CB name + slot from reflection)
        ng::BufferHandle vcbHandle = m_vcbPool->GetOrCreateVCB(
            framegraph::VolatileConstantBufferPool::CBLayout(
                cbInfo.name.c_str(),
                cbInfo.slot,
                cbInfo.size
            )
        );

        if (!vcbHandle.IsValid()) {
            continue;
        }

        // Get NVRHI buffer from handle
        nvrhi::IBuffer* nvrhiBuffer = ctx->GetDevice()->GetNativeBuffer(vcbHandle);
        if (!nvrhiBuffer) {
            continue;
        }

        // Upload staging data to GPU
        ctx->WriteBuffer(nvrhiBuffer, staging.data, staging.size);

        // Clear dirty flag
        staging.dirty = false;
    }
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

    // REFLECTION-DRIVEN ROUTING: Find which CB contains this constant
    u16 cbIndex = constant->cbIndex;

    // Get staging buffer for this specific CB (not entire frequency tier)
    u8* stagingBuffer = GetStagingBuffer(constant->frequency, cbIndex);

    // Write constant data to CB staging buffer at correct offset
    memcpy(stagingBuffer + constant->offset, data, size);

    // Mark only this CB as dirty (not entire frequency tier)
    SetDirtyBit(constant->frequency, cbIndex);
}

// ═══════════════════════════════════════════════════
//  REFLECTION-DRIVEN PER-CB STAGING BUFFER
// ═══════════════════════════════════════════════════

u8* FGConstantSystem::GetStagingBuffer(UpdateFrequency freq, u16 cbIndex) {
    // Get appropriate staging map based on frequency
    xr_map<u16, StagingBuffer>* stagingMap = nullptr;

    switch (freq) {
        case UpdateFrequency::Engine:   stagingMap = &m_engineStaging; break;
        case UpdateFrequency::Pass:     stagingMap = &m_passStaging; break;
        case UpdateFrequency::Material: stagingMap = &m_materialStaging; break;
        case UpdateFrequency::Instance: stagingMap = &m_instanceStaging; break;
        default:                        stagingMap = &m_instanceStaging; break;
    }

    // Get or create staging buffer for this CB
    auto& staging = (*stagingMap)[cbIndex];

    // Initialize on first access
    if (staging.size == 0) {
        const auto& cb = m_pso->constantLayout.constantBuffers.buffers[cbIndex];
        staging.size = cb.size;
        ZeroMemory(staging.data, sizeof(staging.data));
    }

    return staging.data;
}

void FGConstantSystem::SetDirtyBit(UpdateFrequency freq, u16 cbIndex) {
    // Get appropriate staging map based on frequency
    xr_map<u16, StagingBuffer>* stagingMap = nullptr;

    switch (freq) {
        case UpdateFrequency::Engine:   stagingMap = &m_engineStaging; break;
        case UpdateFrequency::Pass:     stagingMap = &m_passStaging; break;
        case UpdateFrequency::Material: stagingMap = &m_materialStaging; break;
        case UpdateFrequency::Instance: stagingMap = &m_instanceStaging; break;
        default:                        return;
    }

    // Mark this CB as dirty (needs upload)
    auto& staging = (*stagingMap)[cbIndex];
    staging.dirty = true;
}

void FGConstantSystem::WriteStaticConstant(const ShaderConstant* constant, const void* data, u32 size) {
    VERIFY(constant != nullptr);
    VERIFY(constant->persistence == ConstantPersistence::Static);

    // Write to static staging buffer (persistent, not per-frame)
    memcpy(m_staticConstants + constant->offset, data, size);

    // Mark static as dirty
    m_dirtyStatic = ~0ull;
}

// ═══════════════════════════════════════════════════
//  MATRIX CONVERSION HELPERS
// ═══════════════════════════════════════════════════

void FGConstantSystem::WriteMatrix3x4(u8* dest, const Fmatrix& src) {
    // Fmatrix is row-major, Slang uses column_major interpretation.
    // column_major naturally transposes row-major bytes — no explicit transpose needed.
    // Write first 3 rows (48 bytes) of raw Fmatrix data.
    memcpy(dest, &src, 48);
}

void FGConstantSystem::WriteMatrix4x4(u8* dest, const Fmatrix& src) {
    // Fmatrix is row-major, Slang uses column_major interpretation.
    // column_major naturally transposes row-major bytes — no explicit transpose needed.
    memcpy(dest, &src, sizeof(Fmatrix));
}

bool FGConstantSystem::ValidateBindings() const {
    // TODO: Check all required constants are set
    return true;
}

void FGConstantSystem::PrintUnboundConstants() const {
    // TODO: Log which constants are missing
}

} // namespace xray::render::fgconstants
