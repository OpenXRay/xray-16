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
    ZoneScoped;

    const auto dwWidth_before = dwWidth;
    const auto dwHeight_before = dwHeight;
    pInput->GrabInput(false);

    const auto tm_start = TimerAsync();

    m_imgui_render->OnDeviceResetBegin();

    UpdateWindowProps();
    GEnv.Render->Reset(m_sdlWnd, dwWidth, dwHeight, fWidth_2, fHeight_2);

    m_imgui_render->OnDeviceResetEnd();

    // Update window props again for DX9 renderer
    if (GEnv.Render->GetBackendAPI() == IRender::BackendAPI::D3D9) // XXX: I don't remember why this hack is needed, thus, I'm not sure if it is needed at all
        UpdateWindowProps(); // hack

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
