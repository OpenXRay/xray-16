#include "stdafx.h"
#include "Include/xrRender/DrawUtils.h"
#include "Render.h"
#include "IGame_Persistent.h"
#include "XR_IOConsole.h"
#include "xr_input.h"

void CRenderDevice::Destroy()
{
    if (!b_is_Ready)
        return;
    Log("Destroying Direct3D...");
    pInput->ClipCursor(false);
    GEnv.Render->ValidateHW();
    GEnv.DU->OnDeviceDestroy();
    b_is_Ready = false;
    Statistic->OnDeviceDestroy();
    GEnv.Render->destroy();
    GEnv.Render->OnDeviceDestroy(false);
    Memory.mem_compact();
    GEnv.Render->DestroyHW();
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
}

#include "IGame_Level.h"
#include "CustomHUD.h"
extern BOOL bNeed_re_create_env;

void CRenderDevice::Reset(bool precache)
{
    const auto dwWidth_before = dwWidth;
    const auto dwHeight_before = dwHeight;
    pInput->ClipCursor(false);

    const auto tm_start = TimerAsync();

    GEnv.Render->Reset(m_hWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);
    GetWindowRect(m_hWnd, &m_rcWindowBounds);
    GetClientRect(m_hWnd, &m_rcWindowClient);

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
        seqResolutionChanged.Process();

    if (!GEnv.isDedicatedServer)
        pInput->ClipCursor(true);
}
