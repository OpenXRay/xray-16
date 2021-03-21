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
#if defined(XR_PLATFORM_WINDOWS)
#include <mmsystem.h>
#endif
#include "SDL.h"
#pragma warning(pop)

#include <thread>

#include "x_ray.h"
#include "Render.h"

#include "xrCore/FS_impl.h"
#include "xrCore/Threading/TaskManager.hpp"

#include "Include/editor/ide.hpp"

#if !defined(XR_PLATFORM_LINUX)
#include "xrSASH.h"
#endif
#include "IGame_Persistent.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "XR_IOConsole.h"
#include "xr_input.h"
#include "splash.h"

ENGINE_API CRenderDevice Device;
ENGINE_API CLoadScreenRenderer load_screen_renderer;

ENGINE_API bool g_bRendering = false;

const u32 CRenderDeviceData::MaximalWaitTime = 16;
constexpr size_t MAX_WINDOW_EVENTS = 32;

extern int ps_always_active;

bool g_bLoaded = false;
ref_light precache_light = 0;

bool CRenderDevice::RenderBegin()
{
    if (GEnv.isDedicatedServer)
        return true;

    switch (GEnv.Render->GetDeviceState())
    {
    case DeviceState::Normal: break;
    case DeviceState::Lost:
        // If the device was lost, do not render until we get it back
        Sleep(33);
        return false;

    case DeviceState::NeedReset:
        // Check if the device is ready to be reset
        Reset();
        return false;

    default: R_ASSERT(0);
    }
    GEnv.Render->Begin();
    FPU::m24r();
    g_bRendering = true;

    return true;
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
            FIND_CHUNK_COUNTER_FLUSH();
            CheckPrivilegySlowdown();
            if (g_pGamePersistent->GameType() == 1 && !ps_always_active) // haCk
            {
                Uint32 flags = SDL_GetWindowFlags(m_sdlWnd);
                if ((flags & SDL_WINDOW_INPUT_FOCUS) == 0)
                    Pause(true, true, true, "application start");
            }
        }
    }
    g_bRendering = false;
    // end scene
    // Present goes here, so call OA Frame end.
#if !defined(XR_PLATFORM_LINUX)
    if (g_SASH.IsBenchmarkRunning())
        g_SASH.DisplayFrame(Device.fTimeGlobal);
#endif
    GEnv.Render->End();

    if (load_finished && m_editor)
        m_editor->on_load_finished();
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

#if !defined(XR_PLATFORM_LINUX)
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
    mInvFullTransform.invert_44(mFullTransform);

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

void CRenderDevice::ProcessParallelSequence(Task&, void*)
{
    for (u32 pit = 0; pit < seqParallel.size(); pit++)
        seqParallel[pit]();
    seqParallel.clear();
    seqFrameMT.Process();
}

void CRenderDevice::ProcessFrame()
{
    if (!BeforeFrame())
        return;

    const u64 frameStartTime = TimerGlobal.GetElapsed_ms();

    GEnv.Render->BeforeFrame();
    FrameMove();

    BeforeRender();

    const auto& processSeqParallel = TaskScheduler->AddTask("Secondary Thread Proc", { this, &CRenderDevice::ProcessParallelSequence });

    DoRender();

    const u64 frameEndTime = TimerGlobal.GetElapsed_ms();
    const u64 frameTime = frameEndTime - frameStartTime;

    u32 updateDelta = 1; // 1 ms

    if (GEnv.isDedicatedServer)
        updateDelta = 1000 / g_svDedicateServerUpdateReate;

    else if (Device.Paused())
        updateDelta = 16; // 16 ms, ~60 FPS max while paused

    if (frameTime < updateDelta)
        Sleep(updateDelta - frameTime);

    TaskScheduler->Wait(processSeqParallel);

    if (!b_is_Active)
        Sleep(1);
}

void CRenderDevice::message_loop_weather_editor()
{
    m_editor->run();
    m_editor_finalize(m_editor);
}

void CRenderDevice::message_loop()
{
    if (editor())
    {
        message_loop_weather_editor();
        return;
    }

    bool canCallActivate = false;
    bool shouldActivate = false;

    while (!SDL_QuitRequested()) // SDL_PumpEvents is here
    {
        SDL_Event events[MAX_WINDOW_EVENTS];
        const int count = SDL_PeepEvents(events, MAX_WINDOW_EVENTS,
            SDL_GETEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT);

        for (int i = 0; i < count; ++i)
        {
            const SDL_Event event = events[i];

            switch (event.type)
            {
            case SDL_WINDOWEVENT:
            {
                switch (event.window.event)
                {
                case SDL_WINDOWEVENT_MOVED:
                    UpdateWindowRects();
                    break;

                case SDL_WINDOWEVENT_SIZE_CHANGED:
                {
                    if (!psDeviceFlags.is(rsFullscreen))
                    {
                        if (psCurrentVidMode[0] == event.window.data1 && psCurrentVidMode[1] == event.window.data2)
                            break; // we don't need to reset device if resolution wasn't really changed

                        string32 buff;
                        xr_sprintf(buff, sizeof(buff), "vid_mode %dx%d", event.window.data1, event.window.data2);
                        Console->Execute(buff);

                        Reset();
                    }
                    else
                        UpdateWindowRects();

                    break;
                }

                case SDL_WINDOWEVENT_SHOWN:
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                case SDL_WINDOWEVENT_RESTORED:
                case SDL_WINDOWEVENT_MAXIMIZED:
                    canCallActivate = true;
                    shouldActivate = true;
                    break;

                case SDL_WINDOWEVENT_HIDDEN:
                case SDL_WINDOWEVENT_FOCUS_LOST:
                case SDL_WINDOWEVENT_MINIMIZED:
                    canCallActivate = true;
                    shouldActivate = false;
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
        // Workaround for screen blinking when there's too much timeouts
        if (canCallActivate)
        {
            OnWM_Activate(shouldActivate ? 1 : 0, 0);
            canCallActivate = false;
        }

        ProcessFrame();
    }
}

void CRenderDevice::Run()
{
    g_bLoaded = false;
    Log("Starting engine...");

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

    // Pre start
    seqAppStart.Process();

    splash::hide();
    SDL_HideWindow(m_sdlWnd);
    SDL_ShowWindow(m_sdlWnd);
    SDL_RaiseWindow(m_sdlWnd);
    UpdateWindowProps(!psDeviceFlags.is(rsFullscreen));
    if (GEnv.isDedicatedServer || strstr(Core.Params, "-center_screen"))
        SDL_SetWindowPosition(m_sdlWnd, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    OnWM_Activate(1, 0);

    // Message cycle
    message_loop();

    // Stop Balance-Thread
    mt_bMustExit = true;

    seqAppEnd.Process();
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
    g_bLoaded = true;
    // else
    // seqFrame.Process(rp_Frame);
    stats.EngineTotal.End();
    stats.EngineTotal.FrameEnd();
}

ENGINE_API bool bShowPauseString = true;
#include "IGame_Persistent.h"

void CRenderDevice::Pause(bool bOn, bool bTimer, bool bSound, pcstr reason)
{
    static int snd_emitters_ = -1;
    if (g_bBenchmark || GEnv.isDedicatedServer)
        return;

    if (bOn)
    {
        if (!Paused())
        {
            if (editor())
                bShowPauseString = false;
#ifdef DEBUG
            else if (xr_strcmp(reason, "li_pause_key_no_clip") == 0)
                bShowPauseString = false;
#endif
            else
                bShowPauseString = true;
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

bool CRenderDevice::Paused() { return g_pauseMngr().Paused(); }

void CRenderDevice::OnWM_Activate(WPARAM wParam, LPARAM /*lParam*/)
{
    u16 fActive = LOWORD(wParam);
    const bool fMinimized = (bool)HIWORD(wParam);

    const bool isWndActive = fActive != WA_INACTIVE && !fMinimized;

    if (!editor() && !GEnv.isDedicatedServer && isWndActive)
        pInput->GrabInput(true);
    else
        pInput->GrabInput(false);

    const bool isGameActive = ps_always_active || isWndActive;

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
