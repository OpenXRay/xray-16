
// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EngineAPI.h"
#include "XR_IOConsole.h"

#include "xrCore/ModuleLookup.hpp"
#include "xrCore/xr_token.h"

#include "xrScriptEngine/ScriptExporter.hpp"

extern xr_vector<xr_token> VidQualityToken;

constexpr pcstr GET_RENDERER_MODULE_FUNC = "GetRendererModule";

constexpr pcstr r1_library     = "xrRender_R1";
constexpr pcstr r2_library     = "xrRender_R2";
constexpr pcstr gl_library     = "xrRender_GL";

constexpr pcstr RENDER_LIBRARIES[] =
{
#if defined(XR_PLATFORM_WINDOWS)
    r1_library,
    r2_library,
    "xrRender_R4",
#endif
    gl_library
};

static bool r2_available = false;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace
{
void __cdecl dummy(void) {}
IFactoryObject* __cdecl dummy(CLASS_ID) { return nullptr; }
template<typename T>
void __cdecl dummy(T* p) { R_ASSERT2(p == nullptr, "Attempting to release an object that shouldn't be allocated"); }
};

CEngineAPI::CEngineAPI()
{
    hGame = nullptr;
    hTuner = nullptr;
    pCreate = dummy;
    pDestroy = dummy;
    tune_enabled = false;
    tune_pause = dummy;
    tune_resume = dummy;
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

    //////////////////////////////////////////////////////////////////////////
    // vTune
    tune_enabled = false;
    if (strstr(Core.Params, "-tune"))
    {
        hTuner = XRay::LoadModule("vTuneAPI");
        tune_pause = (VTPause*)hTuner->GetProcAddress("VTPause");
        tune_resume = (VTResume*)hTuner->GetProcAddress("VTResume");

        if (!tune_pause || !tune_resume)
        {
            Log("Can't initialize Intel vTune");
            tune_pause = dummy;
            tune_resume = dummy;
            return;
        }

        tune_enabled = true;
    }

    CloseUnusedLibraries();
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
	
    hTuner = nullptr;
	
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
        R_ASSERT2(loadLibrary(r1_library), "Dedicated server needs xrRender_R1 to work");
#else
        R_ASSERT2(loadLibrary(gl_library), "Dedicated server needs xrRender_GL to work");
#endif
    }
    else
    {
        for (pcstr library : RENDER_LIBRARIES)
        {
            loadLibrary(library);
        }

        const auto it = std::find_if(renderers.begin(), renderers.end(), [](const RendererDesc& desc)
        {
            return desc.libraryName == r2_library;
        });
        if (it != renderers.end())
            r2_available = true;
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
    
    if (GEnv.isDedicatedServer)
    {
#if defined(XR_PLATFORM_WINDOWS)
        static cpcstr renderForDedicated = r1_library;
#else
        static cpcstr renderForDedicated = gl_library;
#endif
        const auto it = std::find_if(renderers.begin(), renderers.end(), [](RendererDesc& desc)
        {
            return desc.libraryName == renderForDedicated;
        });
        obtainModes(it->module);
    }
    else
    {
        for (RendererDesc& desc : renderers)
        {
            obtainModes(desc.module);
        }
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

bool is_r2_available()
{
    return r2_available;
}

SCRIPT_EXPORT(CheckRendererSupport, (),
{
    using namespace luabind;
    module(luaState)
    [
        def("xrRender_test_r2_hw", &is_r2_available)
    ];
});
