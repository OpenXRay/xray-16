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

    alignas(16) float data[4] = { value.x, value.y, 0.0f, 0.0f };
    WriteConstant(constant, data, 16);
}

void FGConstantSystem::Set(const char* name, const Fvector& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        return;
    }

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

    u16 cbIndex = constant->cbIndex;
    const auto& cbInfo = m_pso->constantLayout.constantBuffers.buffers[cbIndex];

    u8* stagingBuffer = GetStagingBuffer(constant->frequency, cbIndex);

    if (constant->IsMatrix3x4()) {
        WriteMatrix3x4(stagingBuffer + constant->offset, value);
    } else if (constant->IsMatrix4x4()) {
        WriteMatrix4x4(stagingBuffer + constant->offset, value);
    } else {
        return;
    }

    SetDirtyBit(constant->frequency, cbIndex);
}

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

    alignas(16) float data[4] = { value.x, value.y, 0.0f, 0.0f };
    WriteStaticConstant(constant, data, 16);
}

void FGConstantSystem::SetStatic(const char* name, const Fvector& value) {
    const ShaderConstant* constant = m_pso->FindConstant(name);
    if (!constant) {
        return;
    }

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
        return;
    }

    if (!constant->IsArray()) {
        return;
    }

    if (count > constant->arrayCount) {
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
        return;
    }

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
        return;
    }

    for (auto& [cbIndex, staging] : m_instanceStaging) {
        if (!staging.dirty) {
            continue;
        }

        if (cbIndex >= m_pso->constantLayout.constantBuffers.buffers.size()) {
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
            continue;
        }

        nvrhi::IBuffer* nvrhiBuffer = ctx->GetDevice()->GetNativeBuffer(vcbHandle);
        if (!nvrhiBuffer) {
            continue;
        }

        ctx->WriteBuffer(nvrhiBuffer, staging.data, staging.size);

        staging.dirty = false;
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
    memcpy(dest, &src, 48);
}

void FGConstantSystem::WriteMatrix4x4(u8* dest, const Fmatrix& src) {
    memcpy(dest, &src, sizeof(Fmatrix));
}

bool FGConstantSystem::ValidateBindings() const {
    return true;
}

void FGConstantSystem::PrintUnboundConstants() const {
}

}
