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

#include "editor_base.h"
#include "Include/xrRender/FactoryPtr.h"
#include "Render.h"

#include <SDL.h>

class Task;

#pragma pack(push, 4)

// XXX: Merge CRenderDeviceData into CRenderDevice to make it look like in X-Ray 1.5

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

    // Main window
    SDL_Window* m_sdlWnd;

    u32 dwPrecacheFrame;
    bool b_is_Ready;
    bool b_is_Active;
    bool b_is_InFocus;
    bool IsAnselActive;

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
    Fmatrix mInvView;
    Fmatrix mProject;
    Fmatrix mFullTransform;
    Fmatrix mInvFullTransform;

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

    bool m_allowWindowDrag; // For windowed mode

public:
    // Registrators
    MessageRegistry<pureRender> seqRender;
    MessageRegistry<pureAppActivate> seqAppActivate;
    MessageRegistry<pureAppDeactivate> seqAppDeactivate;
    MessageRegistry<pureAppStart> seqAppStart;
    MessageRegistry<pureAppEnd> seqAppEnd;
    MessageRegistry<pureFrame> seqFrame;
};

#pragma pack(pop)
// refs
class ENGINE_API CRenderDevice : public CRenderDeviceData, public IWindowHandler
{
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

    // Main objects used for creating and rendering the 3D scene
    CTimer TimerMM;
    RenderDeviceStatictics stats;
    CStats* Statistic{};

    void _SetupStates();

public:
    // u32 dwFrame;
    // u32 dwPrecacheFrame;
    u32 dwPrecacheTotal;

    // u32 dwWidth, dwHeight;
    float fWidth_2, fHeight_2;
    // bool b_is_Ready;
    // bool b_is_Active;
    void OnWindowActivate(bool activated);

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

    MessageRegistry<pureFrame> seqFrameMT;
    MessageRegistry<pureDeviceReset> seqDeviceReset;
    MessageRegistry<pureUIReset> seqUIReset;
    xr_vector<fastdelegate::FastDelegate0<>> seqParallel;

    CRenderDevice()
        : dwPrecacheTotal(0), fWidth_2(0), fHeight_2(0),
          mt_bMustExit(false)
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
    void ProcessParallelSequence(Task&, void*);

public:
    // Scene control
    void ProcessFrame();

    void PreCache(u32 amount, bool draw_loadscreen, bool wait_user_input);

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
    IC CTimer_paused* GetTimerGlobal() { return &TimerGlobal; }
    u32 TimerAsync() { return TimerGlobal.GetElapsed_ms(); }
    u32 TimerAsync_MMT() { return TimerMM.GetElapsed_ms() + Timer_MM_Delta; }

private:
    // Creation & Destroying
    void CreateInternal();

public:
    void Create();

    void Run(void);
    void Destroy(void);
    void Reset(bool precache = true);

    void UpdateWindowProps();
    void UpdateWindowRects();
    void SelectResolution(bool windowed);

    void Initialize(void);
    void ShutDown(void);

    void FillVideoModes();
    void CleanupVideoModes();

    const RenderDeviceStatictics& GetStats() const { return stats; }
    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert);

    void SetWindowDraggable(bool draggable);
    bool IsWindowDraggable() const { return m_allowWindowDrag; }

    SDL_Window* GetApplicationWindow() override;
    void OnErrorDialog(bool beforeDialog) override;
    void OnFatalError() override;

    void time_factor(const float time_factor);

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

    void AddSeqFrame(pureFrame* f, bool mt);
    void RemoveSeqFrame(pureFrame* f);

    ICF void remove_from_seq_parallel(const fastdelegate::FastDelegate0<>& delegate)
    {
        xr_vector<fastdelegate::FastDelegate0<>>::iterator I =
            std::find(seqParallel.begin(), seqParallel.end(), delegate);
        if (I != seqParallel.end())
            seqParallel.erase(I);
    }

private:
    void CalcFrameStats();

private:
    void message_loop();

private:
    xray::editor::ide m_editor;

public:
    [[nodiscard]]
    auto& editor() { return m_editor; }

    [[nodiscard]]
    auto editor_mode() const { return m_editor.is_shown(); }
};

extern ENGINE_API CRenderDevice Device;

extern ENGINE_API bool g_bBenchmark;

typedef fastdelegate::FastDelegate0<bool> LOADING_EVENT;
extern ENGINE_API xr_list<LOADING_EVENT> g_loading_events;

class ENGINE_API CLoadScreenRenderer : public pureFrame, public pureRender
{
public:
    void OnFrame() override;
    void OnRender() override;

    void Start(bool b_user_input);
    void Stop();

    bool IsActive() const { return m_registered; }
    bool NeedsUserInput() const { return m_need_user_input; }

private:
    bool m_registered{};
    bool m_need_user_input{};
};
extern ENGINE_API CLoadScreenRenderer load_screen_renderer;

class CDeviceResetNotifier : public pureDeviceReset
{
public:
    CDeviceResetNotifier(const int prio = REG_PRIORITY_NORMAL) { Device.seqDeviceReset.Add(this, prio); }
    virtual ~CDeviceResetNotifier() { Device.seqDeviceReset.Remove(this); }
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
};

#endif
