#include "stdafx.h"

#include "Render.h"
#include "xr_input.h"

void CRenderDevice::Destroy()
{
    if (!b_is_Ready)
        return;

    ZoneScoped;
    Log("Destroying Render...");
    b_is_Ready = false;
#if defined(XR_PLATFORM_ANDROID)
    Msg("[android-teardown] statistics begin");
#endif
    Statistic->OnDeviceDestroy();
#if defined(XR_PLATFORM_ANDROID)
    Msg("[android-teardown] renderer OnDeviceDestroy begin");
#endif
    GEnv.Render->OnDeviceDestroy(false);
#if defined(XR_PLATFORM_ANDROID)
    Msg("[android-teardown] renderer OnDeviceDestroy end");
#endif
    Memory.mem_compact();
#if defined(XR_PLATFORM_ANDROID)
    Msg("[android-teardown] renderer Destroy begin");
#endif
    GEnv.Render->Destroy();
#if defined(XR_PLATFORM_ANDROID)
    Msg("[android-teardown] renderer Destroy end");
#endif
    seqRender.Clear();
    seqAppActivate.Clear();
    seqAppDeactivate.Clear();
    seqAppEnd.Clear();
    seqFrame.Clear();
    seqFrameMT.Clear();
    seqDeviceReset.Clear();
    seqParallel.clear();
    xr_delete(Statistic);

    SDL_DestroyWindow(m_sdlWnd);
#if defined(XR_PLATFORM_ANDROID)
    Msg("[android-teardown] device destroy end");
#endif
}

void CRenderDevice::Reset(bool precache /*= true*/)
{
    ZoneScoped;

    const auto dwWidth_before = dwWidth;
    const auto dwHeight_before = dwHeight;
    pInput->GrabInput(false);

    const auto tm_start = TimerAsync();

    m_imgui_render->OnDeviceResetBegin();

    UpdateWindowProps();
    GEnv.Render->Reset(m_sdlWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);

    m_imgui_render->OnDeviceResetEnd();

#if defined(XR_PLATFORM_ANDROID)
    if (!strstr(Core.Params, "-renderer-vulkan"))
#endif
        UpdateWindowProps(); // The Vulkan reset already supplied drawable dimensions.

    SetupStates();

    if (precache)
        PreCache(20, false);

    const auto tm_end = TimerAsync();
    Msg("*** RESET [%d ms]", tm_end - tm_start);

    // TODO: Remove this! It may hide crash
    Memory.mem_compact();

    seqDeviceReset.Process();
    if (dwWidth_before != dwWidth || dwHeight_before != dwHeight)
        seqUIReset.Process();

    if (!GEnv.isDedicatedServer)
        pInput->GrabInput(true);
}
