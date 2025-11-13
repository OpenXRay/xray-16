#include "stdafx.h"
#include "SlangCompilerTest.h"
#include "SlangCompiler.h"

namespace xray::render
{

// Test shaders
static const char* s_testVertexShader = R"(
    struct VSInput
    {
        float3 position : POSITION;
        float2 texCoord : TEXCOORD0;
    };

    struct VSOutput
    {
        float4 position : SV_Position;
        float2 texCoord : TEXCOORD0;
    };

    cbuffer Constants : register(b0)
    {
        float4x4 modelViewProj;
    };

    VSOutput main(VSInput input)
    {
        VSOutput output;
        output.position = mul(float4(input.position, 1.0), modelViewProj);
        output.texCoord = input.texCoord;
        return output;
    }
)";

static const char* s_testPixelShader = R"(
    struct PSInput
    {
        float4 position : SV_Position;
        float2 texCoord : TEXCOORD0;
    };

    Texture2D baseTexture : register(t0);
    SamplerState baseSampler : register(s0);

    float4 main(PSInput input) : SV_Target
    {
        return baseTexture.Sample(baseSampler, input.texCoord);
    }
)";

bool SlangCompilerTest::RunTest()
{
    Msg("========================================");
    Msg("* [SlangCompilerTest] Starting Slang compiler tests");
    Msg("========================================");

    int passCount = 0;
    int failCount = 0;

    if (TestVertexShaderDXBC()) passCount++; else failCount++;
    if (TestPixelShaderDXBC()) passCount++; else failCount++;
    if (TestVertexShaderDXIL()) passCount++; else failCount++;
    if (TestPixelShaderSPIRV()) passCount++; else failCount++;
    if (TestErrorHandling()) passCount++; else failCount++;

    // Summary
    Msg("========================================");
    Msg("* [SlangCompilerTest] Tests Complete");
    Msg("* PASSED: %d", passCount);
    Msg("* FAILED: %d", failCount);
    Msg("* TOTAL:  %d", passCount + failCount);
    Msg("========================================");

    return failCount == 0;
}

bool SlangCompilerTest::TestVertexShaderDXBC()
{
    Msg("* [SlangCompilerTest] Test 1: Vertex Shader -> DXBC (DX11)");

    SlangCompiler compiler;
    if (!compiler.Initialize())
    {
        Msg("! [SlangCompilerTest] Test 1: FAILED - Could not initialize compiler");
        return false;
    }

    auto result = compiler.CompileFromSource(
        s_testVertexShader,
        "main",
        SlangCompiler::Stage::Vertex,
        SlangCompiler::Target::DXBC,
        "test_vs.hlsl"
    );

    if (result.IsValid())
    {
        Msg("* [SlangCompilerTest] Test 1: PASSED - Compiled %zu bytes", result.bytecode.size());
        return true;
    }
    else
    {
        Msg("! [SlangCompilerTest] Test 1: FAILED");
        if (!result.errorMessage.empty())
            Msg("! Error: %s", result.errorMessage.c_str());
        return false;
    }
}

bool SlangCompilerTest::TestPixelShaderDXBC()
{
    Msg("* [SlangCompilerTest] Test 2: Pixel Shader -> DXBC (DX11)");

    SlangCompiler compiler;
    if (!compiler.Initialize())
    {
        Msg("! [SlangCompilerTest] Test 2: FAILED - Could not initialize compiler");
        return false;
    }

    auto result = compiler.CompileFromSource(
        s_testPixelShader,
        "main",
        SlangCompiler::Stage::Pixel,
        SlangCompiler::Target::DXBC,
        "test_ps.hlsl"
    );

    if (result.IsValid())
    {
        Msg("* [SlangCompilerTest] Test 2: PASSED - Compiled %zu bytes", result.bytecode.size());
        return true;
    }
    else
    {
        Msg("! [SlangCompilerTest] Test 2: FAILED");
        if (!result.errorMessage.empty())
            Msg("! Error: %s", result.errorMessage.c_str());
        return false;
    }
}

bool SlangCompilerTest::TestVertexShaderDXIL()
{
    Msg("* [SlangCompilerTest] Test 3: Vertex Shader -> DXIL (DX12)");

    SlangCompiler compiler;
    if (!compiler.Initialize())
    {
        Msg("! [SlangCompilerTest] Test 3: FAILED - Could not initialize compiler");
        return false;
    }

    auto result = compiler.CompileFromSource(
        s_testVertexShader,
        "main",
        SlangCompiler::Stage::Vertex,
        SlangCompiler::Target::DXIL,
        "test_vs.hlsl"
    );

    if (result.IsValid())
    {
        Msg("* [SlangCompilerTest] Test 3: PASSED - Compiled %zu bytes", result.bytecode.size());
        return true;
    }
    else
    {
        Msg("! [SlangCompilerTest] Test 3: FAILED");
        if (!result.errorMessage.empty())
            Msg("! Error: %s", result.errorMessage.c_str());
        return false;
    }
}

bool SlangCompilerTest::TestPixelShaderSPIRV()
{
    Msg("* [SlangCompilerTest] Test 4: Pixel Shader -> SPIR-V (Vulkan)");

    SlangCompiler compiler;
    if (!compiler.Initialize())
    {
        Msg("! [SlangCompilerTest] Test 4: FAILED - Could not initialize compiler");
        return false;
    }

    auto result = compiler.CompileFromSource(
        s_testPixelShader,
        "main",
        SlangCompiler::Stage::Pixel,
        SlangCompiler::Target::SPIRV,
        "test_ps.hlsl"
    );

    if (result.IsValid())
    {
        Msg("* [SlangCompilerTest] Test 4: PASSED - Compiled %zu bytes", result.bytecode.size());
        return true;
    }
    else
    {
        Msg("! [SlangCompilerTest] Test 4: FAILED");
        if (!result.errorMessage.empty())
            Msg("! Error: %s", result.errorMessage.c_str());
        return false;
    }
}

bool SlangCompilerTest::TestErrorHandling()
{
    Msg("* [SlangCompilerTest] Test 5: Error Handling - Invalid Shader");

    SlangCompiler compiler;
    if (!compiler.Initialize())
    {
        Msg("! [SlangCompilerTest] Test 5: FAILED - Could not initialize compiler");
        return false;
    }

    const char* invalidShader = "this is not valid hlsl code!!!";

    auto result = compiler.CompileFromSource(
        invalidShader,
        "main",
        SlangCompiler::Stage::Vertex,
        SlangCompiler::Target::DXBC,
        "invalid.hlsl"
    );

    if (!result.IsValid() && !result.errorMessage.empty())
    {
        Msg("* [SlangCompilerTest] Test 5: PASSED - Error correctly caught");
        Msg("  Expected error: %s", result.errorMessage.c_str());
        return true;
    }
    else
    {
        Msg("! [SlangCompilerTest] Test 5: FAILED - Should have reported error");
        return false;
    }
}

} // namespace render
