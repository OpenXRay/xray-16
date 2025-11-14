#include "stdafx.h"
#include "SlangCompiler.h"
#include "SlangVFSAdapter.h"
#include "xrCore/xrMemory.h"

namespace xray::render
{

SlangCompiler::SlangCompiler()
    : m_globalSession(nullptr)
    , m_session(nullptr)
{
}

SlangCompiler::~SlangCompiler()
{
    // Slang::ComPtr handles cleanup automatically
}

bool SlangCompiler::Initialize()
{
    if (IsInitialized())
        return true;

    // Create global session
    SlangResult result = slang::createGlobalSession(m_globalSession.writeRef());
    if (SLANG_FAILED(result) || !m_globalSession)
    {
        Msg("! [SlangCompiler] Failed to create global session");
        return false;
    }

    // Create VFS adapter for X-Ray virtual file system integration
    m_vfsAdapter = xr_new<SlangVFSAdapter>();
    if (!m_vfsAdapter)
    {
        Msg("! [SlangCompiler] Failed to create VFS adapter");
        return false;
    }

    Msg("* [SlangCompiler] Initialized successfully (VFS integration active)");
    return true;
}

SlangCompiler::CompileResult SlangCompiler::CompileFromSource(
    const char* source,
    const char* entryPoint,
    Stage stage,
    Target target,
    const char* sourcePath,
    const Define* defines,
    size_t defineCount)
{
    CompileResult result;

    if (!IsInitialized())
    {
        result.errorMessage = "SlangCompiler not initialized. Call Initialize() first.";
        Msg("! [SlangCompiler] %s", result.errorMessage.c_str());
        return result;
    }

    // Convert our Define structs to Slang's PreprocessorMacroDesc
    xr_vector<slang::PreprocessorMacroDesc> slangDefines;
    if (defines && defineCount > 0)
    {
        slangDefines.reserve(defineCount);
        for (size_t i = 0; i < defineCount; ++i)
        {
            slang::PreprocessorMacroDesc macro;
            macro.name = defines[i].name;
            macro.value = defines[i].value;
            slangDefines.push_back(macro);
        }
    }

    // Create session for this compilation
    slang::SessionDesc sessionDesc = {};
    slang::TargetDesc targetDesc = {};
    targetDesc.format = GetSlangTarget(target);

    // Choose shader model based on target
    const char* profile = nullptr;
    switch (target)
    {
    case Target::DXBC:
        profile = "sm_5_0"; // Shader Model 5.0 for DX11
        break;
    case Target::DXIL:
        profile = "sm_6_0"; // Shader Model 6.0 for DX12
        break;
    case Target::SPIRV:
        profile = "sm_5_0"; // Use SM 5.0 equivalent for Vulkan
        break;
    default:
        profile = "sm_5_0";
        break;
    }

    targetDesc.profile = m_globalSession->findProfile(profile);

    sessionDesc.targets = &targetDesc;
    sessionDesc.targetCount = 1;

    // Pass preprocessor defines to session
    if (!slangDefines.empty())
    {
        sessionDesc.preprocessorMacros = slangDefines.data();
        sessionDesc.preprocessorMacroCount = slangDefines.size();
    }

    // Configure compiler options to preserve bindings and prevent reordering
    slang::CompilerOptionEntry compilerOptions[2];

    // Disable name mangling to preserve original resource names
    compilerOptions[0].name = slang::CompilerOptionName::NoMangle;
    compilerOptions[0].value.kind = slang::CompilerOptionValueKind::Int;
    compilerOptions[0].value.intValue0 = 1;

    // Preserve all parameters to prevent constant buffer reordering
    compilerOptions[1].name = slang::CompilerOptionName::PreserveParameters;
    compilerOptions[1].value.kind = slang::CompilerOptionValueKind::Int;
    compilerOptions[1].value.intValue0 = 1;

    sessionDesc.compilerOptionEntries = compilerOptions;
    sessionDesc.compilerOptionEntryCount = 2;

    // Use VFS adapter for file system operations (includes from VFS)
    sessionDesc.fileSystem = m_vfsAdapter.get();

    Slang::ComPtr<slang::ISession> session;
    SlangResult slangResult = m_globalSession->createSession(sessionDesc, session.writeRef());
    if (SLANG_FAILED(slangResult) || !session)
    {
        result.errorMessage = "Failed to create Slang session";
        Msg("! [SlangCompiler] %s", result.errorMessage.c_str());
        return result;
    }

    // Create compilation request
    Slang::ComPtr<slang::ICompileRequest> request;
    slangResult = session->createCompileRequest(request.writeRef());
    if (SLANG_FAILED(slangResult) || !request)
    {
        result.errorMessage = "Failed to create compile request";
        Msg("! [SlangCompiler] %s", result.errorMessage.c_str());
        return result;
    }

    // Note: File includes are handled by VFS adapter (no need for addSearchPath)

    // Add translation unit (source code)
    int translationUnitIndex = request->addTranslationUnit(SLANG_SOURCE_LANGUAGE_SLANG, sourcePath);
    request->addTranslationUnitSourceString(translationUnitIndex, sourcePath, source);

    // Add entry point
    SlangStage slangStage = GetSlangStage(stage);
    request->addEntryPoint(translationUnitIndex, entryPoint, slangStage);

    // Compile
    slangResult = request->compile();

    // Extract diagnostics (errors/warnings)
    const char* diagnostics = request->getDiagnosticOutput();
    if (diagnostics && diagnostics[0])
    {
        result.errorMessage = diagnostics;
    }

    // Check for errors
    if (SLANG_FAILED(slangResult))
    {
        result.success = false;
        if (result.errorMessage.empty())
            result.errorMessage = "Compilation failed";

        Msg("! [SlangCompiler] Compilation failed for %s (entry: %s, stage: %s, target: %s)",
            sourcePath, entryPoint, GetStageName(stage), GetTargetName(target));
        if (!result.errorMessage.empty())
            Msg("! [SlangCompiler] Error: %s", result.errorMessage.c_str());

        return result;
    }

    // Get compiled bytecode
    Slang::ComPtr<slang::IBlob> codeBlob;
    slangResult = request->getEntryPointCodeBlob(0, 0, (ISlangBlob**)codeBlob.writeRef());
    if (SLANG_FAILED(slangResult) || !codeBlob)
    {
        result.errorMessage = "Failed to retrieve compiled bytecode";
        result.success = false;
        Msg("! [SlangCompiler] %s", result.errorMessage.c_str());
        return result;
    }

    // Copy bytecode to result
    const u8* bytecodeData = static_cast<const u8*>(codeBlob->getBufferPointer());
    size_t bytecodeSize = codeBlob->getBufferSize();

    result.bytecode.resize(bytecodeSize);
    std::memcpy(result.bytecode.data(), bytecodeData, bytecodeSize);
    result.success = true;

    Msg("* [SlangCompiler] Successfully compiled %s (entry: %s, stage: %s, target: %s) -> %zu bytes",
        sourcePath, entryPoint, GetStageName(stage), GetTargetName(target), bytecodeSize);

    if (!result.warningMessage.empty())
    {
        Msg("~ [SlangCompiler] Warnings: %s", result.warningMessage.c_str());
    }

    return result;
}

SlangCompiler::CompileResult SlangCompiler::CompileFromFile(
    const char* filePath,
    const char* entryPoint,
    Stage stage,
    Target target)
{
    CompileResult result;

    // Read file contents
    IReader* reader = FS.r_open("$game_shaders$", filePath);
    if (!reader)
    {
        result.errorMessage = xr_string("Failed to open shader file: ") + filePath;
        result.success = false;
        Msg("! [SlangCompiler] %s", result.errorMessage.c_str());
        return result;
    }

    // Convert to string
    xr_string source(static_cast<const char*>(reader->pointer()), reader->length());
    FS.r_close(reader);

    // Compile from source
    return CompileFromSource(source.c_str(), entryPoint, stage, target, filePath);
}

SlangCompileTarget SlangCompiler::GetSlangTarget(Target target) const
{
    switch (target)
    {
    case Target::DXBC:
        return SLANG_DXBC;
    case Target::DXIL:
        return SLANG_DXIL;
    case Target::SPIRV:
        return SLANG_SPIRV;
    case Target::GLSL:
        return SLANG_GLSL;
    default:
        Msg("! [SlangCompiler] Unknown target, defaulting to DXBC");
        return SLANG_DXBC;
    }
}

SlangStage SlangCompiler::GetSlangStage(Stage stage) const
{
    switch (stage)
    {
    case Stage::Vertex:
        return SLANG_STAGE_VERTEX;
    case Stage::Pixel:
        return SLANG_STAGE_PIXEL;
    case Stage::Compute:
        return SLANG_STAGE_COMPUTE;
    case Stage::Geometry:
        return SLANG_STAGE_GEOMETRY;
    case Stage::Hull:
        return SLANG_STAGE_HULL;
    case Stage::Domain:
        return SLANG_STAGE_DOMAIN;
    default:
        Msg("! [SlangCompiler] Unknown stage, defaulting to vertex");
        return SLANG_STAGE_VERTEX;
    }
}

const char* SlangCompiler::GetTargetName(Target target)
{
    switch (target)
    {
    case Target::DXBC:  return "DXBC (DX11)";
    case Target::DXIL:  return "DXIL (DX12)";
    case Target::SPIRV: return "SPIR-V (Vulkan)";
    case Target::GLSL:  return "GLSL (OpenGL)";
    default:            return "Unknown";
    }
}

const char* SlangCompiler::GetStageName(Stage stage)
{
    switch (stage)
    {
    case Stage::Vertex:   return "Vertex";
    case Stage::Pixel:    return "Pixel";
    case Stage::Compute:  return "Compute";
    case Stage::Geometry: return "Geometry";
    case Stage::Hull:     return "Hull";
    case Stage::Domain:   return "Domain";
    default:              return "Unknown";
    }
}

} // namespace render
