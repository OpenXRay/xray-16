#pragma once

#include "xrCore/xrstring.h"

// Forward declarations for Slang reflection
namespace slang {
    struct ShaderReflection;
}

namespace xray::render::framegraph {

// ═══════════════════════════════════════════════════
//  RENDER PHASE TYPES
// ═══════════════════════════════════════════════════
//
// Determines which rendering phase a shader belongs to.
// Used for automatic pass routing and RT binding.
//
enum class RenderPhase {
    Geometry,      // Outputs to GBuffer (rt_Position, rt_Normal, rt_Color)
    Lighting,      // Outputs to rt_Accumulator
    Combine,       // Outputs to rt_Generic_0/1
    PostProcess,   // Various outputs (bloom, tonemap, etc.)
    Shadow,        // Shadow map generation
    Custom         // User-defined
};

// ═══════════════════════════════════════════════════
//  VERTEX SHADER INPUT SIGNATURE
// ═══════════════════════════════════════════════════
//
// Extracted from VS using Slang reflection.
// Defines the required vertex attributes in shader-expected order.
//
struct VertexInputSignature {
    struct InputElement {
        shared_str semanticName;   // "POSITION", "TEXCOORD", etc.
        u32 semanticIndex;          // 0, 1, 2, etc.
        nvrhi::Format format;         // R32G32B32_FLOAT, R8G8B8A8_UNORM, etc.
        u32 inputSlot;              // Buffer slot (usually 0)

        InputElement() : semanticIndex(0), format(nvrhi::Format::UNKNOWN), inputSlot(0) {}
    };
    xr_vector<InputElement> elements;  // In shader-expected order!
};

// ═══════════════════════════════════════════════════
//  SHADER RT BINDINGS
// ═══════════════════════════════════════════════════
//
// Extracted from shader using Slang reflection.
// Contains all texture inputs/outputs for automatic RT binding.
//
struct ShaderRTBindings {
    RenderPhase phase = RenderPhase::Custom;

    // ─── Input Textures (SRVs) ───
    struct InputTexture {
        shared_str name;       // "s_position", "s_normal", etc.
        u32 slot;              // Texture slot (t0, t1, ...)

        InputTexture() : slot(0) {}
        InputTexture(const char* n, u32 s) : name(n), slot(s) {}
    };
    xr_vector<InputTexture> inputTextures;

    // ─── Samplers ───
    struct Sampler {
        shared_str name;       // "smp_base", "smp_rtlinear", etc.
        u32 slot;              // Sampler slot (s0, s1, ...)

        // Sampler state metadata (from X-Ray naming conventions)
        // See gamedata/shaders/r5/common_samplers.h for authoritative definitions
        enum class FilterMode : u8 {
            Point,        // D3DTEXF_POINT
            Linear,       // D3DTEXF_LINEAR
            Anisotropic   // D3DTEXF_ANISOTROPIC
        };

        enum class AddressMode : u8 {
            Wrap,   // D3DTADDRESS_WRAP
            Clamp   // D3DTADDRESS_CLAMP
        };

        struct SamplerState {
            AddressMode addressMode;
            FilterMode minFilter;
            FilterMode magFilter;
            FilterMode mipFilter;
            u8 maxAnisotropy;  // 1, 8, 16, etc.

            SamplerState()
                : addressMode(AddressMode::Wrap)
                , minFilter(FilterMode::Linear)
                , magFilter(FilterMode::Linear)
                , mipFilter(FilterMode::Linear)
                , maxAnisotropy(1) {}
        };

        Sampler() : slot(0) {}
        Sampler(const char* n, u32 s) : name(n), slot(s) {}

        // Infer sampler state from X-Ray naming convention
        // Based on common_samplers.h shader definitions
        SamplerState GetExpectedState() const {
            SamplerState state;

            if (strstr(name.c_str(), "smp_rtlinear") != nullptr) {
                // D3DTADDRESS_CLAMP, D3DTEXF_LINEAR, D3DTEXF_NONE, D3DTEXF_LINEAR
                state.addressMode = AddressMode::Clamp;
                state.minFilter = FilterMode::Linear;
                state.magFilter = FilterMode::Linear;
                state.mipFilter = FilterMode::Linear;
                state.maxAnisotropy = 1;
            }
            else if (strstr(name.c_str(), "smp_linear") != nullptr) {
                // D3DTADDRESS_WRAP, D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR
                state.addressMode = AddressMode::Wrap;
                state.minFilter = FilterMode::Linear;
                state.magFilter = FilterMode::Linear;
                state.mipFilter = FilterMode::Linear;
                state.maxAnisotropy = 1;
            }
            else if (strstr(name.c_str(), "smp_base") != nullptr ||
                     strstr(name.c_str(), "smp_material") != nullptr ||
                     strstr(name.c_str(), "smp_bump") != nullptr) {
                // D3DTADDRESS_WRAP, D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_ANISOTROPIC
                state.addressMode = AddressMode::Wrap;
                state.minFilter = FilterMode::Anisotropic;
                state.magFilter = FilterMode::Anisotropic;
                state.mipFilter = FilterMode::Linear;
                state.maxAnisotropy = 8;
            }
            else if (strstr(name.c_str(), "smp_nofilter") != nullptr ||
                     strstr(name.c_str(), "smp_smap") != nullptr ||
                     strstr(name.c_str(), "smp_jitter") != nullptr) {
                // D3DTADDRESS_CLAMP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT
                state.addressMode = AddressMode::Clamp;
                state.minFilter = FilterMode::Point;
                state.magFilter = FilterMode::Point;
                state.mipFilter = FilterMode::Point;
                state.maxAnisotropy = 1;
            }
            else if (strstr(name.c_str(), "PointClamp") != nullptr) {
                // Fluid sim samplers
                state.addressMode = AddressMode::Clamp;
                state.minFilter = FilterMode::Point;
                state.magFilter = FilterMode::Point;
                state.mipFilter = FilterMode::Point;
                state.maxAnisotropy = 1;
            }
            else if (strstr(name.c_str(), "LinearClamp") != nullptr) {
                state.addressMode = AddressMode::Clamp;
                state.minFilter = FilterMode::Linear;
                state.magFilter = FilterMode::Linear;
                state.mipFilter = FilterMode::Linear;
                state.maxAnisotropy = 1;
            }
            else {
                // Default: linear, wrap
                state.addressMode = AddressMode::Wrap;
                state.minFilter = FilterMode::Linear;
                state.magFilter = FilterMode::Linear;
                state.mipFilter = FilterMode::Linear;
                state.maxAnisotropy = 1;
            }

            return state;
        }

        // Create NVRHI sampler from reflection metadata
        nvrhi::SamplerHandle CreateNVRHISampler(nvrhi::IDevice* device) const {
            auto samplerState = GetExpectedState();

            nvrhi::SamplerDesc nvrhiDesc;

            // Convert filter modes
            auto isLinearOrAniso = [](FilterMode mode) {
                return mode == FilterMode::Linear || mode == FilterMode::Anisotropic;
            };

            nvrhiDesc.setMinFilter(isLinearOrAniso(samplerState.minFilter));
            nvrhiDesc.setMagFilter(isLinearOrAniso(samplerState.magFilter));
            nvrhiDesc.setMipFilter(isLinearOrAniso(samplerState.mipFilter));

            // Convert address mode
            nvrhi::SamplerAddressMode nvrhiAddressMode =
                (samplerState.addressMode == AddressMode::Clamp)
                    ? nvrhi::SamplerAddressMode::Clamp
                    : nvrhi::SamplerAddressMode::Wrap;
            nvrhiDesc.setAllAddressModes(nvrhiAddressMode);

            // Set anisotropy
            nvrhiDesc.setMaxAnisotropy(static_cast<float>(samplerState.maxAnisotropy));

            return device->createSampler(nvrhiDesc);
        }
    };
    xr_vector<Sampler> samplers;

    // ─── Output RTs (RTVs) ───
    enum class RTSemantic {
        Unknown,
        Normal,      // World-space or view-space normal
        Albedo,      // Base color / diffuse
        Material,    // Material properties (metallic, roughness, AO)
        Position,    // World-space or view-space position
        Emissive,    // Emissive color
        Accumulator  // Lighting accumulation buffer
    };

    struct OutputRT {
        u32 slot;              // SV_Target index
        RTSemantic semantic;   // Inferred semantic meaning
        shared_str formatDesc; // Format description for debugging

        OutputRT() : slot(0), semantic(RTSemantic::Unknown) {}
        OutputRT(u32 s) : slot(s), semantic(RTSemantic::Unknown) {}
    };
    xr_vector<OutputRT> outputRTs;

    // ─── Depth Output ───
    bool hasDepthOutput = false;

    // ─── Debug Info ───
    shared_str shaderName;
};

// ═══════════════════════════════════════════════════
//  CONSTANT BUFFER INFO
// ═══════════════════════════════════════════════════
//
// Information about a constant buffer used by a shader.
// Used to determine required VCB sizes for different object types.
//
struct ConstantBufferInfo {
    shared_str name;      // "PerObject", "PerFrame", etc.
    u32 slot;             // Register slot (b0, b1, ...)
    u32 size;             // Size in bytes

    ConstantBufferInfo() : slot(0), size(0) {}
    ConstantBufferInfo(const char* n, u32 s, u32 sz) : name(n), slot(s), size(sz) {}
};

// ═══════════════════════════════════════════════════
//  UPDATE FREQUENCY (FOR CONSTANT BINDING OPTIMIZATION)
// ═══════════════════════════════════════════════════
//
// Determines how frequently a constant needs to be updated.
// Used by FGConstantSystem to batch updates and reduce GPU uploads.
//
enum class UpdateFrequency : u8 {
    Engine = 0,    // Once per frame (samplers, global state)
    Pass = 1,      // Once per pass (view/proj matrices, pass RTs)
    Material = 2,  // Once per material (textures, material params)
    Instance = 3   // Once per instance (world matrix, bones)
};

// ═══════════════════════════════════════════════════
//  CONSTANT PERSISTENCE (LIFETIME & ALLOCATION STRATEGY)
// ═══════════════════════════════════════════════════
//
// Determines the lifetime and allocation strategy for constants.
// Volatile: Per-frame transient (VCB ring buffer)
// Static: Cross-frame persistent (cached GPU buffer)
// SemiStatic: Occasional updates (e.g., on resize)
//
enum class ConstantPersistence : u8 {
    Volatile = 0,   // Updated frequently - uses VCB ring buffer
    Static = 1,     // Set once, cached across frames - uses persistent constantBuffers
    SemiStatic = 2  // Updated occasionally (e.g., resolution change)
};

// ═══════════════════════════════════════════════════
//  SHADER CONSTANT (INDIVIDUAL PARAMETER METADATA)
// ═══════════════════════════════════════════════════
//
// Metadata for a single constant within a constant buffer.
// Contains type, size, offset, update frequency, and persistence information.
//
struct ShaderConstant {
    shared_str name;                  // "m_World", "m_VP", "dt_params", etc.
    u32 offset;                       // Byte offset within CB
    u32 size;                         // Size in bytes (16 for float4, 64 for float4x4, etc.)
    UpdateFrequency frequency;        // Binding frequency
    ConstantPersistence persistence;  // Allocation strategy (volatile VCB vs static CB)
    u16 cbIndex;                      // Which CB contains this constant (index into ShaderConstantBuffers::buffers)

    // ═══════════════════════════════════════════════════
    // NEW: Type Metadata for Matrix/Array Support
    // ═══════════════════════════════════════════════════

    enum class Type : u8 {
        Scalar,      // float, int, uint
        Vector,      // float2, float3, float4
        Matrix3x4,   // float3x4 (48 bytes) - bone transforms
        Matrix4x4,   // float4x4 (64 bytes) - world/view/proj matrices
        Struct,      // Custom struct
        Array        // Array of any type (float4[], float3x4[], etc.)
    };

    Type type = Type::Scalar;
    u16 arrayCount = 1;              // 1 for non-arrays, N for T[N]
    u16 elementSize = 0;             // Size of single element (for arrays)
    u16 matrixStride = 0;            // 0 for non-matrices, 16 for matrices

    // Helper predicates
    bool IsArray() const { return arrayCount > 1; }
    bool IsMatrix() const { return type == Type::Matrix3x4 || type == Type::Matrix4x4; }
    bool IsMatrix3x4() const { return type == Type::Matrix3x4; }
    bool IsMatrix4x4() const { return type == Type::Matrix4x4; }

    ShaderConstant()
        : offset(0), size(0), frequency(UpdateFrequency::Instance)
        , persistence(ConstantPersistence::Volatile), cbIndex(0) {}
};

// ═══════════════════════════════════════════════════
//  SHADER CONSTANT BUFFER LAYOUT
// ═══════════════════════════════════════════════════
//
// Collection of all constant buffers used by a shader.
// Can be used to identify unique CB layouts and create appropriate VCBs.
//
struct ShaderConstantBuffers {
    xr_vector<ConstantBufferInfo> buffers;

    // Helper: Get CB by slot
    const ConstantBufferInfo* GetBySlot(u32 slot) const {
        for (const auto& cb : buffers) {
            if (cb.slot == slot) return &cb;
        }
        return nullptr;
    }

    // Helper: Get CB by name
    const ConstantBufferInfo* GetByName(const char* name) const {
        for (const auto& cb : buffers) {
            if (xr_strcmp(cb.name.c_str(), name) == 0) return &cb;
        }
        return nullptr;
    }
};

// ═══════════════════════════════════════════════════
//  SHADER CONSTANT LAYOUT (FULL CB + INDIVIDUAL CONSTANTS)
// ═══════════════════════════════════════════════════
//
// Extended layout that includes both CB-level metadata and per-constant metadata.
// Used by FGConstantSystem for type-safe constant binding.
//
struct ShaderConstantLayout {
    ShaderConstantBuffers constantBuffers;  // Existing CB-level metadata
    xr_vector<ShaderConstant> constants;     // NEW: Per-constant metadata

    // Helper: Find constant by name
    const ShaderConstant* FindConstant(const char* name) const {
        for (const auto& c : constants) {
            if (xr_strcmp(c.name.c_str(), name) == 0) return &c;
        }
        return nullptr;
    }

    // Helper: Get constants for a specific frequency
    xr_vector<const ShaderConstant*> GetConstantsByFrequency(UpdateFrequency freq) const {
        xr_vector<const ShaderConstant*> result;
        for (const auto& c : constants) {
            if (c.frequency == freq) result.push_back(&c);
        }
        return result;
    }
};

// ═══════════════════════════════════════════════════
//  SHADER REFLECTOR
// ═══════════════════════════════════════════════════
//
// Analyzes shaders using Slang reflection to extract render target bindings,
// vertex input signatures, and infer rendering phase.
//
// NOTE: Now uses Slang reflection directly (cross-platform, no D3DReflect!)
//
class ShaderReflector {
public:
    // ═══════════════════════════════════════════════════
    //  REFLECTION EXTRACTION (from live Slang compilation)
    // ═══════════════════════════════════════════════════

    // Extract full reflection data from Slang (call once after compilation)
    static ExtractedReflection ExtractReflection(
        slang::ShaderReflection* slangReflection,
        bool isVertexShader);

    // ═══════════════════════════════════════════════════
    //  REFLECTION ACCESSORS (from ExtractedReflection)
    // ═══════════════════════════════════════════════════

    // Get vertex input signature from extracted reflection
    static const VertexInputSignature& GetVertexInputSignature(
        const ExtractedReflection* reflection);

    // Get RT bindings from extracted reflection
    static const ShaderRTBindings& GetRTBindings(
        const ExtractedReflection* reflection);

    // Get constant buffers from extracted reflection
    static const ShaderConstantBuffers& GetConstantBuffers(
        const ExtractedReflection* reflection);

    // Infer render phase from shader bindings
    static RenderPhase InferPhase(const ShaderRTBindings& bindings);

    // Get typical RT names for a phase
    static xr_vector<const char*> GetPhaseRTNames(RenderPhase phase);

    // Infer RT semantic from output signature
    static ShaderRTBindings::RTSemantic InferRTSemantic(
        u32 slot,
        u32 componentMask,
        RenderPhase phase);

private:
    // Internal helpers (extract data from live Slang reflection)
    static VertexInputSignature AnalyzeVertexShader(slang::ShaderReflection* reflection);
    static ShaderRTBindings AnalyzePixelShader(slang::ShaderReflection* reflection);
    static ShaderConstantBuffers AnalyzeConstantBuffers(slang::ShaderReflection* reflection);
    static ShaderConstantLayout AnalyzeConstantLayout(slang::ShaderReflection* reflection);

    // Helper: Infer constant update frequency from name/CB
    static UpdateFrequency InferConstantFrequency(const char* constantName, const char* cbName);

    // Helper: Infer constant persistence (static vs volatile) from CB name and frequency
    static ConstantPersistence InferConstantPersistence(const char* cbName, UpdateFrequency frequency);

    // Helper: check if texture name matches pattern
    static bool MatchesPattern(const char* name, const char* pattern);
};

} // namespace xray::render::framegraph
