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
    // Rendering resolution
    u32 dwWidth;
    u32 dwHeight;

    // Real application window resolution
    SDL_Rect m_rcWindowBounds;

    // Real game window resolution
    SDL_Rect m_rcWindowClient;

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
    MessageRegistry<pureScreenResolutionChanged> seqResolutionChanged;

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
class ENGINE_API CRenderDevice : public CRenderDeviceBase
{
public:
    class ENGINE_API CSecondVPParams //--#SM+#-- +SecondVP+
    {
        bool isActive; // Флаг активации рендера во второй вьюпорт
        u8 frameDelay; // На каком кадре с момента прошлого рендера во второй вьюпорт мы начнём новый
                       //(не может быть меньше 2 - каждый второй кадр, чем больше тем более низкий FPS во втором
                       //вьюпорте)

    public:
        bool isCamReady; // Флаг готовности камеры (FOV, позиция, и т.п) к рендеру второго вьюпорта

        IC bool IsSVPActive() { return isActive; }
        IC void SetSVPActive(bool bState) { isActive = bState; }
        bool IsSVPFrame();

        IC u8 GetSVPFrameDelay() { return frameDelay; }
        void SetSVPFrameDelay(u8 iDelay)
        {
            frameDelay = iDelay;
            clamp<u8>(frameDelay, 2, u8(-1));
        }
    };

private:
    // Main objects used for creating and rendering the 3D scene
    CTimer TimerMM;
    RenderDeviceStatictics stats;

    void _SetupStates();

public:
    SDL_Window* m_sdlWnd;
#if defined(WINDOWS)
    LRESULT MsgProc(HWND, UINT, WPARAM, LPARAM);
#endif
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
        GEnv.Render->SetCacheXform(mView, mProject);
        // R_ASSERT(0);
        // TODO: re-implement set projection
        // RCache.set_xform_project (mProject);
    }

    void DumpResourcesMemoryUsage() { GEnv.Render->ResourcesDumpMemoryUsage(); }

    MessageRegistry<pureFrame> seqFrameMT;
    MessageRegistry<pureDeviceReset> seqDeviceReset;
    xr_vector<fastdelegate::FastDelegate0<>> seqParallel;
    CSecondVPParams m_SecondViewport; //--#SM+#-- +SecondVP+

    Fmatrix mInvFullTransform;

    CRenderDevice()
        : fWidth_2(0), fHeight_2(0), m_editor_module(nullptr), m_editor_initialize(nullptr),
          m_editor_finalize(nullptr), m_editor(nullptr), m_engine(nullptr)
    {
        m_sdlWnd = NULL;
        b_is_Active = FALSE;
        b_is_Ready = FALSE;
        Timer.Start();
        m_bNearer = FALSE;
        //--#SM+#-- +SecondVP+
        m_SecondViewport.SetSVPActive(false);
        m_SecondViewport.SetSVPFrameDelay(2);
        m_SecondViewport.isCamReady = false;
    };

    void Pause(BOOL bOn, BOOL bTimer, BOOL bSound, LPCSTR reason);
    BOOL Paused();

private:
    static void SecondaryThreadProc(void* context);
    static void RenderThreadProc(void* context);

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

    void UpdateWindowProps(const bool windowed);
    void UpdateWindowRects();
    void SelectResolution(const bool windowed);

    void Initialize(void);
    void ShutDown(void);
    virtual const RenderDeviceStatictics& GetStats() const override { return stats; }
    virtual void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert) override;

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

private:
    Event syncProcessFrame, syncFrameDone, syncThreadExit; // Secondary thread events
    Event renderProcessFrame, renderFrameDone, renderThreadExit; // Render thread events

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
#if !defined(LINUX)
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
    engine_impl* m_engine;
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

#endif
