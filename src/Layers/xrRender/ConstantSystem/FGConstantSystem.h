#pragma once

#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"

namespace xray::render::fgconstants {

using namespace framegraph;

class FGConstantSystem {
public:
    explicit FGConstantSystem(
        const MaterialPSO* pso,
        framegraph::VolatileConstantBufferPool* vcbPool = nullptr
    );

    void Set(const char* name, float value);
    void Set(const char* name, const Fvector2& value);
    void Set(const char* name, const Fvector& value);
    void Set(const char* name, const Fvector4& value);
    void Set(const char* name, const Fmatrix& value);

    void SetArray(const char* name, const Fmatrix* values, u32 count);

    void CommitEngine(ng::RenderContext* ctx);
    void CommitPass(ng::RenderContext* ctx);
    void CommitMaterial(ng::RenderContext* ctx);
    void CommitInstance(ng::RenderContext* ctx);

    void CommitAll(ng::RenderContext* ctx);

    void SetStatic(const char* name, float value);
    void SetStatic(const char* name, const Fvector2& value);
    void SetStatic(const char* name, const Fvector& value);
    void SetStatic(const char* name, const Fvector4& value);
    void SetStatic(const char* name, const Fmatrix& value);

    void SetStaticArray(const char* name, const Fmatrix* values, u32 count);

    void CommitStatic(ng::RenderContext* ctx);

    void InvalidateStatic(const char* name);
    void InvalidateAllStatic();

    bool HasStaticConstants() const;

    bool ValidateBindings() const;
    void PrintUnboundConstants() const;

private:
    const MaterialPSO* m_pso;
    framegraph::VolatileConstantBufferPool* m_vcbPool;

    struct StagingBuffer {
        alignas(16) u8 data[4096];
        bool dirty = false;
        u32 size = 0;
    };

    xr_map<u16, StagingBuffer> m_engineStaging;
    xr_map<u16, StagingBuffer> m_passStaging;
    xr_map<u16, StagingBuffer> m_materialStaging;
    xr_map<u16, StagingBuffer> m_instanceStaging;

    u64 m_dirtyStatic;

    alignas(16) u8 m_staticConstants[1024];

    bool m_staticInitialized;

    void WriteConstant(const ShaderConstant* constant, const void* data, u32 size);

    u8* GetStagingBuffer(UpdateFrequency freq, u16 cbIndex);

    void SetDirtyBit(UpdateFrequency freq, u16 cbIndex);

    void WriteStaticConstant(const ShaderConstant* constant, const void* data, u32 size);

    void WriteMatrix3x4(u8* dest, const Fmatrix& src);
    void WriteMatrix4x4(u8* dest, const Fmatrix& src);
};

}
