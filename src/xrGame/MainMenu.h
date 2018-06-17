#pragma once

class CUIWindow;
class CUIDialogWnd;
class CUICursor;
class CUIMessageBoxEx;

class demo_info_loader;

#include "xrEngine/IInputReceiver.h"
#include "xrEngine/IGame_Persistent.h"
#include "UIDialogHolder.h"
#include "ui/UIWndCallback.h"
#include "ui_base.h"
#include "DemoInfo.h"

class CMainMenu :
	public IMainMenu,
	public IInputReceiver,
	public pureRender,
	public CDialogHolder,
	public CUIWndCallback,
	public CDeviceResetNotifier

{
    CUIDialogWnd* m_startDialog;

    enum
    {
        flRestoreConsole = (1 << 0),
        flRestorePause = (1 << 1),
        flRestorePauseStr = (1 << 2),
        flActive = (1 << 3),
        flNeedChangeCapture = (1 << 4),
        flRestoreCursor = (1 << 5),
        flGameSaveScreenshot = (1 << 6),
        flNeedVidRestart = (1 << 7),
    };
    Flags16 m_Flags;
    string_path m_screenshot_name;
    u32 m_screenshotFrame;
    void ReadTextureInfo();

	xr_vector<CUIWindow*>				m_pp_draw_wnds;

    demo_info_loader* m_demo_info_loader;
    
    bool mLanguageChanged;

public:
	enum	EErrorDlg 
	{
		ErrInvalidPassword,
		ErrInvalidHost,
		ErrSessionFull,
		ErrServerReject,
		ErrCDKeyInUse,
		ErrCDKeyDisabled,
		ErrCDKeyInvalid,
		ErrDifferentVersion,
		ErrGSServiceFailed,
		ErrMasterServerConnectFailed,
		NoNewPatch,
		NewPatchFound,
		PatchDownloadError,
		PatchDownloadSuccess,
		ConnectToMasterServer,
		SessionTerminate,
		LoadingError,
		DownloadMPMap,
		ErrMax,
		ErrNoError = ErrMax,
	};
protected:
    EErrorDlg m_NeedErrDialog;
    u32 m_start_time;

	xr_vector<CUIMessageBoxEx*>	m_pMB_ErrDlgs;
	bool			ReloadUI						();
public:
    u32 m_deactivated_frame;
    bool m_activatedScreenRatio;
    virtual void DestroyInternal(bool bForce);
    CMainMenu();
    virtual ~CMainMenu();

    virtual void Activate(bool bActive);
    virtual bool IsActive();
    virtual bool CanSkipSceneRendering();

    virtual bool isLanguageChanged();
    virtual void setLanguageChanged(bool status);

    virtual bool IgnorePause() { return true; }
    virtual void IR_OnMousePress(int btn);
    virtual void IR_OnMouseRelease(int btn);
    virtual void IR_OnMouseHold(int btn);
    virtual void IR_OnMouseMove(int x, int y);
    virtual void IR_OnMouseStop(int x, int y);

    virtual void IR_OnKeyboardPress(int dik);
    virtual void IR_OnKeyboardRelease(int dik);
    virtual void IR_OnKeyboardHold(int dik);

    virtual void IR_OnMouseWheel(int direction);

    bool OnRenderPPUI_query();
    void OnRenderPPUI_main();
    void OnRenderPPUI_PP();

    virtual void OnRender();
    virtual void OnFrame(void);

    virtual bool UseIndicators() { return false; }
    void OnDeviceCreate();

    void Screenshot(IRender::ScreenshotMode mode = IRender::SM_NORMAL, LPCSTR name = 0);
    void RegisterPPDraw(CUIWindow* w);
    void UnregisterPPDraw(CUIWindow* w);

	void			SetErrorDialog					(EErrorDlg ErrDlg);
	EErrorDlg		GetErrorDialogType				() const { return m_NeedErrDialog; } ;
	void			CheckForErrorDlg				();

	void			OnLoadError						(LPCSTR module);

	void			SetNeedVidRestart				();
	virtual void	OnDeviceReset					();
	
	demo_info const *	GetDemoInfo					(LPCSTR file_name);
};

extern CMainMenu* MainMenu();
