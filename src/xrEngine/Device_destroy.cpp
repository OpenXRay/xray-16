#include "stdafx.h"
#include "Include/xrRender/DrawUtils.h"
#include "Render.h"
#include "IGame_Persistent.h"
#include "XR_IOConsole.h"
#include "xr_input.h"
#include "SDL.h"
#include "SDL_syswm.h"

void CRenderDevice::Destroy()
{
    if (!b_is_Ready)
        return;
    Log("Destroying Render...");
    b_is_Ready = false;
    Statistic->OnDeviceDestroy();
    GEnv.Render->OnDeviceDestroy(false);
    Memory.mem_compact();
    GEnv.Render->Destroy();
    seqRender.Clear();
    seqAppActivate.Clear();
    seqAppDeactivate.Clear();
    seqAppStart.Clear();
    seqAppEnd.Clear();
    seqFrame.Clear();
    seqFrameMT.Clear();
    seqDeviceReset.Clear();
    seqParallel.clear();
    xr_delete(Statistic);

    SDL_DestroyWindow(m_sdlWnd);
}

void CRenderDevice::Reset(bool precache /*= true*/)
{
    ResetInternal(precache);
}

#include "IGame_Level.h"
#include "CustomHUD.h"
extern bool bNeed_re_create_env;

void CRenderDevice::ResetInternal(bool precache)
{
    const auto dwWidth_before = dwWidth;
    const auto dwHeight_before = dwHeight;
    pInput->GrabInput(false);

    const auto tm_start = TimerAsync();

    UpdateWindowProps(!psDeviceFlags.is(rsFullscreen));
    GEnv.Render->Reset(m_sdlWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);

    // Update window props again for DX9 renderer
    const bool isDX9Renderer = GEnv.Render->get_dx_level() == 0x00090000;
    if (isDX9Renderer)
        UpdateWindowProps(!psDeviceFlags.is(rsFullscreen)); // hack

    if (g_pGamePersistent)
        g_pGamePersistent->Environment().bNeed_re_create_env = true;
    _SetupStates();

    if (precache)
        PreCache(20, true, false);

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
