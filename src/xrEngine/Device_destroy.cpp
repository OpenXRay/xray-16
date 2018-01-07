#include "stdafx.h"
#include "Include/xrRender/DrawUtils.h"
#include "Render.h"
#include "IGame_Persistent.h"
#include "xr_IOConsole.h"

void CRenderDevice::Destroy()
{
    if (!b_is_Ready)
        return;
    Log("Destroying Direct3D...");
    ShowCursor(true);
    GEnv.Render->ValidateHW();
    GEnv.DU->OnDeviceDestroy();
    b_is_Ready = false;
    Statistic->OnDeviceDestroy();
    GEnv.Render->destroy();
    GEnv.Render->OnDeviceDestroy(false);
    Memory.mem_compact();
    GEnv.Render->DestroyHW();
    seqRender.R.clear();
    seqAppActivate.R.clear();
    seqAppDeactivate.R.clear();
    seqAppStart.R.clear();
    seqAppEnd.R.clear();
    seqFrame.R.clear();
    seqFrameMT.R.clear();
    seqDeviceReset.R.clear();
    seqParallel.clear();
    xr_delete(Statistic);
}

#include "IGame_Level.h"
#include "CustomHUD.h"
extern BOOL bNeed_re_create_env;

void CRenderDevice::Reset(bool precache)
{
    u32 dwWidth_before = dwWidth;
    u32 dwHeight_before = dwHeight;
    ShowCursor(true);
    u32 tm_start = TimerAsync();
    GEnv.Render->Reset(m_hWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);
    if (g_pGamePersistent)
        g_pGamePersistent->Environment().bNeed_re_create_env = true;
    _SetupStates();
    if (precache)
        PreCache(20, true, false);
    u32 tm_end = TimerAsync();
    Msg("*** RESET [%d ms]", tm_end - tm_start);
    // TODO: Remove this! It may hide crash
    Memory.mem_compact();

    if (!GEnv.isDedicatedServer)
        ShowCursor(false);

    seqDeviceReset.Process(rp_DeviceReset);
    if (dwWidth_before != dwWidth || dwHeight_before != dwHeight)
        seqResolutionChanged.Process(rp_ScreenResolutionChanged);
}
