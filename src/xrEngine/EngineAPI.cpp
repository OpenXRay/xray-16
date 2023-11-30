
// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"
#include "XR_IOConsole.h"

#include "xrCore/ModuleLookup.hpp"
#include "xrCore/xr_token.h"

#include "xrScriptEngine/ScriptExporter.hpp"

#ifdef XRAY_STATIC_BUILD
extern "C"
{
    XR_EXPORT RendererModule* GetRendererModule();
    
    XR_EXPORT IFactoryObject* __cdecl xrFactory_Create(CLASS_ID clsid);
    XR_EXPORT void __cdecl xrFactory_Destroy(IFactoryObject* O);
    XR_EXPORT void initialize_library();
    XR_EXPORT void finalize_library();
}
#endif

extern xr_vector<xr_token> VidQualityToken;

constexpr pcstr GET_RENDERER_MODULE_FUNC = "GetRendererModule";

constexpr pcstr r1_library     = "xrRender_R1";
constexpr pcstr gl_library     = "xrRender_GL";

constexpr pcstr RENDER_LIBRARIES[] =
{
#if defined(XR_PLATFORM_WINDOWS)
    r1_library,
    "xrRender_R2",
    "xrRender_R4",
#endif
    gl_library
};

static bool r2_available = false;

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

pcstr CEngineAPI::SelectRenderer()
{
    cpcstr selected_mode = Console->GetString("renderer");
    const auto it = renderModes.find(selected_mode);
    if (it != renderModes.end())
    {
        selectedRenderer = it->second;
    }
    return selected_mode;
}

void CEngineAPI::InitializeRenderers()
{
    pcstr selected_mode = SelectRenderer();

    if (selectedRenderer == nullptr
        && VidQualityToken[0].id != -1)
    {
        // if engine failed to load renderer
        // but there is at least one available
        // then try again
        string64 buf;
        xr_sprintf(buf, "renderer %s", VidQualityToken[0].name);
        Console->Execute(buf);

        // Second attempt
        selected_mode = SelectRenderer();
    }

    // Ask current renderer to setup GEnv
    R_ASSERT2(selectedRenderer, "Can't setup renderer");
    selectedRenderer->SetupEnv(selected_mode);

    Log("Selected renderer:", selected_mode);
}

void CEngineAPI::Initialize(void)
{
    InitializeRenderers();

#ifdef XRAY_STATIC_BUILD
    pCreate = xrFactory_Create;
    pDestroy = xrFactory_Destroy;
    pInitializeGame = initialize_library;
    pFinalizeGame = finalize_library;

    pInitializeGame();
#else
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
#endif
}

void CEngineAPI::Destroy(void)
{
    if (pFinalizeGame)
        pFinalizeGame();

    pInitializeGame = nullptr;
    pFinalizeGame = nullptr;
    pCreate = nullptr;
    pDestroy = nullptr;

    hGame = nullptr;

    renderers.clear();
    Engine.Event._destroy();
    XRC.r_clear_compact();
}

void CEngineAPI::CloseUnusedLibraries()
{
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

    const auto loadLibrary = [&](pcstr library) -> bool
    {
#ifdef XRAY_STATIC_BUILD
        const auto handle = nullptr;
        const auto getModule = ::GetRendererModule;
#else
        auto handle = XRay::LoadModule(library);
        if (!handle->IsLoaded())
            return false;

        const auto getModule = (GetRendererModule)handle->GetProcAddress(GET_RENDERER_MODULE_FUNC);
#endif
        RendererModule* module = getModule ? getModule() : nullptr;
        if (!module)
            return false;

        renderers.emplace_back(RendererDesc({ library, std::move(handle), module }));
        return true;
    };

    if (GEnv.isDedicatedServer)
    {
#if defined(XR_PLATFORM_WINDOWS)
        R_ASSERT2(loadLibrary(r1_library), "Dedicated server needs xrRender_R1 to work");
#else
        R_ASSERT2(loadLibrary(gl_library), "Dedicated server needs xrRender_GL to work");
#endif
    }
    else
    {
        for (pcstr library : RENDER_LIBRARIES)
        {
            if (loadLibrary(library) && library != r1_library)
                r2_available = true;
        }
    }

    int modeIndex{};
    const auto obtainModes = [&](RendererModule* module)
    {
        if (module)
        {
            const auto& modes = module->ObtainSupportedModes();
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
                renderModes[copiedMode] = module;
                VidQualityToken.emplace_back(copiedMode.c_str(), modeIndex++); // It's important to have postfix increment!
            }
        }
    };

    for (RendererDesc& desc : renderers)
    {
        obtainModes(desc.module);
    }

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
        def("xrRender_test_r2_hw", +[](){ return r2_available; })
    ];
});
