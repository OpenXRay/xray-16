#pragma once
#ifndef xr_device
#define xr_device

// Note:
// ZNear - always 0.0f
// ZFar - always 1.0f

// class ENGINE_API CResourceManager;
// class ENGINE_API CGammaControl;

#include "pure.h"

#include "xrCore/FTimer.h"
#include "Stats.h"
#include "xrCommon/xr_list.h"
#include "xrCore/Threading/Event.hpp"
#include "xrCore/fastdelegate.h"
#include "xrCore/ModuleLookup.hpp"

#define VIEWPORT_NEAR 0.2f

#define DEVICE_RESET_PRECACHE_FRAME_COUNT 10

#include "Include/editor/interfaces.hpp"
#include "Include/xrRender/FactoryPtr.h"
#include "Render.h"
#include "SDL.h"

class Task;
class engine_impl;

#pragma pack(push, 4)

class ENGINE_API IRenderDevice
{
public:
    struct RenderDeviceStatictics
    {
        CStatTimer RenderTotal; // pureRender
        CStatTimer EngineTotal; // pureFrame
        float fFPS, fRFPS, fTPS; // FPS, RenderFPS, TPS

        RenderDeviceStatictics()
        {
            fFPS = 30.f;
            fRFPS = 30.f;
            fTPS = 0;
        }
    };

    virtual ~IRenderDevice() {}
    virtual void AddSeqFrame(pureFrame* f, bool mt) = 0;
    virtual void RemoveSeqFrame(pureFrame* f) = 0;
    virtual const RenderDeviceStatictics& GetStats() const = 0;
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) = 0;
};

class ENGINE_API CRenderDeviceData
{
public:
    static const u32 MaximalWaitTime; // ms

    // Rendering resolution
    u32 dwWidth;
    u32 dwHeight;

    // Real application window resolution
    SDL_Rect m_rcWindowBounds;

    // Real game window resolution
    SDL_Rect m_rcWindowClient;

    u32 dwPrecacheFrame;
    bool b_is_Ready;
    bool b_is_Active;
    bool IsAnselActive;
    bool AllowWindowDrag; // For windowed mode

    // Engine flow-control
    u32 dwFrame;

    float fTimeDelta;
    float fTimeGlobal;
    u32 dwTimeDelta;
    u32 dwTimeGlobal;
    u32 dwTimeContinual;

    Fvector vCameraPosition;
    Fvector vCameraDirection;
    Fvector vCameraTop;
    Fvector vCameraRight;

    Fmatrix mView;
    Fmatrix mProject;
    Fmatrix mFullTransform;

    // Copies of corresponding members. Used for synchronization.
    Fvector vCameraPositionSaved;
    Fvector vCameraDirectionSaved;
    Fvector vCameraTopSaved;
    Fvector vCameraRightSaved;

    Fmatrix mViewSaved;
    Fmatrix mProjectSaved;
    Fmatrix mFullTransformSaved;

    float fFOV;
    float fASPECT;

protected:
    u32 Timer_MM_Delta;
    CTimer_paused Timer;
    CTimer_paused TimerGlobal;

public:
    // Registrators
    MessageRegistry<pureRender> seqRender;
    MessageRegistry<pureAppActivate> seqAppActivate;
    MessageRegistry<pureAppDeactivate> seqAppDeactivate;
    MessageRegistry<pureAppStart> seqAppStart;
    MessageRegistry<pureAppEnd> seqAppEnd;
    MessageRegistry<pureFrame> seqFrame;

    SDL_Window* m_sdlWnd;
};

class ENGINE_API CRenderDeviceBase : public IRenderDevice, public CRenderDeviceData
{
protected:
    CStats* Statistic;
    CRenderDeviceBase() { Statistic = nullptr; }
};

#pragma pack(pop)
// refs
class ENGINE_API CRenderDevice : public CRenderDeviceBase, public IWindowHandler
{
    // Main objects used for creating and rendering the 3D scene
    CTimer TimerMM;
    RenderDeviceStatictics stats;

    void _SetupStates();

public:
#if defined(XR_PLATFORM_WINDOWS)
    LRESULT MsgProc(HWND, UINT, WPARAM, LPARAM);
#endif
    // u32 dwFrame;
    // u32 dwPrecacheFrame;
    u32 dwPrecacheTotal;

    // u32 dwWidth, dwHeight;
    float fWidth_2, fHeight_2;
    // bool b_is_Ready;
    // bool b_is_Active;
    void OnWM_Activate(WPARAM wParam, LPARAM lParam);

    // ref_shader m_WireShader;
    // ref_shader m_SelectionShader;

    bool m_bNearer;
    void SetNearer(bool enabled)
    {
        if (enabled && !m_bNearer)
        {
            m_bNearer = true;
            mProject._43 -= EPS_L;
        }
        else if (!enabled && m_bNearer)
        {
            m_bNearer = false;
            mProject._43 += EPS_L;
        }
        GEnv.Render->SetCacheXform(mView, mProject);
        // R_ASSERT(0);
        // TODO: re-implement set projection
        // RCache.set_xform_project (mProject);
    }

    void DumpResourcesMemoryUsage() { GEnv.Render->ResourcesDumpMemoryUsage(); }

    MessageRegistry<pureFrame> seqFrameMT;
    MessageRegistry<pureDeviceReset> seqDeviceReset;
    MessageRegistry<pureUIReset> seqUIReset;
    xr_vector<fastdelegate::FastDelegate0<>> seqParallel;

    Fmatrix mInvFullTransform;

    CRenderDevice()
        : dwPrecacheTotal(0), fWidth_2(0), fHeight_2(0),
          mt_bMustExit(false),
          m_editor_module(nullptr), m_editor_initialize(nullptr),
          m_editor_finalize(nullptr), m_editor(nullptr)
    {
        m_sdlWnd = NULL;
        b_is_Active = false;
        b_is_Ready = false;
        Timer.Start();
        m_bNearer = false;
    };

    void Pause(bool bOn, bool bTimer, bool bSound, pcstr reason);
    bool Paused();

private:
    void xr_stdcall ProcessParallelSequence(Task&, void*);

public:
    // Scene control
    void xr_stdcall ProcessFrame();

    void PreCache(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input);

    bool BeforeFrame();
    void FrameMove();

    void BeforeRender();
    void DoRender();
    bool RenderBegin();
    void Clear();
    void RenderEnd();

    void overdrawBegin();
    void overdrawEnd();

    // Mode control
    void DumpFlags();
    IC CTimer_paused* GetTimerGlobal() { return &TimerGlobal; }
    u32 TimerAsync() { return TimerGlobal.GetElapsed_ms(); }
    u32 TimerAsync_MMT() { return TimerMM.GetElapsed_ms() + Timer_MM_Delta; }

private:
    // Creation & Destroying
    void CreateInternal();
    void ResetInternal(bool precache = true);

public:
    void Create();

    void Run(void);
    void Destroy(void);
    void Reset(bool precache = true);

    void UpdateWindowProps(const bool windowed);
    void UpdateWindowRects();
    void SelectResolution(const bool windowed);

    void Initialize(void);
    void ShutDown(void);
    virtual const RenderDeviceStatictics& GetStats() const override { return stats; }
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;

    SDL_Window* GetApplicationWindow() override;
    void DisableFullscreen() override;
    void ResetFullscreen() override;

    void time_factor(const float& time_factor)
    {
        Timer.time_factor(time_factor);
        TimerGlobal.time_factor(time_factor);
    }

    IC const float time_factor() const
    {
        VERIFY(Timer.time_factor() == TimerGlobal.time_factor());
        return (Timer.time_factor());
    }

public:
    Event PresentationFinished = nullptr;
    volatile bool mt_bMustExit;

    // Usable only when called from thread, that initialized SDL
    // Calls SDL_PumpEvents() at least twice.
    static void WaitEvent(Event& event)
    {
        // Once at the beginning:
        SDL_PumpEvents();

        while (!event.Wait(MaximalWaitTime))
            SDL_PumpEvents();

        // And once in the end:
        SDL_PumpEvents();
    }

    ICF void remove_from_seq_parallel(const fastdelegate::FastDelegate0<>& delegate)
    {
        xr_vector<fastdelegate::FastDelegate0<>>::iterator I =
            std::find(seqParallel.begin(), seqParallel.end(), delegate);
        if (I != seqParallel.end())
            seqParallel.erase(I);
    }

private:
    void CalcFrameStats();

public:
#if !defined(XR_PLATFORM_LINUX)
    bool xr_stdcall on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
#endif

private:
    void message_loop();
    virtual void AddSeqFrame(pureFrame* f, bool mt);
    virtual void RemoveSeqFrame(pureFrame* f);

public:
    XRay::Editor::ide_base* editor() const { return m_editor; }

private:
    void initialize_weather_editor();
    void message_loop_weather_editor();

    using initialize_function_ptr = XRay::Editor::initialize_function_ptr;
    using finalize_function_ptr = XRay::Editor::finalize_function_ptr;

    XRay::Module m_editor_module;

    initialize_function_ptr m_editor_initialize;
    finalize_function_ptr m_editor_finalize;
    XRay::Editor::ide_base* m_editor;
};

extern ENGINE_API CRenderDevice Device;

#ifndef _EDITOR
#define RDEVICE Device
#else
#define RDEVICE EDevice
#endif

extern ENGINE_API bool g_bBenchmark;

typedef fastdelegate::FastDelegate0<bool> LOADING_EVENT;
extern ENGINE_API xr_list<LOADING_EVENT> g_loading_events;

class ENGINE_API CLoadScreenRenderer : public pureRender
{
public:
    CLoadScreenRenderer();
    void start(bool b_user_input);
    void stop();
    virtual void OnRender();
    bool IsActive() const { return b_registered; }

    bool b_registered;
    bool b_need_user_input;
};
extern ENGINE_API CLoadScreenRenderer load_screen_renderer;

class CDeviceResetNotifier : public pureDeviceReset
{
public:
    CDeviceResetNotifier(const int prio = REG_PRIORITY_NORMAL) { Device.seqDeviceReset.Add(this, prio); }
    virtual ~CDeviceResetNotifier() { Device.seqDeviceReset.Remove(this); }
    void OnDeviceReset() override {}
};

class CUIResetNotifier : public pureUIReset
{
public:
    CUIResetNotifier(const int uiResetPrio = REG_PRIORITY_NORMAL)
    {
        Device.seqUIReset.Add(this, uiResetPrio);
    }

    virtual ~CUIResetNotifier()
    {
        Device.seqUIReset.Remove(this);
    }

    void OnUIReset() override {}
};

#endif
