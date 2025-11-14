#pragma once

#include <slang.h>
#include <slang-com-ptr.h>
#include "xrCore/xrstring.h"

namespace xray::render
{

// Forward declaration
class SlangVFSAdapter;

/// <summary>
/// Slang shader compiler wrapper for cross-platform shader compilation
/// Compiles HLSL shaders to DXBC (DX11), DXIL (DX12), and SPIR-V (Vulkan)
/// </summary>
class SlangCompiler
{
public:
    /// <summary>
    /// Shader compilation target (output format)
    /// </summary>
    enum class Target
    {
        DXBC,      // DirectX Bytecode (Shader Model 5.0/5.1) - DX11
        DXIL,      // DirectX Intermediate Language (Shader Model 6.x) - DX12
        SPIRV,     // SPIR-V (Vulkan)
        GLSL       // GLSL (OpenGL) - future support
    };

    /// <summary>
    /// Shader stage/type
    /// </summary>
    enum class Stage
    {
        Vertex,
        Pixel,
        Compute,
        Geometry,
        Hull,      // Tessellation control (domain shader)
        Domain     // Tessellation evaluation (hull shader)
    };

    /// <summary>
    /// Compilation result containing bytecode and diagnostics
    /// </summary>
    struct CompileResult
    {
        bool success = false;
        xr_vector<u8> bytecode;
        xr_string errorMessage;
        xr_string warningMessage;

        bool IsValid() const { return success && !bytecode.empty(); }
    };

public:
    SlangCompiler();
    ~SlangCompiler();

    /// <summary>
    /// Initialize the Slang compiler
    /// Must be called before any compilation
    /// </summary>
    bool Initialize();

    /// <summary>
    /// Preprocessor macro definition
    /// </summary>
    struct Define
    {
        const char* name;
        const char* value;
    };

    /// <summary>
    /// Compile HLSL source code to target bytecode format
    /// </summary>
    /// <param name="source">HLSL source code</param>
    /// <param name="entryPoint">Entry point function name (e.g., "main", "VSMain")</param>
    /// <param name="stage">Shader stage (vertex, pixel, etc.)</param>
    /// <param name="target">Target output format (DXBC, DXIL, SPIRV)</param>
    /// <param name="sourcePath">Optional source file path for error messages</param>
    /// <param name="defines">Optional preprocessor defines</param>
    /// <param name="defineCount">Number of defines</param>
    CompileResult CompileFromSource(
        const char* source,
        const char* entryPoint,
        Stage stage,
        Target target,
        const char* sourcePath = "shader.hlsl",
        const Define* defines = nullptr,
        size_t defineCount = 0
    );

    /// <summary>
    /// Compile HLSL file to target bytecode format
    /// </summary>
    CompileResult CompileFromFile(
        const char* filePath,
        const char* entryPoint,
        Stage stage,
        Target target
    );

    /// <summary>
    /// Check if compiler is initialized and ready
    /// </summary>
    bool IsInitialized() const { return m_globalSession != nullptr; }

    /// <summary>
    /// Get human-readable target name
    /// </summary>
    static const char* GetTargetName(Target target);

    /// <summary>
    /// Get human-readable stage name
    /// </summary>
    static const char* GetStageName(Stage stage);

private:
    /// <summary>
    /// Convert our Target enum to Slang's SlangCompileTarget
    /// </summary>
    SlangCompileTarget GetSlangTarget(Target target) const;

    /// <summary>
    /// Convert our Stage enum to Slang's SlangStage
    /// </summary>
    SlangStage GetSlangStage(Stage stage) const;

private:
    Slang::ComPtr<slang::IGlobalSession> m_globalSession;
    Slang::ComPtr<slang::ISession> m_session;
    Slang::ComPtr<ISlangFileSystem> m_vfsAdapter;  // X-Ray VFS integration
};

} // namespace render
