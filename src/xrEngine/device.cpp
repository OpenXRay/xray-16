#include "stdafx.h"
#include "xrCDB/Frustum.h"

#pragma warning(disable : 4995)
// mmsystem.h
#define MMNOSOUND
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
#define MMNOJOY
#include <mmsystem.h>
#pragma warning(default : 4995)

#include "x_ray.h"
#include "Render.h"

// must be defined before include of FS_impl.h
#define INCLUDE_FROM_ENGINE
#include "xrCore/FS_impl.h"

#ifdef INGAME_EDITOR
#include "Include/editor/ide.hpp"
#include "engine_impl.hpp"
#endif  // #ifdef INGAME_EDITOR

#include "xrSASH.h"
#include "IGame_Persistent.h"
#include "xrScriptEngine/ScriptExporter.hpp"

ENGINE_API CRenderDevice Device;
ENGINE_API CLoadScreenRenderer load_screen_renderer;

ENGINE_API BOOL g_bRendering = FALSE;

BOOL g_bLoaded = FALSE;
ref_light precache_light = 0;
int g_dwFPSlimit = 120;

BOOL CRenderDevice::Begin()
{
#ifndef DEDICATED_SERVER
    switch (GlobalEnv.Render->GetDeviceState())
    {
    case DeviceState::Normal: break;
    case DeviceState::Lost:
        // If the device was lost, do not render until we get it back
        Sleep(33);
        return FALSE;
        break;
    case DeviceState::NeedReset:
        // Check if the device is ready to be reset
        Reset();
        break;
    default: R_ASSERT(0);
    }
    GlobalEnv.Render->Begin();
    FPU::m24r();
    g_bRendering = TRUE;
#endif
    return TRUE;
}

void CRenderDevice::Clear()
{
    GlobalEnv.Render->Clear();
}

extern void CheckPrivilegySlowdown();

void CRenderDevice::End(void)
{
#ifndef DEDICATED_SERVER
#ifdef INGAME_EDITOR
    bool load_finished = false;
#endif  // #ifdef INGAME_EDITOR
    if (dwPrecacheFrame) {
        ::Sound->set_master_volume(0.f);
        dwPrecacheFrame--;
        if (!dwPrecacheFrame) {
#ifdef INGAME_EDITOR
            load_finished = true;
#endif
            GlobalEnv.Render->updateGamma();
            if (precache_light) {
                precache_light->set_active(false);
                precache_light.destroy();
            }
            ::Sound->set_master_volume(1.f);
            GlobalEnv.Render->ResourcesDestroyNecessaryTextures();
            Memory.mem_compact();
            Msg("* MEMORY USAGE: %d K", Memory.mem_usage() / 1024);
            Msg("* End of synchronization A[%d] R[%d]", b_is_Active, b_is_Ready);
#ifdef FIND_CHUNK_BENCHMARK_ENABLE
            g_find_chunk_counter.flush();
#endif
            CheckPrivilegySlowdown();
            if (g_pGamePersistent->GameType() == 1)  // haCk
            {
                WINDOWINFO wi;
                GetWindowInfo(m_hWnd, &wi);
                if (wi.dwWindowStatus != WS_ACTIVECAPTION) Pause(TRUE, TRUE, TRUE, "application start");
            }
        }
    }
    g_bRendering = FALSE;
    // end scene
    // Present goes here, so call OA Frame end.
    if (g_SASH.IsBenchmarkRunning()) g_SASH.DisplayFrame(Device.fTimeGlobal);
    GlobalEnv.Render->End();
#ifdef INGAME_EDITOR
    if (load_finished && m_editor) m_editor->on_load_finished();
#endif
#endif  // !DEDICATED_SERVER
}

void CRenderDevice::SecondaryThreadProc(void* context)
{
    auto& device = *static_cast<CRenderDevice*>(context);
    while (true)
    {
        device.syncProcessFrame.Wait();
        if (device.mt_bMustExit) {
            device.mt_bMustExit = FALSE;
            device.syncThreadExit.Set();
            return;
        }
        for (u32 pit = 0; pit < device.seqParallel.size(); pit++)
            device.seqParallel[pit]();
        device.seqParallel.clear_not_free();
        device.seqFrameMT.Process(rp_Frame);
        device.syncFrameDone.Set();
    }
}

#include "IGame_Level.h"
void CRenderDevice::PreCache(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input)
{
#ifdef DEDICATED_SERVER
    amount = 0;
#else
    if (GlobalEnv.Render->GetForceGPU_REF()) amount = 0;
#endif
    dwPrecacheFrame = dwPrecacheTotal = amount;
    if (amount && !precache_light && g_pGameLevel && g_loading_events.empty()) {
        precache_light = GlobalEnv.Render->light_create();
        precache_light->set_shadow(false);
        precache_light->set_position(vCameraPosition);
        precache_light->set_color(255, 255, 255);
        precache_light->set_range(5.0f);
        precache_light->set_active(true);
    }
    if (amount && b_draw_loadscreen && !load_screen_renderer.b_registered) {
        load_screen_renderer.start(b_wait_user_input);
    }
}

void CRenderDevice::CalcFrameStats()
{
    stats.RenderTotal.FrameEnd();
    do
    {
        // calc FPS & TPS
        if (fTimeDelta <= EPS_S) break;
        float fps = 1.f / fTimeDelta;
        // if (Engine.External.tune_enabled) vtune.update (fps);
        float fOne = 0.3f;
        float fInv = 1.0f - fOne;
        stats.fFPS = fInv * stats.fFPS + fOne * fps;
        if (stats.RenderTotal.result > EPS_S) {
            u32 renderedPolys = GlobalEnv.Render->GetCacheStatPolys();
            stats.fTPS = fInv * stats.fTPS + fOne * float(renderedPolys) / (stats.RenderTotal.result * 1000.f);
            stats.fRFPS = fInv * stats.fRFPS + fOne * 1000.f / stats.RenderTotal.result;
        }
    } while (false);
    stats.RenderTotal.FrameStart();
}

int g_svDedicateServerUpdateReate = 100;

ENGINE_API xr_list<LOADING_EVENT> g_loading_events;

void CRenderDevice::on_idle()
{
    if (!b_is_Ready) {
        Sleep(100);
        return;
    }

#ifndef DEDICATED_SERVER
    // FPS Lock
    static DWORD dwLastFrameTime = 0;
    int dwCurrentTime = timeGetTime();
    if (g_dwFPSlimit > 0)
        if ((dwCurrentTime - dwLastFrameTime) < (1000 / g_dwFPSlimit)) return;
    dwLastFrameTime = dwCurrentTime;
#endif

#ifdef DEDICATED_SERVER
    u32 FrameStartTime = TimerGlobal.GetElapsed_ms();
#endif
    if (psDeviceFlags.test(rsStatistic))
        g_bEnableStatGather = TRUE;  // XXX: why not use either rsStatistic or g_bEnableStatGather?
    else
        g_bEnableStatGather = FALSE;
    if (g_loading_events.size()) {
        if (g_loading_events.front()()) g_loading_events.pop_front();
        pApp->LoadDraw();
        return;
    }
    if (!Device.dwPrecacheFrame && !g_SASH.IsBenchmarkRunning() && g_bLoaded) g_SASH.StartBenchmark();
    FrameMove();
    // Precache
    if (dwPrecacheFrame) {
        float factor = float(dwPrecacheFrame) / float(dwPrecacheTotal);
        float angle = PI_MUL_2 * factor;
        vCameraDirection.set(_sin(angle), 0, _cos(angle));
        vCameraDirection.normalize();
        vCameraTop.set(0, 1, 0);
        vCameraRight.crossproduct(vCameraTop, vCameraDirection);
        mView.build_camera_dir(vCameraPosition, vCameraDirection, vCameraTop);
    }
    // Matrices
    mFullTransform.mul(mProject, mView);
    GlobalEnv.Render->SetCacheXform(mView, mProject);
    mInvFullTransform.invert(mFullTransform);
    vCameraPosition_saved = vCameraPosition;
    mFullTransform_saved = mFullTransform;
    mView_saved = mView;
    mProject_saved = mProject;
    syncProcessFrame.Set();  // allow secondary thread to do its job
    Sleep(0);

#ifndef DEDICATED_SERVER
    // all rendering is done here
    CStatTimer renderTotalReal;
    renderTotalReal.FrameStart();
    renderTotalReal.Begin();
    if (b_is_Active && Begin()) {
        seqRender.Process(rp_Render);
        CalcFrameStats();
        Statistic->Show();
        End();  // Present goes here
    }
    renderTotalReal.End();
    renderTotalReal.FrameEnd();
    stats.RenderTotal.accum = renderTotalReal.accum;
#endif                     // #ifndef DEDICATED_SERVER
    syncFrameDone.Wait();  // wait until secondary thread finish its job
#ifdef DEDICATED_SERVER
    u32 FrameEndTime = TimerGlobal.GetElapsed_ms();
    u32 FrameTime = (FrameEndTime - FrameStartTime);
    u32 DSUpdateDelta = 1000 / g_svDedicateServerUpdateReate;
    if (FrameTime < DSUpdateDelta) Sleep(DSUpdateDelta - FrameTime);
#endif
    if (!b_is_Active) Sleep(1);
}

#ifdef INGAME_EDITOR
void CRenderDevice::message_loop_editor()
{
    m_editor->run();
    m_editor_finalize(m_editor);
    xr_delete(m_engine);
}
#endif  // #ifdef INGAME_EDITOR

void CRenderDevice::message_loop()
{
#ifdef INGAME_EDITOR
    if (editor()) {
        message_loop_editor();
        return;
    }
#endif
    MSG msg;
    PeekMessage(&msg, NULL, 0U, 0U, PM_NOREMOVE);
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        on_idle();
    }
}

void CRenderDevice::Run()
{
    g_bLoaded = FALSE;
    Log("Starting engine...");
    thread_name("X-RAY Primary thread");
    // Startup timers and calculate timer delta
    dwTimeGlobal = 0;
    Timer_MM_Delta = 0;
    {
        u32 time_mm = timeGetTime();
        while (timeGetTime() == time_mm)
            ;  // wait for next tick
        u32 time_system = timeGetTime();
        u32 time_local = TimerAsync();
        Timer_MM_Delta = time_system - time_local;
    }
    // Start all threads
    mt_bMustExit = FALSE;
    thread_spawn(SecondaryThreadProc, "X-RAY Secondary thread", 0, this);
    // App start
    seqAppStart.Process(rp_AppStart);
    GlobalEnv.Render->ClearTarget();
    // Load FPS Lock
    if (strstr(Core.Params, "-nofpslock"))
        g_dwFPSlimit = -1;
    else if (strstr(Core.Params, "-fpslock60"))
        g_dwFPSlimit = 60;
    // Message cycle
    message_loop();
    seqAppEnd.Process(rp_AppEnd);
    // Stop Balance-Thread
    mt_bMustExit = TRUE;
    syncProcessFrame.Set();
    syncThreadExit.Wait();
    while (mt_bMustExit)
        Sleep(0);
}

u32 app_inactive_time = 0;
u32 app_inactive_time_start = 0;

void CRenderDevice::FrameMove()
{
    dwFrame++;
    Core.dwFrame = dwFrame;
    dwTimeContinual = TimerMM.GetElapsed_ms() - app_inactive_time;
    if (psDeviceFlags.test(rsConstantFPS)) {
        // 20ms = 50fps
        // fTimeDelta = 0.020f;
        // fTimeGlobal += 0.020f;
        // dwTimeDelta = 20;
        // dwTimeGlobal += 20;
        // 33ms = 30fps
        fTimeDelta = 0.033f;
        fTimeGlobal += 0.033f;
        dwTimeDelta = 33;
        dwTimeGlobal += 33;
    }
    else
    {
        // Timer
        float fPreviousFrameTime = Timer.GetElapsed_sec();
        Timer.Start();  // previous frame
        fTimeDelta =
            0.1f * fTimeDelta + 0.9f * fPreviousFrameTime;  // smooth random system activity - worst case ~7% error
        // fTimeDelta = 0.7f * fTimeDelta + 0.3f*fPreviousFrameTime; // smooth random system activity
        if (fTimeDelta > .1f) fTimeDelta = .1f;             // limit to 15fps minimum
        if (fTimeDelta <= 0.f) fTimeDelta = EPS_S + EPS_S;  // limit to 15fps minimum
        if (Paused()) fTimeDelta = 0.0f;
        // u64 qTime = TimerGlobal.GetElapsed_clk();
        fTimeGlobal = TimerGlobal.GetElapsed_sec();  // float(qTime)*CPU::cycles2seconds;
        u32 _old_global = dwTimeGlobal;
        dwTimeGlobal = TimerGlobal.GetElapsed_ms();
        dwTimeDelta = dwTimeGlobal - _old_global;
    }
    // Frame move
    stats.EngineTotal.FrameStart();
    stats.EngineTotal.Begin();
    // TODO: HACK to test loading screen.
    // if(!g_bLoaded)
    Device.seqFrame.Process(rp_Frame);
    g_bLoaded = TRUE;
    // else
    // seqFrame.Process(rp_Frame);
    stats.EngineTotal.End();
    stats.EngineTotal.FrameEnd();
}

ENGINE_API BOOL bShowPauseString = TRUE;
#include "IGame_Persistent.h"

void CRenderDevice::Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason)
{
    static int snd_emitters_ = -1;
    if (g_bBenchmark) return;
#ifndef DEDICATED_SERVER
    if (bOn) {
        if (!Paused())
            bShowPauseString =
#ifdef INGAME_EDITOR
                editor() ? FALSE :
#endif  // #ifdef INGAME_EDITOR
#ifdef DEBUG
                           !xr_strcmp(reason, "li_pause_key_no_clip") ? FALSE :
#endif  // DEBUG
                                                                        TRUE;
        if (bTimer && (!g_pGamePersistent || g_pGamePersistent->CanBePaused())) {
            g_pauseMngr()->Pause(TRUE);
#ifdef DEBUG
            if (!xr_strcmp(reason, "li_pause_key_no_clip")) TimerGlobal.Pause(FALSE);
#endif
        }
        if (bSound && ::Sound) snd_emitters_ = ::Sound->pause_emitters(true);
    }
    else
    {
        if (bTimer && g_pauseMngr()->Paused()) {
            fTimeDelta = EPS_S + EPS_S;
            g_pauseMngr()->Pause(FALSE);
        }
        if (bSound) {
            if (snd_emitters_ > 0)  // avoid crash
                snd_emitters_ = ::Sound->pause_emitters(false);
            else
            {
#ifdef DEBUG
                Log("Sound->pause_emitters underflow");
#endif
            }
        }
    }
#endif
}

BOOL CRenderDevice::Paused()
{
    return g_pauseMngr()->Paused();
}

void CRenderDevice::OnWM_Activate(WPARAM wParam, LPARAM lParam)
{
    u16 fActive = LOWORD(wParam);
    BOOL fMinimized = (BOOL)HIWORD(wParam);
    BOOL bActive = ((fActive != WA_INACTIVE) && (!fMinimized)) ? TRUE : FALSE;
    if (bActive != Device.b_is_Active) {
        Device.b_is_Active = bActive;
        if (Device.b_is_Active) {
            Device.seqAppActivate.Process(rp_AppActivate);
            app_inactive_time += TimerMM.GetElapsed_ms() - app_inactive_time_start;
#ifndef DEDICATED_SERVER
#ifdef INGAME_EDITOR
            if (!editor())
#endif  // #ifdef INGAME_EDITOR
                ShowCursor(FALSE);
#endif  // #ifndef DEDICATED_SERVER
        }
        else
        {
            app_inactive_time_start = TimerMM.GetElapsed_ms();
            Device.seqAppDeactivate.Process(rp_AppDeactivate);
            ShowCursor(TRUE);
        }
    }
}

void CRenderDevice::AddSeqFrame(pureFrame* f, bool mt)
{
    if (mt)
        seqFrameMT.Add(f, REG_PRIORITY_HIGH);
    else
        seqFrame.Add(f, REG_PRIORITY_LOW);
}

void CRenderDevice::RemoveSeqFrame(pureFrame* f)
{
    seqFrameMT.Remove(f);
    seqFrame.Remove(f);
}

CRenderDevice* get_device()
{
    return &Device;
}
u32 script_time_global()
{
    return Device.dwTimeGlobal;
}
u32 script_time_global_async()
{
    return Device.TimerAsync_MMT();
}

SCRIPT_EXPORT(Device, (), {
    using namespace luabind;
    module(luaState)[def("time_global", &script_time_global), def("time_global_async", &script_time_global_async),
        def("device", &get_device), def("is_enough_address_space_available", &is_enough_address_space_available)];
});

CLoadScreenRenderer::CLoadScreenRenderer() : b_registered(false)
{
}

void CLoadScreenRenderer::start(bool b_user_input)
{
    Device.seqRender.Add(this, 0);
    b_registered = true;
    b_need_user_input = b_user_input;
}

void CLoadScreenRenderer::stop()
{
    if (!b_registered) return;
    Device.seqRender.Remove(this);
    pApp->destroy_loading_shaders();
    b_registered = false;
    b_need_user_input = false;
}

void CLoadScreenRenderer::OnRender()
{
    pApp->load_draw_internal();
}
