// Engine.cpp: implementation of the CEngine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Engine.h"

#include "XR_IOConsole.h"
#include "xr_ioc_cmd.h"
#if defined(XR_PLATFORM_ANDROID)
#include "Include/xrRender/xrRender.h"
#include "AndroidRendererChoice.h"
#endif

struct SoundProcessor final : public pureFrame
{
    void OnFrame() override
    {
        GEnv.Sound->update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop, Device.vCameraRight);
    }
} g_sound_processor;

struct SoundRenderer final : public pureFrame
{
    void OnFrame() override
    {
        GEnv.Sound->render();
    }
} g_sound_renderer;

CEngine Engine;

CEngine::CEngine() {}
CEngine::~CEngine() {}

void CheckAndSetupRenderer()
{
    if (GEnv.isDedicatedServer)
    {
        Console->Execute("renderer renderer_r1");
        return;
    }

#if defined(XR_PLATFORM_ANDROID)
    const bool wants_vulkan = xray::render::android_renderer_needs_vulkan_probe(Core.Params);
    const auto choice = xray::render::choose_android_renderer(Core.Params, wants_vulkan && xray::render::vulkan::GetRendererModule()->CheckGameRequirements());
    R_ASSERT2(choice != xray::render::AndroidRendererChoice::VulkanUnavailable,
              "Explicit Vulkan renderer unavailable: inspect the Vulkan probe and shader resource diagnostics");
    if (choice == xray::render::AndroidRendererChoice::Vulkan)
        Console->Execute("renderer renderer_vulkan");
    else if (choice == xray::render::AndroidRendererChoice::GLES)
        Console->Execute("renderer renderer_gles");
    else
#endif
    if (strstr(Core.Params, "-rgl"))
        Console->Execute("renderer renderer_rgl");
    else if (strstr(Core.Params, "-r4"))
        Console->Execute("renderer renderer_r4");
    else if (strstr(Core.Params, "-r3"))
        Console->Execute("renderer renderer_r3");
    else if (strstr(Core.Params, "-r2.5"))
        Console->Execute("renderer renderer_r2.5");
    else if (strstr(Core.Params, "-r2a"))
        Console->Execute("renderer renderer_r2a");
    else if (strstr(Core.Params, "-r2"))
        Console->Execute("renderer renderer_r2");
    else if (strstr(Core.Params, "-r1"))
        Console->Execute("renderer renderer_r1");
    else
    {
        CCC_LoadCFG_custom cmd("renderer ");
        cmd.Execute(Console->ConfigFile);
        renderer_allow_override = true;
    }
}

extern void msCreate(pcstr name);

void CEngine::Initialize(GameModule* game, const std::array<RendererModule*, 3>& modules)
{
    ZoneScoped;
#ifdef DEBUG
    msCreate("game");
#endif

    eQuit = Event.Handler_Attach("KERNEL:quit", this);

    Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1000);

    Device.seqFrame.Add(&g_sound_processor, REG_PRIORITY_NORMAL - 1000); // Place it after Level update
    Device.seqFrameMT.Add(&g_sound_renderer);

    External.CreateRendererList(modules);
    CheckAndSetupRenderer();

    External.Initialize(game);
    Sheduler.Initialize();
}

void CEngine::Destroy()
{
    ZoneScoped;

    Sheduler.Destroy();
    External.Destroy();
    Event._destroy();

    Event.Handler_Detach(eQuit, this);

    Device.seqFrameMT.Remove(&g_sound_processor);
    Device.seqFrame.Remove(&g_sound_processor);
    Device.seqFrame.Remove(this);
}

void CEngine::OnEvent(EVENT E, u64 P1, u64 P2)
{
    if (E == eQuit)
    {
#if defined(XR_PLATFORM_ANDROID)
        Msg("[android-lifecycle] KERNEL:quit handled frame=%u", Device.dwFrame);
#endif
        if (pInput != nullptr)
            pInput->GrabInput(false);

        SDL_Event quit = { SDL_QUIT };
        SDL_PushEvent(&quit);
    }
}

void CEngine::OnFrame()
{
    Event.OnFrame();
}
