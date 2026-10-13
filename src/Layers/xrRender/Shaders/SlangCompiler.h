#pragma once

#include <slang.h>
#include <slang-com-ptr.h>
#include "xrCore/xrstring.h"
#include "Layers/xrRender/FrameGraph/ShaderReflection.h"

namespace xray::render
{

class SlangVFSAdapter;

class SlangCompiler
{
public:
    enum class Target
    {
        DXBC,
        DXIL,
        Metal,
        SPIRV,
        GLSL
    };

    enum class Stage
    {
        Vertex,
        Pixel,
        Compute,
        Geometry,
        Hull,
        Domain,
        Amplification,
        Mesh
    };

    struct CompileResult
    {
        bool success = false;
        xr_vector<u8> bytecode;
        xr_string errorMessage;
        xr_string warningMessage;

        Slang::ComPtr<slang::ISession> session;
        slang::IComponentType* slangProgram = nullptr;
        slang::IComponentType* linkedProgram = nullptr;
        slang::ShaderReflection* reflection = nullptr;
        xray::render::framegraph::VulkanBindShifts vkShifts;

        bool IsValid() const { return success && !bytecode.empty(); }

        ~CompileResult()
        {
            if (linkedProgram)
            {
                linkedProgram->release();
                linkedProgram = nullptr;
            }
            if (slangProgram)
            {
                slangProgram->release();
                slangProgram = nullptr;
            }
        }
    };

public:
    SlangCompiler();
    ~SlangCompiler();

    bool Initialize();

    struct Define
    {
        const char* name;
        const char* value;
    };

    CompileResult CompileFromSource(
        const char* source,
        const char* entryPoint,
        Stage stage,
        Target target,
        const char* sourcePath = "shader.hlsl",
        const Define* defines = nullptr,
        size_t defineCount = 0
    );

    CompileResult CompileFromFile(
        const char* filePath,
        const char* entryPoint,
        Stage stage,
        Target target
    );

    bool IsInitialized() const { return m_globalSession != nullptr; }

    static const char* GetTargetName(Target target);

    static const char* GetStageName(Stage stage);

private:
    SlangCompileTarget GetSlangTarget(Target target) const;

    SlangStage GetSlangStage(Stage stage) const;

private:
    Slang::ComPtr<slang::IGlobalSession> m_globalSession;
    Slang::ComPtr<slang::ISession> m_session;
    Slang::ComPtr<ISlangFileSystem> m_vfsAdapter;
};

}
