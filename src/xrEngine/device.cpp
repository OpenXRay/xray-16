#include "stdafx.h"
#include "xrCDB/Frustum.h"

#pragma warning(push)
#pragma warning(disable : 4995)
// mmsystem.h
#define MMNOSOUND
#define MMNOMIDI
#define MMNOAUX
#define MMNOMIXER
#define MMNOJOY
#if defined(WINDOWS)
#include <mmsystem.h>
#endif
#include "SDL.h"
#pragma warning(pop)

#include <thread>

#include "x_ray.h"
#include "Render.h"

// must be defined before include of FS_impl.h
#define INCLUDE_FROM_ENGINE
#include "xrCore/FS_impl.h"

#include "Include/editor/ide.hpp"
#include "engine_impl.hpp"

#include "xrEngine/TaskScheduler.hpp"

#if !defined(LINUX)
#include "xrSASH.h"
#endif
#include "IGame_Persistent.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "XR_IOConsole.h"
#include "xr_input.h"
#include "splash.h"

ENGINE_API CRenderDevice Device;
ENGINE_API CLoadScreenRenderer load_screen_renderer;

ENGINE_API BOOL g_bRendering = FALSE;

constexpr size_t MAX_WINDOW_EVENTS = 32;

extern int ps_always_active;

BOOL g_bLoaded = FALSE;
ref_light precache_light = 0;

BOOL CRenderDevice::RenderBegin()
{
    if (GEnv.isDedicatedServer)
        return TRUE;

    switch (GEnv.Render->GetDeviceState())
    {
    case DeviceState::Normal: break;
    case DeviceState::Lost:
        // If the device was lost, do not render until we get it back
        Sleep(33);
        return FALSE;
        break;
    case DeviceState::NeedReset:
        // Check if the device is ready to be reset
        RequireReset();
        return FALSE;
        break;
    default: R_ASSERT(0);
    }
    GEnv.Render->Begin();
    FPU::m24r();
    g_bRendering = TRUE;

    return TRUE;
}

void CRenderDevice::Clear() { GEnv.Render->Clear(); }
extern void CheckPrivilegySlowdown();

void CRenderDevice::RenderEnd(void)
{
    if (GEnv.isDedicatedServer)
        return;

    bool load_finished = false;
    if (dwPrecacheFrame)
    {
        GEnv.Sound->set_master_volume(0.f);
        dwPrecacheFrame--;
        if (!dwPrecacheFrame)
        {
            load_finished = true;
            GEnv.Render->updateGamma();
            if (precache_light)
            {
                precache_light->set_active(false);
                precache_light.destroy();
            }
            GEnv.Sound->set_master_volume(1.f);
            GEnv.Render->ResourcesDestroyNecessaryTextures();
            Memory.mem_compact();
            Msg("* MEMORY USAGE: %d K", Memory.mem_usage() / 1024);
            Msg("* End of synchronization A[%d] R[%d]", b_is_Active, b_is_Ready);
#ifdef FIND_CHUNK_BENCHMARK_ENABLE
            g_find_chunk_counter.flush();
#endif
            CheckPrivilegySlowdown();
            if (g_pGamePersistent->GameType() == 1 && !ps_always_active) // haCk
            {
                Uint32 flags = SDL_GetWindowFlags(m_sdlWnd);
                if ((flags & SDL_WINDOW_INPUT_FOCUS) == 0)
                    Pause(TRUE, TRUE, TRUE, "application start");
            }
        }
    }
    g_bRendering = FALSE;
    // end scene
    // Present goes here, so call OA Frame end.
#if !defined(LINUX)
    if (g_SASH.IsBenchmarkRunning())
        g_SASH.DisplayFrame(Device.fTimeGlobal);
#endif
    GEnv.Render->End();

    if (load_finished && m_editor)
        m_editor->on_load_finished();
}

// XXX: make it work correct in all situations
void CRenderDevice::RenderThreadProc(void* context)
{
    auto& device = *static_cast<CRenderDevice*>(context);
    while (true)
    {
        device.renderProcessFrame.Wait();
        if (device.mt_bMustExit)
        {
            device.renderThreadExit.Set();
            return;
        }

        if (!GEnv.isDedicatedServer)
        {
            // all rendering is done here
            CStatTimer renderTotalReal;
            renderTotalReal.FrameStart();
            renderTotalReal.Begin();
            if (device.b_is_Active && device.RenderBegin())
            {
                device.seqRender.Process();
                device.CalcFrameStats();
                device.Statistic->Show();
                device.RenderEnd(); // Present goes here
            }
            renderTotalReal.End();
            renderTotalReal.FrameEnd();
            device.stats.RenderTotal.accum = renderTotalReal.accum;
        }
        device.renderFrameDone.Set();
    }
}

void CRenderDevice::PrimaryThreadProc(void* context)
{
    auto& device = *static_cast<CRenderDevice*>(context);

    SDL_Event resetCheck;
    bool shouldSwitch = true; // always switch for the first time

    while (true)
    {
        device.primaryProcessFrame.Wait();
        if (device.mt_bMustExit)
        {
            GEnv.Render->MakeContextCurrent(false);
            device.primaryThreadExit.Set();
            return;
        }

        if (shouldSwitch)
        {
            GEnv.Render->MakeContextCurrent(true);
            shouldSwitch = false;
        }

        device.ProcessFrame();

        if (SDL_PeepEvents(&resetCheck, 1,
            SDL_PEEKEVENT, SDL_USEREVENT, SDL_USEREVENT))
        {
            if (resetCheck.user.type == device.resetEventId)
            {
                shouldSwitch = true;
                GEnv.Render->MakeContextCurrent(false);
            }
        }

        device.primaryFrameDone.Set();
    }
}

void CRenderDevice::SecondaryThreadProc(void* context)
{
    auto& device = *static_cast<CRenderDevice*>(context);
    while (true)
    {
        device.syncProcessFrame.Wait();
        if (device.mt_bMustExit)
        {
            device.syncThreadExit.Set();
            return;
        }
        for (u32 pit = 0; pit < device.seqParallel.size(); pit++)
            device.seqParallel[pit]();
        device.seqParallel.clear();
        device.seqFrameMT.Process();
        device.syncFrameDone.Set();
    }
}

#include "IGame_Level.h"
void CRenderDevice::PreCache(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input)
{
    if (GEnv.isDedicatedServer)
        amount = 0;
    else if (GEnv.Render->GetForceGPU_REF())
        amount = 0;

    dwPrecacheFrame = dwPrecacheTotal = amount;
    if (amount && !precache_light && g_pGameLevel && g_loading_events.empty())
    {
        precache_light = GEnv.Render->light_create();
        precache_light->set_shadow(false);
        precache_light->set_position(vCameraPosition);
        precache_light->set_color(255, 255, 255);
        precache_light->set_range(5.0f);
        precache_light->set_active(true);
    }
    if (amount && b_draw_loadscreen && !load_screen_renderer.b_registered)
    {
        pApp->LoadForceDrop();
        load_screen_renderer.start(b_wait_user_input);
    }
}

void CRenderDevice::CalcFrameStats()
{
    stats.RenderTotal.FrameEnd();
    do
    {
        // calc FPS & TPS
        if (fTimeDelta <= EPS_S)
            break;
        float fps = 1.f / fTimeDelta;
        // if (Engine.External.tune_enabled) vtune.update (fps);
        float fOne = 0.3f;
        float fInv = 1.0f - fOne;
        stats.fFPS = fInv * stats.fFPS + fOne * fps;
        if (stats.RenderTotal.result > EPS_S)
        {
            u32 renderedPolys = GEnv.Render->GetCacheStatPolys();
            stats.fTPS = fInv * stats.fTPS + fOne * float(renderedPolys) / (stats.RenderTotal.result * 1000.f);
            stats.fRFPS = fInv * stats.fRFPS + fOne * 1000.f / stats.RenderTotal.result;
        }
    } while (false);
    stats.RenderTotal.FrameStart();
}

int g_svDedicateServerUpdateReate = 100;

ENGINE_API xr_list<LOADING_EVENT> g_loading_events;

bool CRenderDevice::BeforeFrame()
{
    if (!b_is_Ready)
    {
        Sleep(100);
        return false;
    }

    if (psDeviceFlags.test(rsStatistic))
        g_bEnableStatGather = true; // XXX: why not use either rsStatistic or g_bEnableStatGather?
    else
        g_bEnableStatGather = false;

    if (!g_loading_events.empty())
    {
        if (g_loading_events.front()())
            g_loading_events.pop_front();
        pApp->LoadDraw();
        return false;
    }

#if !defined(LINUX)
    if (!Device.dwPrecacheFrame && !g_SASH.IsBenchmarkRunning() && g_bLoaded)
        g_SASH.StartBenchmark();
#endif

    return true;
}

void CRenderDevice::BeforeRender()
{
    // Precache
    if (dwPrecacheFrame)
    {
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
    GEnv.Render->SetCacheXform(mView, mProject);
    mInvFullTransform.invert(mFullTransform);

    vCameraPositionSaved = vCameraPosition;
    vCameraDirectionSaved = vCameraDirection;
    vCameraTopSaved = vCameraTop;
    vCameraRightSaved = vCameraRight;

    mFullTransformSaved = mFullTransform;
    mViewSaved = mView;
    mProjectSaved = mProject;
}

void CRenderDevice::DoRender()
{
    if (GEnv.isDedicatedServer)
        return;

    CStatTimer renderTotalReal;
    renderTotalReal.FrameStart();
    renderTotalReal.Begin();
    if (b_is_Active && RenderBegin())
    {
        // all rendering is done here
        seqRender.Process();

        CalcFrameStats();
        Statistic->Show();
        RenderEnd(); // Present goes here
    }
    renderTotalReal.End();
    renderTotalReal.FrameEnd();
    stats.RenderTotal.accum = renderTotalReal.accum;
}

void CRenderDevice::ProcessFrame()
{
    if (!BeforeFrame())
        return;

    const auto frameStartTime = TimerGlobal.GetElapsed_ms();

    GEnv.Render->BeforeFrame();
    FrameMove();

    BeforeRender();

    // renderProcessFrame.Set(); // allow render thread to do its job
    syncProcessFrame.Set(); // allow secondary thread to do its job
    mtProcessingAllowed = true;

    const auto frameEndTime = TimerGlobal.GetElapsed_ms();
    const auto frameTime = frameEndTime - frameStartTime;

    DoRender();

    // Eco render (by alpet)
    u32 updateDelta = 0;

    if (GEnv.isDedicatedServer)
        updateDelta = 1000 / g_svDedicateServerUpdateReate;

    else if (Device.Paused() || IGame_Persistent::IsMainMenuActive())
        updateDelta = 10;

    if (frameTime < updateDelta)
        Sleep(updateDelta - frameTime);

    syncFrameDone.Wait(); // wait until secondary thread finish its job
    // renderFrameDone.Wait(); // wait until render thread finish its job
    while (!TaskScheduler->TaskQueueIsEmpty())
        std::this_thread::yield();
    mtProcessingAllowed = false;

    if (!b_is_Active)
        Sleep(1);
}

void CRenderDevice::message_loop_weather_editor()
{
    m_editor->run();
    m_editor_finalize(m_editor);
    xr_delete(m_engine);
}

void CRenderDevice::message_loop()
{
    if (editor())
    {
        message_loop_weather_editor();
        return;
    }

    GEnv.Render->MakeContextCurrent(false);

    bool timedOut = false;

    while (!SDL_QuitRequested()) // SDL_PumpEvents is here
    {
        SDL_Event events[MAX_WINDOW_EVENTS];
        int count = 0;
        if (!timedOut)
        {
            count = SDL_PeepEvents(events, MAX_WINDOW_EVENTS,
                SDL_GETEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT);

            // We need to collect only SDL_USEREVENT,
            // Other types of events should not be collected
            // Let's do this like that:
            if (count < MAX_WINDOW_EVENTS)
            {
                count += SDL_PeepEvents(events + count, MAX_WINDOW_EVENTS - count,
                    SDL_GETEVENT, SDL_USEREVENT, SDL_USEREVENT);
            }
        }

        for (int i = 0; i < count; ++i)
        {
            const SDL_Event event = events[i];

            switch (event.type)
            {
            case SDL_USEREVENT:
            {
                if (event.user.type == resetEventId)
                {
                    GEnv.Render->MakeContextCurrent(true);
                    Reset(event.user.code);
                    GEnv.Render->MakeContextCurrent(false);
                }
                break;
            }
            case SDL_WINDOWEVENT:
            {
                switch (event.window.event)
                {
                case SDL_WINDOWEVENT_MOVED:
                    UpdateWindowRects();
                    break;

                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                {
                    if (!psDeviceFlags.is(rsFullscreen))
                    {
                        if (psCurrentVidMode[0] == event.window.data1 && psCurrentVidMode[1] == event.window.data2)
                            break; // we don't need to reset device if resolution wasn't really changed

                        string32 buff;
                        xr_sprintf(buff, sizeof(buff), "vid_mode %dx%d", event.window.data1, event.window.data2);
                        Console->Execute(buff);

                        GEnv.Render->MakeContextCurrent(true);
                        Reset();
                        GEnv.Render->MakeContextCurrent(false);
                    }
                    else
                        UpdateWindowRects();

                    break;
                }

                case SDL_WINDOWEVENT_SHOWN:
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                case SDL_WINDOWEVENT_RESTORED:
                case SDL_WINDOWEVENT_MAXIMIZED:
                    OnWM_Activate(1, event.window.data2);
                    break;

                case SDL_WINDOWEVENT_HIDDEN:
                case SDL_WINDOWEVENT_FOCUS_LOST:
                case SDL_WINDOWEVENT_MINIMIZED:
                    OnWM_Activate(0, event.window.data2);
                    break;

                case SDL_WINDOWEVENT_ENTER:
                    SDL_ShowCursor(SDL_FALSE);
                    break;

                case SDL_WINDOWEVENT_LEAVE:
                    SDL_ShowCursor(SDL_TRUE);
                    break;

                case SDL_WINDOWEVENT_CLOSE:
                    SDL_Event quit = { SDL_QUIT };
                    SDL_PushEvent(&quit);
                }
            }
            }
        }

        if (!timedOut)
        {
            primaryProcessFrame.Set();
        }

        timedOut = !primaryFrameDone.Wait(33);
    }

    if (timedOut)
        primaryFrameDone.Wait();
}

void CRenderDevice::Run()
{
    g_bLoaded = FALSE;
    Log("Starting engine...");
    thread_name("X-RAY Window thread");

    // Startup timers and calculate timer delta
    dwTimeGlobal = 0;
    Timer_MM_Delta = 0;
    {
        u32 time_mm = timeGetTime();
        while (timeGetTime() == time_mm)
            ; // wait for next tick
        u32 time_system = timeGetTime();
        u32 time_local = TimerAsync();
        Timer_MM_Delta = time_system - time_local;
    }

    resetEventId = SDL_RegisterEvents(1);
    R_ASSERT2(resetEventId != u32(-1), "Failed to allocate Device reset SDL event.");

    // Start all threads
    mt_bMustExit = FALSE;

    thread_spawn(PrimaryThreadProc, "X-RAY Primary thread", 0, this);
    thread_spawn(SecondaryThreadProc, "X-RAY Secondary thread", 0, this);
    // thread_spawn(RenderThreadProc, "X-RAY Render thread", 0, this);

    TaskScheduler.reset(new TaskManager());
    TaskScheduler->Initialize();

    // Message cycle
    seqAppStart.Process();
    GEnv.Render->ClearTarget();
    splash::hide();
    if (GEnv.isDedicatedServer || strstr(Core.Params, "-center_screen"))
        SDL_SetWindowPosition(m_sdlWnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_HideWindow(m_sdlWnd);
    SDL_FlushEvents(SDL_WINDOWEVENT, SDL_SYSWMEVENT);
    SDL_ShowWindow(m_sdlWnd);
    SDL_RaiseWindow(m_sdlWnd);
    pInput->GrabInput(true);
    
    message_loop();

    // Stop Balance-Thread
    mt_bMustExit = TRUE;

    primaryProcessFrame.Set();
    primaryThreadExit.Wait();
    GEnv.Render->MakeContextCurrent(true);

    seqAppEnd.Process();
    
    // renderProcessFrame.Set();
    // renderThreadExit.Wait();
    
    syncProcessFrame.Set();
    syncThreadExit.Wait();
}

u32 app_inactive_time = 0;
u32 app_inactive_time_start = 0;

void CRenderDevice::FrameMove()
{
    dwFrame++;
    Core.dwFrame = dwFrame;
    dwTimeContinual = TimerMM.GetElapsed_ms() - app_inactive_time;
    if (psDeviceFlags.test(rsConstantFPS))
    {
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
        Timer.Start(); // previous frame
        fTimeDelta =
            0.1f * fTimeDelta + 0.9f * fPreviousFrameTime; // smooth random system activity - worst case ~7% error
        // fTimeDelta = 0.7f * fTimeDelta + 0.3f*fPreviousFrameTime; // smooth random system activity
        if (fTimeDelta > .1f)
            fTimeDelta = .1f; // limit to 15fps minimum
        if (fTimeDelta <= 0.f)
            fTimeDelta = EPS_S + EPS_S; // limit to 15fps minimum
        if (Paused())
            fTimeDelta = 0.0f;
        // u64 qTime = TimerGlobal.GetElapsed_clk();
        fTimeGlobal = TimerGlobal.GetElapsed_sec(); // float(qTime)*CPU::cycles2seconds;
        u32 _old_global = dwTimeGlobal;
        dwTimeGlobal = TimerGlobal.GetElapsed_ms();
        dwTimeDelta = dwTimeGlobal - _old_global;
    }
    // Frame move
    stats.EngineTotal.FrameStart();
    stats.EngineTotal.Begin();
    // TODO: HACK to test loading screen.
    // if(!g_bLoaded)
    Device.seqFrame.Process();
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
    if (g_bBenchmark || GEnv.isDedicatedServer)
        return;

    if (bOn)
    {
        if (!Paused())
        {
            if (bShowPauseString && editor())
                bShowPauseString = FALSE;
#ifdef DEBUG
            else if (xr_strcmp(reason, "li_pause_key_no_clip") == 0)
                bShowPauseString = FALSE;
#endif
        }
        if (bTimer && (!g_pGamePersistent || g_pGamePersistent->CanBePaused()))
        {
            g_pauseMngr().Pause(true);
#ifdef DEBUG
            if (xr_strcmp(reason, "li_pause_key_no_clip") == 0)
                TimerGlobal.Pause(false);
#endif
        }
        if (bSound && GEnv.Sound)
            snd_emitters_ = GEnv.Sound->pause_emitters(true);
    }
    else
    {
        if (bTimer && g_pauseMngr().Paused())
        {
            fTimeDelta = EPS_S + EPS_S;
            g_pauseMngr().Pause(false);
        }
        if (bSound)
        {
            if (snd_emitters_ > 0) // avoid crash
                snd_emitters_ = GEnv.Sound->pause_emitters(false);
            else
            {
#ifdef DEBUG
                Log("GEnv.Sound->pause_emitters underflow");
#endif
            }
        }
    }
}

BOOL CRenderDevice::Paused() { return g_pauseMngr().Paused(); }
void CRenderDevice::OnWM_Activate(WPARAM wParam, LPARAM /*lParam*/)
{
    u16 fActive = LOWORD(wParam);
    const BOOL fMinimized = (BOOL)HIWORD(wParam);

    const BOOL isWndActive = (fActive != WA_INACTIVE && !fMinimized) ? TRUE : FALSE;

    if (!editor() && !GEnv.isDedicatedServer && isWndActive)
        pInput->GrabInput(true);
    else
        pInput->GrabInput(false);

    const BOOL isGameActive = ps_always_active || isWndActive;

    if (isGameActive != Device.b_is_Active)
    {
        Device.b_is_Active = isGameActive;
        if (Device.b_is_Active)
        {
            Device.seqAppActivate.Process();
            app_inactive_time += TimerMM.GetElapsed_ms() - app_inactive_time_start;
        }
        else
        {
            app_inactive_time_start = TimerMM.GetElapsed_ms();
            Device.seqAppDeactivate.Process();
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

CRenderDevice* get_device() { return &Device; }
u32 script_time_global() { return Device.dwTimeGlobal; }
u32 script_time_global_async() { return Device.TimerAsync_MMT(); }

SCRIPT_EXPORT(Device, (),
{
    using namespace luabind;
    module(luaState)
    [
        def("time_global", &script_time_global),
        def("time_global_async", &script_time_global_async),
        def("device", &get_device),
        def("is_enough_address_space_available", &is_enough_address_space_available)
    ];
});

CLoadScreenRenderer::CLoadScreenRenderer() : b_registered(false), b_need_user_input(false) {}
void CLoadScreenRenderer::start(bool b_user_input)
{
    Device.seqRender.Add(this, 0);
    b_registered = true;
    b_need_user_input = b_user_input;

    pApp->LoadForceDrop();
    pApp->ShowLoadingScreen(true);
}

void CLoadScreenRenderer::stop()
{
    if (!b_registered)
        return;
    Device.seqRender.Remove(this);

    b_registered = false;
    b_need_user_input = false;

    pApp->LoadForceFinish();
    pApp->ShowLoadingScreen(false);
}

void CLoadScreenRenderer::OnRender() { pApp->load_draw_internal(true); }

bool CRenderDevice::CSecondVPParams::IsSVPFrame() //--#SM+#-- +SecondVP+
{
    return IsSVPActive() && Device.dwFrame % frameDelay == 0;
}
