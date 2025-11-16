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
    // Initialize with material PSO (contains ShaderConstantLayout)
    explicit FGConstantSystem(const MaterialPSO* pso);

    // ═══════════════════════════════════════════════════
    //  TYPE-SAFE CONSTANT SETTERS
    // ═══════════════════════════════════════════════════

    // Scalar/vector types
    void Set(const char* name, float value);
    void Set(const char* name, const Fvector2& value);  // float2
    void Set(const char* name, const Fvector& value);   // float3
    void Set(const char* name, const Fvector4& value);  // float4
    void Set(const char* name, const Fmatrix& value);   // float4x4

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

    // ═══════════════════════════════════════════════════
    //  VOLATILE CONSTANT STATE (VCB-BASED)
    // ═══════════════════════════════════════════════════

    // Dirty tracking for volatile constants (bitfield - max 64 per tier)
    u64 m_dirtyEngine;
    u64 m_dirtyPass;
    u64 m_dirtyMaterial;
    u64 m_dirtyInstance;

    // Staging buffers for volatile constants (stack-allocated, aligned for GPU)
    alignas(16) u8 m_engineConstants[512];   // Engine-frequency volatile
    alignas(16) u8 m_passConstants[512];     // Pass-frequency volatile
    alignas(16) u8 m_materialConstants[256]; // Material-frequency volatile
    alignas(16) u8 m_instanceConstants[256]; // Instance-frequency volatile

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
    //  HELPER METHODS
    // ═══════════════════════════════════════════════════

    // Volatile constant helpers
    void WriteConstant(const ShaderConstant* constant, const void* data, u32 size);
    u8* GetStagingBuffer(UpdateFrequency freq);
    void SetDirtyBit(UpdateFrequency freq, u32 constantIndex);

    // Static constant helpers
    void WriteStaticConstant(const ShaderConstant* constant, const void* data, u32 size);
};

} // namespace xray::render::fgconstants
