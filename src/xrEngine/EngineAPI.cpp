
// EngineAPI.cpp: implementation of the CEngineAPI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "EngineAPI.h"
#include "XR_IOConsole.h"

#include "xrCore/xr_token.h"

#include "xrScriptEngine/ScriptExporter.hpp"

extern xr_vector<xr_token> VidQualityToken;

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

    // User has some renderer selected, find it
    pcstr selected_mode = Console->GetString("renderer");
    const auto it = std::find_if(renderModes.begin(), renderModes.end(), [selected_mode](const auto& pair)
    {
        return xr_strcmp(selected_mode, pair.first) == 0;
    });

    // Check if we can use it
    if (it != renderModes.end())
    {
        if (it->second->CheckGameRequirements())
            selectedRenderer = it->second;
    }

    // Renderer is either fully unsupported (hardware)
    // or we don't comply with it's requirements (e.g. shaders missing)
    if (!selectedRenderer)
    {
        // Select any suitable
        for (const auto& [mode, renderer] : renderModes)
        {
            if (renderer->CheckGameRequirements())
            {
                selectedRenderer = renderer;
                selected_mode = mode;
                string64 buf;
                xr_sprintf(buf, "renderer %s", selected_mode);
                Console->Execute(buf);
                break;
            }
        }
    }

    R_ASSERT2(selectedRenderer, "Can't setup renderer");

    // Ask current renderer to setup GEnv
    selectedRenderer->SetupEnv(selected_mode);

    Log("Selected renderer:", selected_mode);
}

void CEngineAPI::Initialize(GameModule* game)
{
    ZoneScoped;

    SelectRenderer();

    if (game)
    {
        gameModule = game;
        gameModule->initialize(pCreate, pDestroy);
        R_ASSERT(pCreate);
        R_ASSERT(pDestroy);
    }
}

void CEngineAPI::Destroy()
{
    ZoneScoped;

    if (gameModule)
        gameModule->finalize();

    selectedRenderer = nullptr;

    pCreate = nullptr;
    pDestroy = nullptr;

    XRC.r_clear_compact();
}

void CEngineAPI::CreateRendererList(const std::array<RendererModule*, 2>& modules)
{
    if (!VidQualityToken.empty())
        return;

    ZoneScoped;

    const auto loadRenderer = [this](RendererModule* module) -> bool
    {
        if (!module)
            return false;

        const auto& modes = module->ObtainSupportedModes(); // Performs HW tests, may take time
        if (modes.empty())
            return false;

        for (const auto [mode, modeIndex] : modes)
        {
            const auto it = renderModes.find(mode);
            if (it != renderModes.end())
            {
                VERIFY3(false, "Renderer mode duplicate. Skipping.", mode);
                continue;
            }
            renderModes[mode] = module;
            VidQualityToken.emplace_back(mode, modeIndex);
        }

        return true;
    };

    if (GEnv.isDedicatedServer)
    {
        R_ASSERT2(loadRenderer(modules[0]), "Dedicated server needs xrRender to work");
    }
    else
    {
        for (const auto& module : modules)
            loadRenderer(module);
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
        def("xrRender_test_r2_hw", +[](){ return true; })
    ];
});
