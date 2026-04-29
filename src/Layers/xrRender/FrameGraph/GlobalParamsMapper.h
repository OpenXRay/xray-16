// xrRender/FrameGraph/GlobalParamsMapper.h
#pragma once

#include "Layers/xrRender/Shaders/SlangReflectionWrapper.h"
#include "xrCore/xrstring.h"

// Forward declarations
namespace slang {
    struct ShaderReflection;
}

namespace xray::render {

namespace fg {
    // Forward declarations (renderer-specific)
    class R_constant_table;
    class CBackend;
}

/// <summary>
/// Maps Slang's globalParams_0 constant buffer to X-Ray's uniform system
///
/// Slang automatically wraps loose uniforms into a cbuffer called globalParams_0.
/// This class extracts the layout of that buffer from Slang reflection and
/// populates it with values from X-Ray's constant system.
///
/// Flow:
///   1. Shader compilation -> Slang wraps loose uniforms -> globalParams_0
///   2. ExtractLayout() -> Query Slang reflection -> Store member layout
///   3. PopulateBuffer() -> Query X-Ray constants -> Copy to buffer
/// </summary>
class GlobalParamsMapper {
public:
    /// <summary>
    /// Information about a uniform member within globalParams_0
    /// </summary>
    struct UniformMember {
        shared_str name;          // Name (e.g., "m_AlphaRef", "screen_res")
        u32 offset;               // Byte offset in globalParams_0
        u32 size;                 // Size in bytes
        slang::TypeReflection::ScalarType type;  // Scalar type (float, int, etc.)
    };

    GlobalParamsMapper() = default;
    ~GlobalParamsMapper() = default;

    /// <summary>
    /// Extract globalParams_0 layout from Slang reflection
    /// Called once during PSO creation
    /// </summary>
    /// <param name="reflection">Slang shader reflection</param>
    /// <returns>True if globalParams_0 was found and extracted</returns>
    bool ExtractLayout(slang::ShaderReflection* reflection);

    /// <summary>
    /// Populate buffer from X-Ray constant table
    /// Called every frame before draw
    /// </summary>
    /// <param name="bufferData">192-byte buffer to fill</param>
    /// <param name="constantTable">Shader's constant table (contains R_constant handlers)</param>
    /// <param name="backend">X-Ray render backend (for calling setup())</param>
    void PopulateBuffer(
        void* bufferData,
        const fg::R_constant_table* constantTable,
        fg::CBackend& backend);

    /// <summary>
    /// Check if this mapper has a valid layout
    /// </summary>
    bool HasGlobalParams() const { return !m_members.empty(); }

    /// <summary>
    /// Get the total size of globalParams_0 buffer
    /// </summary>
    u32 GetBufferSize() const { return m_bufferSize; }

    /// <summary>
    /// Get the list of member uniforms
    /// </summary>
    const xr_vector<UniformMember>& GetMembers() const { return m_members; }

private:
    xr_vector<UniformMember> m_members;
    u32 m_bufferSize = 0;
};

} // namespace xray::render
