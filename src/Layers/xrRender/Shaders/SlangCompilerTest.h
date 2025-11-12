#pragma once

namespace render
{

/// <summary>
/// Test harness for Slang shader compiler validation
/// Tests HLSL -> DXBC/DXIL/SPIR-V compilation pipeline
/// </summary>
class SlangCompilerTest
{
public:
    /// <summary>
    /// Run all Slang compiler tests
    /// Returns true if all tests pass
    /// </summary>
    static bool RunTest();

private:
    static bool TestVertexShaderDXBC();
    static bool TestPixelShaderDXBC();
    static bool TestVertexShaderDXIL();
    static bool TestPixelShaderSPIRV();
    static bool TestErrorHandling();
};

} // namespace render
