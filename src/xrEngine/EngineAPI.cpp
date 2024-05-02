
// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"
#include "XR_IOConsole.h"

#include "xrCore/ModuleLookup.hpp"
#include "xrCore/xr_token.h"
#include "xrCore/Threading/ParallelForEach.hpp"

#include "xrScriptEngine/ScriptExporter.hpp"

extern xr_vector<xr_token> VidQualityToken;

constexpr pcstr GET_RENDERER_MODULE_FUNC = "GetRendererModule";

constexpr pcstr r4_library     = "xrRender_R4";
constexpr pcstr gl_library     = "xrRender_GL";

constexpr pcstr RENDER_LIBRARIES[] =
{
    r4_library,
    gl_library
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEngineAPI::CEngineAPI()
{
    pCreate = [](CLASS_ID) -> IFactoryObject*
    {
        return nullptr;
    };

    pDestroy = [](IFactoryObject* p)
    {
        R_ASSERT2(p == nullptr, "Attempting to release an object that shouldn't be allocated");
    };
}

CEngineAPI::~CEngineAPI()
{
    VidQualityToken.clear();
}

bool is_enough_address_space_available()
{
#if defined(XR_PLATFORM_WINDOWS)
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    return (*(u32*)&system_info.lpMaximumApplicationAddress) > 0x90000000;
#else
    return true; // In Linux allocated memory limited only by pointer size
#endif
}

void CEngineAPI::SelectRenderer()
{
    ZoneScoped;

    // User has some renderer selected
    // Find it and check if we can use it
    pcstr selected_mode = Console->GetString("renderer");
    const auto it = renderModes.find(selected_mode);
    if (it != renderModes.end())
    {
        if (it->second->CheckGameRequirements())
            selectedRenderer = it->second;
    }

    // Renderer is either fully unsupported (hardware)
    // or we don't comply with it requirements (e.g. shaders missing)
    if (!selectedRenderer)
    {
        // Select any suitable
        for (const auto& [mode, renderer] : renderModes)
        {
            if (renderer->CheckGameRequirements())
            {
                selectedRenderer = renderer;
                selected_mode = mode.c_str();
                string64 buf;
                xr_sprintf(buf, "renderer %s", selected_mode);
                Console->Execute(buf);
                break;
            }
        }
    }

    // Ask current renderer to setup GEnv
    R_ASSERT2(selectedRenderer, "Can't setup renderer");
    selectedRenderer->SetupEnv(selected_mode);

    Log("Selected renderer:", selected_mode);
}

void CEngineAPI::Initialize(void)
{
    ZoneScoped;

    SelectRenderer();

    hGame = XRay::LoadModule("xrGame");
    if (!CanSkipGameModuleLoading())
    {
        R_ASSERT2(hGame->IsLoaded(), "! Game DLL raised exception during loading or there is no game DLL at all");

        pCreate = (Factory_Create*)hGame->GetProcAddress("xrFactory_Create");
        R_ASSERT(pCreate);

        pDestroy = (Factory_Destroy*)hGame->GetProcAddress("xrFactory_Destroy");
        R_ASSERT(pDestroy);

        pInitializeGame = (InitializeGameLibraryProc)hGame->GetProcAddress("initialize_library");
        R_ASSERT(pInitializeGame);

        pFinalizeGame = (FinalizeGameLibraryProc)hGame->GetProcAddress("finalize_library");
        R_ASSERT(pFinalizeGame);

        pInitializeGame();
    }

    CloseUnusedLibraries();
}

void CEngineAPI::Destroy(void)
{
    ZoneScoped;
    if (pFinalizeGame)
        pFinalizeGame();

    pInitializeGame = nullptr;
    pFinalizeGame = nullptr;
    pCreate = nullptr;
    pDestroy = nullptr;

    hGame = nullptr;

    renderers.clear();
    XRC.r_clear_compact();
}

void CEngineAPI::CloseUnusedLibraries()
{
    ZoneScoped;
    for (RendererDesc& desc : renderers)
    {
        if (desc.module != selectedRenderer)
            desc.handle = nullptr;
    }
}

void CEngineAPI::CreateRendererList()
{
    if (!VidQualityToken.empty())
        return;

    ZoneScoped;

    const auto loadLibrary = [&](pcstr library) -> bool
    {
        auto handle = XRay::LoadModule(library);
        if (!handle->IsLoaded())
            return false;

        const auto getModule = (GetRendererModule)handle->GetProcAddress(GET_RENDERER_MODULE_FUNC);
        RendererModule* module = getModule ? getModule() : nullptr;
        if (!module)
            return false;

        renderers.emplace_back(RendererDesc({ library, std::move(handle), module }));
        return true;
    };

    if (GEnv.isDedicatedServer)
    {
#if defined(XR_PLATFORM_WINDOWS)
        R_ASSERT2(loadLibrary(r4_library), "Dedicated server needs xrRender_R1 to work");
#else
        R_ASSERT2(loadLibrary(gl_library), "Dedicated server needs xrRender_GL to work");
#endif
    }
    else
    {
        for (cpcstr library : RENDER_LIBRARIES)
        {
            loadLibrary(library);
        }
    }

    std::mutex mutex;

    int modeIndex{};
    const auto obtainModes = [&](const RendererDesc& desc)
    {
        if (!desc.module)
            return;

        const auto& modes = desc.module->ObtainSupportedModes();
        for (pcstr mode : modes)
        {
            const auto it = std::find_if(renderModes.begin(), renderModes.end(), [&](auto& pair)
            {
                return 0 == xr_strcmp(mode, pair.first.c_str());
            });
            string256 temp;
            if (it != renderModes.end())
            {
                xr_sprintf(temp, "%s__dup%d", mode, modeIndex);
                mode = temp;
            }
            shared_str copiedMode = mode;
            renderModes[copiedMode] = desc.module;
            std::lock_guard guard{ mutex };
            VidQualityToken.emplace_back(copiedMode.c_str(), modeIndex++); // It's important to have postfix increment!
        }
    };

    xr_parallel_for_each(renderers, obtainModes);

    auto& modes = VidQualityToken;
    Msg("Available render modes[%d]:", modes.size());
    for (const auto& mode : modes)
    {
        if (mode.name)
            Log(mode.name);
    }
    modes.emplace_back(nullptr, -1);
}

SCRIPT_EXPORT(CheckRendererSupport, (),
{
    using namespace luabind;
    module(luaState)
    [
        def("xrRender_test_r2_hw", +[](){ return true; })
    ];
});
