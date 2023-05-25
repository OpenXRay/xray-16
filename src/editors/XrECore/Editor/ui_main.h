//---------------------------------------------------------------------------
#ifndef UI_MainH
#define UI_MainH

#include "UI_MainCommand.h"
#include "IInputReceiver.h"

// refs
class CCustomObject;
class TUI_Tools;
class TUI_Tools;
class C3DCursor;
//------------------------------------------------------------------------------

enum EEditorState
{
    esNone,
    esEditScene,
    esEditLibrary,
    esEditLightAnim,
    esBuildLevel
};

struct ECORE_API SPBItem
{
    shared_str text;
    shared_str info;
    float max;
    float progress;

public:
    SPBItem(LPCSTR txt, LPCSTR inf, float mx) : text(txt), info(inf), max(mx), progress(0.f) {}
    void GetInfo(xr_string &txt, float &p, float &m);
    void Inc(LPCSTR info = 0, bool bWarn = false);
    void Update(float val);
    void Info(LPCSTR text, bool bWarn = false);
};

typedef xr_vector<EEditorState> EStateList;
typedef EStateList::iterator EStateIt;

class ECORE_API TUI : public IInputReceiver, public XrUIManager
{
    bool m_AppClosed;
    inline void RealQuit() { m_AppClosed = true; }

protected:
    Ivector2 m_Size;
    bool m_Size_Maximize;

protected:
    friend class CCustomPreferences;
    friend class CEditorRenderDevice;

    TShiftState m_ShiftState;

    bool m_bAppActive;

protected:
    EStateList m_EditorState;
    bool bNeedAbort;

public:
    bool m_bReady;

protected:
    Fvector m_Pivot;

protected:
    bool m_SelectionRect;
    Ivector2 m_SelStart;
    Ivector2 m_SelEnd;

protected:
    enum
    {
        flRedraw = (1 << 0),
        flUpdateScene = (1 << 1),
        flResize = (1 << 2),
        flNeedQuit = (1 << 3),
        flResetUI = (1 << 4),
    };
    Flags32 m_Flags;

protected:
    long m_StartTime;

    void PrepareRedraw();
    void Redraw();

protected:
    void D3D_CreateStateBlocks();
    void D3D_DestroyStateBlocks();

public:
    virtual void OutUICursorPos() = 0;
    virtual void OutGridSize() = 0;
    virtual void OutInfo() = 0;

public:
    // non-hidden ops
    Ivector2 m_StartCp;
    Ivector2 m_CurrentCp;

    Fvector m_CurrentRStart;
    Fvector m_CurrentRDir;

    // hidden ops
    Ivector2 m_StartCpH;
    Ivector2 m_DeltaCpH;

protected:
    bool m_MouseCaptured;
    bool m_MouseMultiClickCaptured;
    bool bMouseInUse;

    xr_string m_LastHint;
    bool m_bHintShowing;
    POINT m_HintPoint;

    // mailslot
    HANDLE hMailSlot;

public:
    void ShowObjectHint();
    void ShowHint(const xr_string &s);
    bool ShowHint(const AStringVec &SS);
    void HideHint();

public:
    // mouse sensetive
    float m_MouseSM, m_MouseSS, m_MouseSR;

protected:
    virtual void RealUpdateScene() = 0;
    void RealRedrawScene();
    void RealResize();
    void OnFrame();

public:
    TUI();
    virtual ~TUI();

    void Quit() { m_Flags.set(flNeedQuit, TRUE); }

    u32 &GetRenderWidth() { return EDevice.m_RenderWidth; }
    u32 &GetRenderHeight() { return EDevice.m_RenderHeight; }
    int GetRealWidth() { return EDevice.dwWidth; }
    int GetRealHeight() { return EDevice.dwHeight; }

    IC float ZFar() { return EDevice.m_Camera.m_Zfar; }
    IC TShiftState GetShiftState() { return m_ShiftState; }

    virtual bool OnCreate();
    virtual void OnDestroy();

    virtual char *GetCaption() = 0;

    bool IsModified();

    bool Idle();
    void Resize(int x, int y, bool maximize = false, bool bForced = false)
    {
        m_Size.set(x, y);
        m_Size_Maximize = maximize;
        m_Flags.set(flResize | flRedraw, TRUE);
        if (bForced)
            RealResize();
    }
    void Resize(bool bForced = false)
    {
        m_Flags.set(flResize | flRedraw, TRUE);
        if (bForced)
            RealResize();
    }

    // add, remove, changing objects/scene
    void UpdateScene(bool bForced = false)
    {
        m_Flags.set(flUpdateScene, TRUE);
        if (bForced)
            RealUpdateScene();
    }
    // only redraw scene
    void RedrawScene(bool bForced = false)
    {
        m_Flags.set(flRedraw, TRUE);
        if (bForced)
            RealRedrawScene();
    }

    void SetRenderQuality(float q) { EDevice.m_ScreenQuality = q; }
    // mouse action
    void EnableSelectionRect(bool flag);
    void UpdateSelectionRect(const Ivector2 &from, const Ivector2 &to);

    void MouseMultiClickCapture(bool b) { m_MouseMultiClickCaptured = b; }

    bool IsMouseCaptured() { return m_MouseCaptured | m_MouseMultiClickCaptured; }
    bool IsMouseInUse() { return bMouseInUse; }

    bool KeyDown(WORD Key, TShiftState Shift);
    bool KeyUp(WORD Key, TShiftState Shift);
    bool KeyPress(WORD Key, TShiftState Shift);
    void MousePress(TShiftState Shift, int X, int Y);
    void MouseRelease(TShiftState Shift, int X, int Y);
    void MouseMove(TShiftState Shift, int X, int Y);

    void BeginEState(EEditorState st) { m_EditorState.push_back(st); }
    void EndEState() { m_EditorState.pop_back(); }
    void EndEState(EEditorState st)
    {
        VERIFY(std::find(m_EditorState.begin(), m_EditorState.end(), st) != m_EditorState.end());
        for (EStateIt it = m_EditorState.end() - 1; it >= m_EditorState.begin(); it--)
            if (*it == st)
            {
                m_EditorState.erase(it, m_EditorState.end());
                break;
            }
    }
    EEditorState GetEState() { return m_EditorState.back(); }
    bool ContainEState(EEditorState st) { return std::find(m_EditorState.begin(), m_EditorState.end(), st) != m_EditorState.end(); }

    virtual void OutCameraPos() = 0;
    virtual void SetStatus(LPCSTR s, bool bOutLog = true) = 0;
    virtual void ResetStatus() = 0;

    // direct input
    virtual void IR_OnMouseMove(int x, int y);

    void OnAppActivate();
    void OnAppDeactivate();

    bool NeedAbort() { return bNeedAbort; }
    void NeedBreak() { bNeedAbort = true; }
    void ResetBreak() { bNeedAbort = false; }

    virtual bool ApplyShortCut(DWORD Key, TShiftState Shift) = 0;
    virtual bool ApplyGlobalShortCut(DWORD Key, TShiftState Shift) = 0;

    void SetGradient(u32 color) { ; }

    void OnDeviceCreate();
    void OnDeviceDestroy();

    // mailslot
#if 0
	bool 			CreateMailslot		();
	void 			CheckMailslot		();
	void 			OnReceiveMail		(LPCSTR msg);
	void 			SendMail			(LPCSTR name, LPCSTR dest, LPCSTR msg);
#endif

    void CheckWindowPos(HWND *form);

    virtual LPCSTR EditorName() = 0;
    virtual LPCSTR EditorDesc() = 0;
    virtual HICON EditorIcon() { return nullptr; }

    // commands
    virtual void RegisterCommands() = 0;
    void ClearCommands();

    CCommandVar CommandRenderFocus(CCommandVar p1, CCommandVar p2);
    CCommandVar CommandBreakLastOperation(CCommandVar p1, CCommandVar p2);
    CCommandVar CommandRenderResize(CCommandVar p1, CCommandVar p2);

    virtual void SaveSettings(CInifile *) {}
    virtual void LoadSettings(CInifile *) {}

protected:
    // progress bar
    DEFINE_VECTOR(SPBItem *, PBVec, PBVecIt);
    PBVec m_ProgressItems;

public:
    SPBItem *ProgressStart(float max_val, LPCSTR text);
    void ProgressEnd(SPBItem *&);
    virtual void ProgressDraw();
    SPBItem *ProgressLast() { return m_ProgressItems.empty() ? 0 : m_ProgressItems.back(); }

public:
    ref_rt RT;
    ref_rt ZB;
    _vector2<u32> RTSize;

protected:
    virtual void OnDrawUI();
    void RealResetUI();
    HANDLE m_HConsole;

public:
    IC void ResetUI(bool bForced = false)
    {
        if (!bForced)
            m_Flags.set(flResetUI, TRUE);
        if (bForced)
            RealResetUI();
    }
    virtual Ivector2 GetRenderMousePosition() const { return Ivector2().set(0, 0); }
};
//---------------------------------------------------------------------------
extern ECORE_API TUI *UI;
//---------------------------------------------------------------------------
void ECORE_API ResetActionToSelect();
#define COMMAND0(cmd)     \
    {                     \
        ExecCommand(cmd); \
        bExec = true;     \
    }
#define COMMAND1(cmd, p0)     \
    {                         \
        ExecCommand(cmd, p0); \
        bExec = true;         \
    }
//---------------------------------------------------------------------------
#endif
