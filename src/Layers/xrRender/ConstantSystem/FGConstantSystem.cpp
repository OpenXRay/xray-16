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

    ZeroMemory(m_staticConstants, sizeof(m_staticConstants));
}

void FGConstantSystem::Set(const char* name, float value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem::Set] Constant '%s' not found in shader '%s'", name, m_pso->debugName.c_str());
        return;
    }

    if (constant->size != sizeof(float)) {
        Msg("! [FGConstantSystem] Size mismatch for '%s': expected %u, got %u",
            name, constant->size, sizeof(float));
        return;
    }

    Msg("~ [FGConstantSystem::Set] Setting '%s' (cb=%s, offset=%u)", name,
        m_pso->constantLayout.constantBuffers.buffers[constant->cbIndex].name.c_str(), constant->offset);
    WriteConstant(constant, &value, sizeof(float));
}

void FGConstantSystem::Set(const char* name, const Fvector2& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem] Constant '%s' not found in shader", name);
        return;
    }

    alignas(16) float data[4] = { value.x, value.y, 0.0f, 0.0f };
    WriteConstant(constant, data, 16);
}

void FGConstantSystem::Set(const char* name, const Fvector& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem] Constant '%s' not found in shader", name);
        return;
    }

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
        Msg("! [FGConstantSystem::Set] Matrix constant '%s' not found in shader '%s'", name, m_pso->debugName.c_str());
        return;
    }

    if (!constant->IsMatrix()) {
        Msg("! [FGConstantSystem] Constant '%s' is not a matrix (type=%d)",
            name, (int)constant->type);
        return;
    }

    u16 cbIndex = constant->cbIndex;
    const auto& cbInfo = m_pso->constantLayout.constantBuffers.buffers[cbIndex];
    Msg("~ [FGConstantSystem::Set] Setting matrix '%s' (cb=%s[%u], offset=%u, type=%s)",
        name, cbInfo.name.c_str(), cbIndex, constant->offset,
        constant->IsMatrix3x4() ? "float3x4" : "float4x4");

    u8* stagingBuffer = GetStagingBuffer(constant->frequency, cbIndex);

    if (constant->IsMatrix3x4()) {
        WriteMatrix3x4(stagingBuffer + constant->offset, value);
    } else if (constant->IsMatrix4x4()) {
        WriteMatrix4x4(stagingBuffer + constant->offset, value);
    } else {
        Msg("! [FGConstantSystem] Unknown matrix type for '%s'", name);
        return;
    }

    SetDirtyBit(constant->frequency, cbIndex);
}

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

    alignas(16) float data[4] = { value.x, value.y, 0.0f, 0.0f };
    WriteStaticConstant(constant, data, 16);
}

void FGConstantSystem::SetStatic(const char* name, const Fvector& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem::SetStatic] Constant '%s' not found in shader", name);
        return;
    }

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

    if (!constant->IsMatrix()) {
        Msg("! [FGConstantSystem::SetStatic] Constant '%s' is not a matrix", name);
        return;
    }

    if (constant->IsMatrix3x4()) {
        WriteMatrix3x4(m_staticConstants + constant->offset, value);
    } else if (constant->IsMatrix4x4()) {
        WriteMatrix4x4(m_staticConstants + constant->offset, value);
    }

    m_dirtyStatic = ~0ull;
}

void FGConstantSystem::SetArray(const char* name, const Fmatrix* values, u32 count) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem] Array constant '%s' not found", name);
        return;
    }

    if (!constant->IsArray()) {
        Msg("! [FGConstantSystem] Constant '%s' is not an array", name);
        return;
    }

    if (count > constant->arrayCount) {
        Msg("! [FGConstantSystem] Array size mismatch for '%s': writing %u, shader expects %u",
            name, count, constant->arrayCount);
        count = constant->arrayCount;
    }

    u16 cbIndex = constant->cbIndex;
    u8* stagingBuffer = GetStagingBuffer(constant->frequency, cbIndex);
    u8* dest = stagingBuffer + constant->offset;

    if (constant->elementSize == 48) {
        for (u32 i = 0; i < count; ++i) {
            WriteMatrix3x4(dest, values[i]);
            dest += 48;
        }
    } else if (constant->elementSize == 64) {
        for (u32 i = 0; i < count; ++i) {
            WriteMatrix4x4(dest, values[i]);
            dest += 64;
        }
    } else {
        Msg("! [FGConstantSystem] Unknown matrix element size %u for array '%s'",
            constant->elementSize, name);
        return;
    }

    SetDirtyBit(constant->frequency, cbIndex);
}

void FGConstantSystem::SetStaticArray(const char* name, const Fmatrix* values, u32 count) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        Msg("! [FGConstantSystem::SetStaticArray] Array constant '%s' not found", name);
        return;
    }

    if (!constant->IsArray()) {
        Msg("! [FGConstantSystem::SetStaticArray] Constant '%s' is not an array", name);
        return;
    }

    if (count > constant->arrayCount) {
        Msg("! [FGConstantSystem::SetStaticArray] Array size mismatch for '%s': writing %u, shader expects %u",
            name, count, constant->arrayCount);
        count = constant->arrayCount;
    }

    u8* dest = m_staticConstants + constant->offset;

    if (constant->elementSize == 48) {
        for (u32 i = 0; i < count; ++i) {
            WriteMatrix3x4(dest, values[i]);
            dest += 48;
        }
    } else if (constant->elementSize == 64) {
        for (u32 i = 0; i < count; ++i) {
            WriteMatrix4x4(dest, values[i]);
            dest += 64;
        }
    } else {
        Msg("! [FGConstantSystem::SetStaticArray] Unknown matrix element size %u for array '%s'",
            constant->elementSize, name);
        return;
    }

    m_dirtyStatic = ~0ull;
}

void FGConstantSystem::CommitEngine(ng::RenderContext* ctx) {
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

    if (!m_vcbPool) {
        Msg("! [FGConstantSystem] No VCB pool provided - cannot commit instance constants");
        return;
    }

    for (auto& [cbIndex, staging] : m_instanceStaging) {
        if (!staging.dirty) {
            continue;
        }

        if (cbIndex >= m_pso->constantLayout.constantBuffers.buffers.size()) {
            Msg("! [FGConstantSystem] Invalid cbIndex %u in instance staging", cbIndex);
            continue;
        }

        const auto& cbInfo = m_pso->constantLayout.constantBuffers.buffers[cbIndex];

        ng::BufferHandle vcbHandle = m_vcbPool->GetOrCreateVCB(
            framegraph::VolatileConstantBufferPool::CBLayout(
                cbInfo.name.c_str(),
                cbInfo.slot,
                cbInfo.size
            )
        );

        if (!vcbHandle.IsValid()) {
            Msg("! [FGConstantSystem] Failed to get VCB for CB '%s' (slot=b%u, size=%u)",
                cbInfo.name.c_str(), cbInfo.slot, cbInfo.size);
            continue;
        }

        nvrhi::IBuffer* nvrhiBuffer = ctx->GetDevice()->GetNativeBuffer(vcbHandle);
        if (!nvrhiBuffer) {
            Msg("! [FGConstantSystem] Failed to get NVRHI buffer for CB '%s'", cbInfo.name.c_str());
            continue;
        }

        ctx->WriteBuffer(nvrhiBuffer, staging.data, staging.size);

        staging.dirty = false;

        Msg("~ [FGConstantSystem] Uploaded Instance CB '%s' (slot=b%u, size=%u)",
            cbInfo.name.c_str(), cbInfo.slot, staging.size);
    }
}

void FGConstantSystem::CommitAll(ng::RenderContext* ctx) {
    CommitEngine(ctx);
    CommitPass(ctx);
    CommitMaterial(ctx);
    CommitInstance(ctx);
}

void FGConstantSystem::CommitStatic(ng::RenderContext* ctx) {

    if (m_dirtyStatic == 0 && m_staticInitialized) {
        return;
    }

    for (const auto& cbInfo : m_pso->constantBuffers) {
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

        ctx->WriteBuffer(cbInfo.nvrhiBuffer.Get(), m_staticConstants, cbInfo.size);
    }

    m_dirtyStatic = 0;
    m_staticInitialized = true;
}

void FGConstantSystem::InvalidateStatic(const char* name) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant || constant->persistence != ConstantPersistence::Static) {
        return;
    }

    m_dirtyStatic = ~0ull;
}

void FGConstantSystem::InvalidateAllStatic() {
    m_dirtyStatic = ~0ull;
    m_staticInitialized = false;
}

bool FGConstantSystem::HasStaticConstants() const {
    for (const auto& constant : m_pso->constantLayout.constants) {
        if (constant.persistence == ConstantPersistence::Static) {
            return true;
        }
    }
    return false;
}

void FGConstantSystem::WriteConstant(const ShaderConstant* constant, const void* data, u32 size) {
    VERIFY(constant != nullptr);

    u16 cbIndex = constant->cbIndex;

    u8* stagingBuffer = GetStagingBuffer(constant->frequency, cbIndex);

    memcpy(stagingBuffer + constant->offset, data, size);

    SetDirtyBit(constant->frequency, cbIndex);
}

u8* FGConstantSystem::GetStagingBuffer(UpdateFrequency freq, u16 cbIndex) {
    xr_map<u16, StagingBuffer>* stagingMap = nullptr;

    switch (freq) {
        case UpdateFrequency::Engine:   stagingMap = &m_engineStaging; break;
        case UpdateFrequency::Pass:     stagingMap = &m_passStaging; break;
        case UpdateFrequency::Material: stagingMap = &m_materialStaging; break;
        case UpdateFrequency::Instance: stagingMap = &m_instanceStaging; break;
        default:                        stagingMap = &m_instanceStaging; break;
    }

    auto& staging = (*stagingMap)[cbIndex];

    if (staging.size == 0) {
        const auto& cb = m_pso->constantLayout.constantBuffers.buffers[cbIndex];
        staging.size = cb.size;
        ZeroMemory(staging.data, sizeof(staging.data));
    }

    return staging.data;
}

void FGConstantSystem::SetDirtyBit(UpdateFrequency freq, u16 cbIndex) {
    xr_map<u16, StagingBuffer>* stagingMap = nullptr;

    switch (freq) {
        case UpdateFrequency::Engine:   stagingMap = &m_engineStaging; break;
        case UpdateFrequency::Pass:     stagingMap = &m_passStaging; break;
        case UpdateFrequency::Material: stagingMap = &m_materialStaging; break;
        case UpdateFrequency::Instance: stagingMap = &m_instanceStaging; break;
        default:                        return;
    }

    auto& staging = (*stagingMap)[cbIndex];
    staging.dirty = true;
}

void FGConstantSystem::WriteStaticConstant(const ShaderConstant* constant, const void* data, u32 size) {
    VERIFY(constant != nullptr);
    VERIFY(constant->persistence == ConstantPersistence::Static);

    memcpy(m_staticConstants + constant->offset, data, size);

    m_dirtyStatic = ~0ull;
}

void FGConstantSystem::WriteMatrix3x4(u8* dest, const Fmatrix& src) {

    float* out = (float*)dest;

    out[0]  = src._11;  out[1]  = src._12;  out[2]  = src._13;  out[3]  = src._14;

    out[4]  = src._21;  out[5]  = src._22;  out[6]  = src._23;  out[7]  = src._24;

    out[8]  = src._31;  out[9]  = src._32;  out[10] = src._33;  out[11] = src._34;

}

void FGConstantSystem::WriteMatrix4x4(u8* dest, const Fmatrix& src) {

    float* out = (float*)dest;

    out[0]  = src._11;  out[1]  = src._12;  out[2]  = src._13;  out[3]  = src._14;

    out[4]  = src._21;  out[5]  = src._22;  out[6]  = src._23;  out[7]  = src._24;

    out[8]  = src._31;  out[9]  = src._32;  out[10] = src._33;  out[11] = src._34;

    out[12] = src._41;  out[13] = src._42;  out[14] = src._43;  out[15] = src._44;
}

bool FGConstantSystem::ValidateBindings() const {
    return true;
}

void FGConstantSystem::PrintUnboundConstants() const {
}

}
