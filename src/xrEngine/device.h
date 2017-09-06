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
#include "xrCore/Threading/Event.hpp"
#include "xrCommon/xr_list.h"

#define VIEWPORT_NEAR 0.2f

#define DEVICE_RESET_PRECACHE_FRAME_COUNT 10

#include "Include/xrRender/FactoryPtr.h"
#include "Render.h"

#ifdef INGAME_EDITOR
#include "Include/editor/interfaces.hpp"
#endif

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
    u32 dwWidth;
    u32 dwHeight;

    u32 dwPrecacheFrame;
    BOOL b_is_Ready;
    BOOL b_is_Active;

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
    Fvector vCameraPosition_saved;

    Fmatrix mView_saved;
    Fmatrix mProject_saved;
    Fmatrix mFullTransform_saved;

    float fFOV;
    float fASPECT;

protected:
    u32 Timer_MM_Delta;
    CTimer_paused Timer;
    CTimer_paused TimerGlobal;

public:
    // Registrators
    CRegistrator<pureRender> seqRender;
    CRegistrator<pureAppActivate> seqAppActivate;
    CRegistrator<pureAppDeactivate> seqAppDeactivate;
    CRegistrator<pureAppStart> seqAppStart;
    CRegistrator<pureAppEnd> seqAppEnd;
    CRegistrator<pureFrame> seqFrame;
    CRegistrator<pureScreenResolutionChanged> seqResolutionChanged;

    HWND m_hWnd;
};

class ENGINE_API CRenderDeviceBase : public IRenderDevice, public CRenderDeviceData
{
protected:
    CStats* Statistic;
    CRenderDeviceBase() { Statistic = nullptr; }
};

#pragma pack(pop)
// refs
class ENGINE_API CRenderDevice : public CRenderDeviceBase
{
    // Main objects used for creating and rendering the 3D scene
    u32 m_dwWindowStyle;
    RECT m_rcWindowBounds;
    RECT m_rcWindowClient;
    CTimer TimerMM;
    RenderDeviceStatictics stats;

    void _SetupStates();

public:
    // HWND m_hWnd;
    LRESULT MsgProc(HWND, UINT, WPARAM, LPARAM);

    // u32 dwFrame;
    // u32 dwPrecacheFrame;
    u32 dwPrecacheTotal;

    // u32 dwWidth, dwHeight;
    float fWidth_2, fHeight_2;
    // BOOL b_is_Ready;
    // BOOL b_is_Active;
    void OnWM_Activate(WPARAM wParam, LPARAM lParam);

    // ref_shader m_WireShader;
    // ref_shader m_SelectionShader;

    BOOL m_bNearer;
    void SetNearer(BOOL enabled)
    {
        if (enabled && !m_bNearer)
        {
            m_bNearer = TRUE;
            mProject._43 -= EPS_L;
        }
        else if (!enabled && m_bNearer)
        {
            m_bNearer = FALSE;
            mProject._43 += EPS_L;
        }
        GlobalEnv.Render->SetCacheXform(mView, mProject);
        // R_ASSERT(0);
        // TODO: re-implement set projection
        // RCache.set_xform_project (mProject);
    }

    void DumpResourcesMemoryUsage() { GlobalEnv.Render->ResourcesDumpMemoryUsage(); }

    CRegistrator<pureFrame> seqFrameMT;
    CRegistrator<pureDeviceReset> seqDeviceReset;
    xr_vector<fastdelegate::FastDelegate0<>> seqParallel;

    Fmatrix mInvFullTransform;

    CRenderDevice()
        : m_dwWindowStyle(0)
#ifdef INGAME_EDITOR
          ,
          m_editor_module(0), m_editor_initialize(0), m_editor_finalize(0), m_editor(0), m_engine(0)
#endif // #ifdef INGAME_EDITOR
    {
        m_hWnd = NULL;
        b_is_Active = FALSE;
        b_is_Ready = FALSE;
        Timer.Start();
        m_bNearer = FALSE;
    };

    void Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason);
    BOOL Paused();

private:
    static void SecondaryThreadProc(void* context);

public:
    // Scene control
    void PreCache(u32 amount, bool b_draw_loadscreen, bool b_wait_user_input);
    BOOL Begin();
    void Clear();
    void End();
    void FrameMove();

    void overdrawBegin();
    void overdrawEnd();

    // Mode control
    void DumpFlags();
    IC CTimer_paused* GetTimerGlobal() { return &TimerGlobal; }
    u32 TimerAsync() { return TimerGlobal.GetElapsed_ms(); }
    u32 TimerAsync_MMT() { return TimerMM.GetElapsed_ms() + Timer_MM_Delta; }
    // Creation & Destroying
    void Create(void);
    void Run(void);
    void Destroy(void);
    void Reset(bool precache = true);

    void Initialize(void);
    void ShutDown(void);
    virtual const RenderDeviceStatictics& GetStats() const override { return stats; }
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;

    void time_factor(const float& time_factor)
    {
        Timer.time_factor(time_factor);
        TimerGlobal.time_factor(time_factor);
    }

    IC const float& time_factor() const
    {
        VERIFY(Timer.time_factor() == TimerGlobal.time_factor());
        return (Timer.time_factor());
    }

private:
    Event syncProcessFrame, syncFrameDone, syncThreadExit;

public:
    volatile BOOL mt_bMustExit;

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
    void xr_stdcall on_idle();
    bool xr_stdcall on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);

private:
    void message_loop();
    virtual void AddSeqFrame(pureFrame* f, bool mt);
    virtual void RemoveSeqFrame(pureFrame* f);
#ifdef INGAME_EDITOR
public:
    IC XRay::Editor::ide_base* editor() const { return m_editor; }
private:
    void initialize_editor();
    void message_loop_editor();

    typedef XRay::Editor::initialize_function_ptr initialize_function_ptr;
    typedef XRay::Editor::finalize_function_ptr finalize_function_ptr;

    HMODULE m_editor_module;
    initialize_function_ptr m_editor_initialize;
    finalize_function_ptr m_editor_finalize;
    XRay::Editor::ide_base* m_editor;
    engine_impl* m_engine;
#endif // #ifdef INGAME_EDITOR
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

    bool b_registered;
    bool b_need_user_input;
};
extern ENGINE_API CLoadScreenRenderer load_screen_renderer;

#endif
