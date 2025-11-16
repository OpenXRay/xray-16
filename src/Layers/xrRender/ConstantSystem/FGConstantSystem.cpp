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

    ZeroMemory(m_engineConstants, sizeof(m_engineConstants));
    ZeroMemory(m_passConstants, sizeof(m_passConstants));
    ZeroMemory(m_materialConstants, sizeof(m_materialConstants));
    ZeroMemory(m_instanceConstants, sizeof(m_instanceConstants));

    ZeroMemory(m_staticConstants, sizeof(m_staticConstants));
}

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
        Msg("! [FGConstantSystem] Constant '%s' not found in shader", name);
        return;
    }

    if (!constant->IsMatrix()) {
        Msg("! [FGConstantSystem] Constant '%s' is not a matrix (type=%d)",
            name, (int)constant->type);
        return;
    }

    u8* stagingBuffer = GetStagingBuffer(constant->frequency);

    if (constant->IsMatrix3x4()) {
        WriteMatrix3x4(stagingBuffer + constant->offset, value);
    } else if (constant->IsMatrix4x4()) {
        WriteMatrix4x4(stagingBuffer + constant->offset, value);
    } else {
        Msg("! [FGConstantSystem] Unknown matrix type for '%s'", name);
        return;
    }

    switch (constant->frequency) {
        case UpdateFrequency::Engine:   m_dirtyEngine = ~0ull; break;
        case UpdateFrequency::Pass:     m_dirtyPass = ~0ull; break;
        case UpdateFrequency::Material: m_dirtyMaterial = ~0ull; break;
        case UpdateFrequency::Instance: m_dirtyInstance = ~0ull; break;
    }
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

    u8* stagingBuffer = GetStagingBuffer(constant->frequency);
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

    switch (constant->frequency) {
        case UpdateFrequency::Engine:   m_dirtyEngine = ~0ull; break;
        case UpdateFrequency::Pass:     m_dirtyPass = ~0ull; break;
        case UpdateFrequency::Material: m_dirtyMaterial = ~0ull; break;
        case UpdateFrequency::Instance: m_dirtyInstance = ~0ull; break;
    }
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
    const auto* engineCB = m_pso->constantLayout.constantBuffers.GetByName("static_globals");
    if (!engineCB) {
        return;
    }

    if (m_dirtyEngine == 0) {
        return;
    }

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

    ctx->WriteBuffer(engineBuffer, m_engineConstants, engineCB->size);

    m_dirtyEngine = 0;
}

void FGConstantSystem::CommitPass(ng::RenderContext* ctx) {
    m_dirtyPass = 0;
}

void FGConstantSystem::CommitMaterial(ng::RenderContext* ctx) {
    const auto* materialCB = m_pso->constantLayout.constantBuffers.GetByName("dynamic_transforms");
    if (!materialCB) {
        return;
    }

    if (m_dirtyMaterial == 0) {
        return;
    }

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
    const auto* instanceCB = m_pso->constantLayout.constantBuffers.GetByName("$Globals");
    if (!instanceCB) {
        return;
    }

    if (m_dirtyInstance == 0) {
        return;
    }

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

    u8* stagingBuffer = GetStagingBuffer(constant->frequency);
    memcpy(stagingBuffer + constant->offset, data, size);

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
