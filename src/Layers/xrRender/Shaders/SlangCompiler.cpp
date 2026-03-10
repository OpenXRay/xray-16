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
    // column_major: HLSL interprets row-major C++ bytes (Fmatrix) as columns,
    // naturally transposing so mul(M, v) gives the correct row-vector result (v * M).
    slang::SessionDesc sessionDesc = {};
    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
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
        profile = "sm_6_6"; // Shader Model 6.6 for DX12 (ResourceDescriptorHeap bindless)
        break;
    case Target::SPIRV:
        profile = "sm_6_6"; // SPIR-V 1.3+ required by Slang direct emit
        break;
    default:
        profile = "sm_5_0";
        break;
    }

    targetDesc.profile = m_globalSession->findProfile(profile);

    slang::CompilerOptionEntry targetOptions[1];
    u32 targetOptionCount = 0;

    if (target == Target::SPIRV)
    {
        targetOptions[targetOptionCount++] = {slang::CompilerOptionName::EmitSpirvMethod,
            {slang::CompilerOptionValueKind::Int, SLANG_EMIT_SPIRV_DIRECTLY, 0}};

        result.vkShifts.unorderedAccess = 384;
        result.vkShifts.sampler = 128;
        result.vkShifts.shaderResource = 0;
        result.vkShifts.constantBuffer = 256;
    }

    targetDesc.compilerOptionEntries = targetOptions;
    targetDesc.compilerOptionEntryCount = targetOptionCount;

    sessionDesc.targets = &targetDesc;
    sessionDesc.targetCount = 1;

    if (!slangDefines.empty())
    {
        sessionDesc.preprocessorMacros = slangDefines.data();
        sessionDesc.preprocessorMacroCount = slangDefines.size();
    }

    slang::CompilerOptionEntry sessionOptions[8];
    u32 sessionOptionCount = 0;

    sessionOptions[sessionOptionCount].name = slang::CompilerOptionName::NoMangle;
    sessionOptions[sessionOptionCount].value = {slang::CompilerOptionValueKind::Int, 1, 0};
    sessionOptionCount++;

    sessionOptions[sessionOptionCount].name = slang::CompilerOptionName::PreserveParameters;
    sessionOptions[sessionOptionCount].value = {slang::CompilerOptionValueKind::Int, 0, 0};
    sessionOptionCount++;

    sessionOptions[sessionOptionCount].name = slang::CompilerOptionName::DebugInformation;
#ifdef DEBUG
    sessionOptions[sessionOptionCount].value = {slang::CompilerOptionValueKind::Int, SLANG_DEBUG_INFO_LEVEL_MAXIMAL, 0};
#else
    sessionOptions[sessionOptionCount].value = {slang::CompilerOptionValueKind::Int, SLANG_DEBUG_INFO_LEVEL_NONE, 0};
#endif
    sessionOptionCount++;

    if (target == Target::SPIRV)
    {
        sessionOptions[sessionOptionCount++] = {slang::CompilerOptionName::VulkanBindShiftAll,
            {slang::CompilerOptionValueKind::Int, 0, 384}};
        sessionOptions[sessionOptionCount++] = {slang::CompilerOptionName::VulkanBindShiftAll,
            {slang::CompilerOptionValueKind::Int, 1, 128}};
        sessionOptions[sessionOptionCount++] = {slang::CompilerOptionName::VulkanBindShiftAll,
            {slang::CompilerOptionValueKind::Int, 2, 0}};
        sessionOptions[sessionOptionCount++] = {slang::CompilerOptionName::VulkanBindShiftAll,
            {slang::CompilerOptionValueKind::Int, 3, 256}};
    }

    sessionDesc.compilerOptionEntries = sessionOptions;
    sessionDesc.compilerOptionEntryCount = sessionOptionCount;

    // Use VFS adapter for file system operations (includes from VFS)
    sessionDesc.fileSystem = m_vfsAdapter.get();

    SlangResult slangResult = m_globalSession->createSession(sessionDesc, result.session.writeRef());
    if (SLANG_FAILED(slangResult) || !result.session)
    {
        result.errorMessage = "Failed to create Slang session";
        Msg("! [SlangCompiler] %s", result.errorMessage.c_str());
        return result;
    }
    auto& session = result.session;

    Slang::ComPtr<slang::IBlob> diagnosticBlob;

    Slang::ComPtr<slang::IModule> module;
    {
        module = session->loadModuleFromSourceString(
            sourcePath, sourcePath, source, diagnosticBlob.writeRef());
        if (diagnosticBlob)
        {
            auto* msg = static_cast<const char*>(diagnosticBlob->getBufferPointer());
            if (msg && msg[0])
                result.warningMessage = msg;
            diagnosticBlob = nullptr;
        }
        if (!module)
        {
            result.errorMessage = result.warningMessage.empty()
                ? "Failed to load module" : result.warningMessage;
            Msg("! [SlangCompiler] %s: %s", sourcePath, result.errorMessage.c_str());
            return result;
        }
    }

    Slang::ComPtr<slang::IEntryPoint> entryPointComp;
    {
        SlangStage slangStage = GetSlangStage(stage);
        slangResult = module->findAndCheckEntryPoint(
            entryPoint, slangStage, entryPointComp.writeRef(), diagnosticBlob.writeRef());
        if (diagnosticBlob)
        {
            auto* msg = static_cast<const char*>(diagnosticBlob->getBufferPointer());
            if (msg && msg[0] && result.warningMessage.empty())
                result.warningMessage = msg;
            diagnosticBlob = nullptr;
        }
        if (SLANG_FAILED(slangResult) || !entryPointComp)
        {
            result.errorMessage = "Failed to find entry point: " + xr_string(entryPoint);
            Msg("! [SlangCompiler] %s", result.errorMessage.c_str());
            return result;
        }
    }

    Slang::ComPtr<slang::IComponentType> compositeProgram;
    {
        slang::IComponentType* components[] = { module, entryPointComp.get() };
        slangResult = session->createCompositeComponentType(
            components, 2, compositeProgram.writeRef(), diagnosticBlob.writeRef());
        if (SLANG_FAILED(slangResult) || !compositeProgram)
        {
            result.errorMessage = "Failed to create composite program";
            Msg("! [SlangCompiler] %s", result.errorMessage.c_str());
            return result;
        }
    }

    Slang::ComPtr<slang::IComponentType> linkedProgram;
    {
        slangResult = compositeProgram->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());
        if (diagnosticBlob)
        {
            auto* msg = static_cast<const char*>(diagnosticBlob->getBufferPointer());
            if (msg && msg[0])
            {
                result.errorMessage = msg;
                Msg("! [SlangCompiler] Link errors for %s: %s", sourcePath, msg);
            }
            diagnosticBlob = nullptr;
        }
        if (SLANG_FAILED(slangResult) || !linkedProgram)
        {
            if (result.errorMessage.empty())
                result.errorMessage = "Failed to link program";
            Msg("! [SlangCompiler] Compilation failed for %s (entry: %s, stage: %s, target: %s)",
                sourcePath, entryPoint, GetStageName(stage), GetTargetName(target));
            return result;
        }
    }

    Slang::ComPtr<slang::IBlob> codeBlob;
    slangResult = linkedProgram->getEntryPointCode(0, 0, codeBlob.writeRef(), diagnosticBlob.writeRef());
    if (SLANG_FAILED(slangResult) || !codeBlob)
    {
        result.errorMessage = "Failed to retrieve compiled bytecode";
        Msg("! [SlangCompiler] %s", result.errorMessage.c_str());
        return result;
    }

    const u8* bytecodeData = static_cast<const u8*>(codeBlob->getBufferPointer());
    size_t bytecodeSize = codeBlob->getBufferSize();
    result.bytecode.resize(bytecodeSize);
    std::memcpy(result.bytecode.data(), bytecodeData, bytecodeSize);
    result.success = true;

    auto* compositeLayout = compositeProgram->getLayout();
    result.reflection = compositeLayout;

    compositeProgram->addRef();
    result.slangProgram = compositeProgram.get();

    linkedProgram->addRef();
    result.linkedProgram = linkedProgram.get();

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
    case Stage::Amplification:
        return SLANG_STAGE_AMPLIFICATION;
    case Stage::Mesh:
        return SLANG_STAGE_MESH;
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
    case Stage::Amplification: return "Amplification";
    case Stage::Mesh:          return "Mesh";
    default:              return "Unknown";
    }
}

} // namespace render
