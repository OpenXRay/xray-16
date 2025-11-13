#pragma once

namespace xray::render
{

class SlangCompilerTest
{
public:
    static bool RunTest();

private:
    static bool TestVertexShaderDXBC();
    static bool TestPixelShaderDXBC();
    static bool TestVertexShaderDXIL();
    static bool TestPixelShaderSPIRV();
    static bool TestErrorHandling();
};

}
