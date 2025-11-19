#pragma once

#include "Layers/xrRender/FrameGraph/ShaderReflection.h"
#include "Layers/xrRender/RenderContext/RenderContext.h"
#include "Layers/xrRender/Geometry/MaterialCache.h"

namespace xray::render::fgconstants {

using namespace framegraph;

// ═══════════════════════════════════════════════════
//  FG CONSTANT SYSTEM (TYPE-SAFE CONSTANT BINDING API)
// ═══════════════════════════════════════════════════
//
// Provides type-safe, frequency-aware constant binding for shaders.
// Replaces manual memcpy + WriteBuffer with named constant setters.
//
// Features:
// - Type-safe constant setters (Set<T>("name", value))
// - Automatic dirty tracking (only upload changed constants)
// - Frequency-based batching (Engine/Pass/Material/Instance)
// - Stack-allocated staging buffers (no heap allocations)
//
// Usage:
//   FGConstantSystem constants(materialPSO);
//   constants.Set("m_World", worldMatrix);
//   constants.Set("dt_params", detailParams);
//   constants.CommitInstance(ctx);  // Upload dirty constants
//
class FGConstantSystem {
public:
    // Initialize with material PSO and VCB pool for per-instance CB management
    explicit FGConstantSystem(
        const MaterialPSO* pso,
        framegraph::VolatileConstantBufferPool* vcbPool = nullptr
    );

    // ═══════════════════════════════════════════════════
    //  TYPE-SAFE CONSTANT SETTERS
    // ═══════════════════════════════════════════════════

    // Scalar/vector types
    void Set(const char* name, float value);
    void Set(const char* name, const Fvector2& value);  // float2
    void Set(const char* name, const Fvector& value);   // float3
    void Set(const char* name, const Fvector4& value);  // float4
    void Set(const char* name, const Fmatrix& value);   // float4x4 or float3x4 (auto-detected)

    // Matrix arrays - for bone transforms (m_xform[64])
    void SetArray(const char* name, const Fmatrix* values, u32 count);

    // Texture/sampler types (future - for now use MaterialCache binding sets)
    // void Set(const char* name, nvrhi::TextureHandle texture);
    // void Set(const char* name, nvrhi::SamplerHandle sampler);

    // ═══════════════════════════════════════════════════
    //  FREQUENCY-BASED COMMIT (VOLATILE CONSTANTS)
    // ═══════════════════════════════════════════════════

    // Commit volatile constants (uploaded every frame/pass/draw)
    void CommitEngine(ng::RenderContext* ctx);   // Once per frame
    void CommitPass(ng::RenderContext* ctx);     // Once per pass
    void CommitMaterial(ng::RenderContext* ctx); // Per material
    void CommitInstance(ng::RenderContext* ctx); // Per draw call

    // Batch commit (for efficiency when all frequencies need update)
    void CommitAll(ng::RenderContext* ctx);

    // ═══════════════════════════════════════════════════
    //  STATIC CONSTANT API (PERSISTENT CONSTANTS)
    // ═══════════════════════════════════════════════════

    // Set static constants (uploaded once, cached across frames)
    void SetStatic(const char* name, float value);
    void SetStatic(const char* name, const Fvector2& value);
    void SetStatic(const char* name, const Fvector& value);
    void SetStatic(const char* name, const Fvector4& value);
    void SetStatic(const char* name, const Fmatrix& value);

    // Matrix arrays (static)
    void SetStaticArray(const char* name, const Fmatrix* values, u32 count);

    // Commit static constants (upload if dirty, bind always)
    // First frame: Uploads data to GPU
    // Later frames: Just binds existing buffer (no upload)
    void CommitStatic(ng::RenderContext* ctx);

    // Invalidate static constant (force re-upload on next commit)
    void InvalidateStatic(const char* name);
    void InvalidateAllStatic();

    // Query static state
    bool HasStaticConstants() const;

    // ═══════════════════════════════════════════════════
    //  DEBUG VALIDATION
    // ═══════════════════════════════════════════════════

    bool ValidateBindings() const;
    void PrintUnboundConstants() const;

private:
    const MaterialPSO* m_pso;
    framegraph::VolatileConstantBufferPool* m_vcbPool;

    // ═══════════════════════════════════════════════════
    //  VOLATILE CONSTANT STATE (VCB-BASED - PER-CB STAGING)
    // ═══════════════════════════════════════════════════

    // Per-CB staging buffers (one staging buffer per CB at each frequency)
    // This enables:
    // - Uploading only dirty CBs (not entire frequency tier)
    // - Different sizes per CB (SkeletonBones: 3072 bytes vs PerInstanceTransforms: 160 bytes)
    // - Reflection-driven routing (no hardcoded CB names)
    struct StagingBuffer {
        alignas(16) u8 data[4096];  // Max CB size (covers SkeletonBones: 3072 bytes)
        bool dirty = false;
        u32 size = 0;                // Actual CB size from reflection
    };

    // Per-frequency, per-CB staging buffers (cbIndex → staging)
    xr_map<u16, StagingBuffer> m_engineStaging;
    xr_map<u16, StagingBuffer> m_passStaging;
    xr_map<u16, StagingBuffer> m_materialStaging;
    xr_map<u16, StagingBuffer> m_instanceStaging;

    // ═══════════════════════════════════════════════════
    //  STATIC CONSTANT STATE (PERSISTENT CB-BASED)
    // ═══════════════════════════════════════════════════

    // Dirty tracking for static constants (per-CB bitfield)
    u64 m_dirtyStatic;

    // Static constant staging buffer (persistent, larger for engine data)
    alignas(16) u8 m_staticConstants[1024];

    // Initialization flag (static CBs uploaded once)
    bool m_staticInitialized;

    // ═══════════════════════════════════════════════════
    //  HELPER METHODS (REFLECTION-DRIVEN ROUTING)
    // ═══════════════════════════════════════════════════

    // Volatile constant helpers (per-CB staging)
    void WriteConstant(const ShaderConstant* constant, const void* data, u32 size);

    // Get staging buffer for specific CB at given frequency
    // Returns pointer to staging data for writing constants
    u8* GetStagingBuffer(UpdateFrequency freq, u16 cbIndex);

    // Mark CB as dirty (needs upload on next commit)
    void SetDirtyBit(UpdateFrequency freq, u16 cbIndex);

    // Static constant helpers
    void WriteStaticConstant(const ShaderConstant* constant, const void* data, u32 size);

    // Matrix conversion helpers (Fmatrix → HLSL row-major format)
    void WriteMatrix3x4(u8* dest, const Fmatrix& src);  // 48 bytes (bone transforms)
    void WriteMatrix4x4(u8* dest, const Fmatrix& src);  // 64 bytes (world/view/proj matrices)
};

} // namespace xray::render::fgconstants
