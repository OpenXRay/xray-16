#include "StdAfx.h"
#include "MainMenu.h"
#include "ui/UIDialogWnd.h"
#include "ui/UIMessageBoxEx.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/CameraManager.h"
#include "xr_level_controller.h"
#include "xrUICore/XML/UITextureMaster.h"
#include "ui/UIXmlInit.h"
#include "SDL.h"
#include "xrUICore/Buttons/UIBtnHint.h"
#include "xrUICore/Cursor/UICursor.h"
#include "xrGameSpy/GameSpy_Full.h"
#include "xrGameSpy/GameSpy_HTTP.h"
#include "xrGameSpy/GameSpy_Available.h"
#include "xrGameSpy/GameSpy_Browser.h"
#include "xrGameSpy/xrGameSpy.h"
#include "CdkeyDecode/cdkeydecode.h"
#include "string_table.h"
#include "xrCore/os_clipboard.h"
#include "xrGame/game_type.h"

#include "DemoInfo.h"
#include "DemoInfo_Loader.h"

#include "ui/UICDkey.h"

#ifdef XR_PLATFORM_WINDOWS
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
#endif

#include "Common/object_broker.h"

#include "account_manager.h"
#include "login_manager.h"
#include "profile_store.h"
#include "stats_submitter.h"
#include "atlas_submit_queue.h"
#include "xrEngine/xr_input.h"

// fwd. decl.
extern ENGINE_API bool bShowPauseString;

//#define DEMO_BUILD

constexpr cpcstr ErrMsgBoxTemplate[] =
{
    "message_box_invalid_pass",
    "message_box_invalid_host",
    "message_box_session_full",
    "message_box_server_reject",
    "message_box_cdkey_in_use",
    "message_box_cdkey_disabled",
    "message_box_cdkey_invalid",
    "message_box_different_version",
    "message_box_gs_service_not_available",
    "message_box_sb_master_server_connect_failed",
    "msg_box_no_new_patch",
    "msg_box_new_patch",
    "msg_box_patch_download_error",
    "msg_box_patch_download_success",
    "msg_box_connect_to_master_server",
    "msg_box_kicked_by_server",
    "msg_box_error_loading",
    "message_box_download_level"
};

extern bool b_shniaganeed_pp;

CMainMenu* MainMenu() { return (CMainMenu*)g_pGamePersistent->m_pMainMenu; };

CMainMenu::CMainMenu()
{
    class CResetEventCb : public CEventNotifierCallbackWithCid
    {
        CMainMenu* m_mainmenu;

    public:
        CResetEventCb(CID cid, CMainMenu* mm) : m_mainmenu(mm), CEventNotifierCallbackWithCid(cid) {}
        void ProcessEvent() override { m_mainmenu->DestroyInternal(true); }
    };

    m_script_reset_event_cid = ai().template Subscribe<CResetEventCb>(CAI_Space::EVENT_SCRIPT_ENGINE_RESET, this);

    m_Flags.zero();
    m_startDialog = NULL;
    m_screenshotFrame = u32(-1);
    g_pGamePersistent->m_pMainMenu = this;
    if (Device.b_is_Ready)
        OnDeviceCreate();
    g_btnHint = NULL;
    g_statHint = NULL;
    m_deactivated_frame = 0;

    m_sPatchURL = "";
#ifdef XR_PLATFORM_WINDOWS
    m_pGameSpyFull = NULL;
    m_account_mngr = NULL;
    m_login_mngr = NULL;
    m_profile_store = NULL;
    m_stats_submitter = NULL;
    m_atlas_submit_queue = NULL;
#endif

    m_sPDProgress.IsInProgress = false;
    m_downloaded_mp_map_url._set("");

    //-------------------------------------------

    m_NeedErrDialog = ErrNoError;
    m_start_time = 0;

    GetPlayerName();
    GetCDKeyFromRegistry();
    m_demo_info_loader = NULL;

    if (!GEnv.isDedicatedServer)
    {
        g_btnHint = xr_new<CUIButtonHint>();
        g_statHint = xr_new<CUIButtonHint>();
        m_pGameSpyFull = xr_new<CGameSpy_Full>();

#ifdef XR_PLATFORM_WINDOWS
        for (cpcstr name : ErrMsgBoxTemplate)
        {
            CUIMessageBoxEx* msgBox = m_pMB_ErrDlgs.emplace_back(xr_new<CUIMessageBoxEx>());
            if (!msgBox->InitMessageBox(name))
            {
                m_pMB_ErrDlgs.pop_back();
                xr_delete(msgBox);
            }
        }

        m_pMB_ErrDlgs[PatchDownloadSuccess]->AddCallbackStr("button_yes", MESSAGE_BOX_YES_CLICKED,
            CUIWndCallback::void_function(this, &CMainMenu::OnRunDownloadedPatch));
        m_pMB_ErrDlgs[PatchDownloadSuccess]->AddCallbackStr("button_yes", MESSAGE_BOX_OK_CLICKED,
            CUIWndCallback::void_function(this, &CMainMenu::OnConnectToMasterServerOkClicked));

        CUIMessageBoxEx* downloadMsg = m_pMB_ErrDlgs[DownloadMPMap];
        if (downloadMsg)
        {
            downloadMsg->AddCallbackStr("button_copy", MESSAGE_BOX_COPY_CLICKED,
                CUIWndCallback::void_function(this, &CMainMenu::OnDownloadMPMap_CopyURL));
            downloadMsg->AddCallbackStr(
                "button_yes", MESSAGE_BOX_YES_CLICKED, CUIWndCallback::void_function(this, &CMainMenu::OnDownloadMPMap));
        }

#endif

        m_account_mngr = xr_new<gamespy_gp::account_manager>(m_pGameSpyFull->GetGameSpyGP());
        m_login_mngr = xr_new<gamespy_gp::login_manager>(m_pGameSpyFull);
        m_profile_store = xr_new<gamespy_profile::profile_store>(m_pGameSpyFull);
#ifdef XR_PLATFORM_WINDOWS
        m_stats_submitter = xr_new<gamespy_profile::stats_submitter>(m_pGameSpyFull);
        m_atlas_submit_queue = xr_new<atlas_submit_queue>(m_stats_submitter);
#endif
    }

    Device.seqFrame.Add(this, REG_PRIORITY_LOW - 1000);
}

CMainMenu::~CMainMenu()
{
    Device.seqFrame.Remove(this);

    xr_delete(g_btnHint);
    xr_delete(g_statHint);
    xr_delete(m_startDialog);
    g_pGamePersistent->m_pMainMenu = nullptr;

#ifdef XR_PLATFORM_WINDOWS
    xr_delete(m_account_mngr);
    xr_delete(m_login_mngr);
    xr_delete(m_profile_store);
    xr_delete(m_stats_submitter);
    xr_delete(m_atlas_submit_queue);

    xr_delete(m_pGameSpyFull);
#endif

    xr_delete(m_demo_info_loader);
    delete_data(m_pMB_ErrDlgs);

    ai().Unsubscribe(m_script_reset_event_cid, CAI_Space::EVENT_SCRIPT_ENGINE_RESET);
}

void CMainMenu::Activate(bool bActivate)
{
    if (m_Flags.test(flActive) == bActivate)
        return;
    if (m_Flags.test(flGameSaveScreenshot))
        return;
    if ((m_screenshotFrame == Device.dwFrame) || (m_screenshotFrame == Device.dwFrame - 1) ||
        (m_screenshotFrame == Device.dwFrame + 1))
        return;

    bool b_is_single = IsGameTypeSingle();

    if (GEnv.isDedicatedServer && bActivate)
        return;

    if (bActivate)
    {
        b_shniaganeed_pp = true;
        Device.Pause(TRUE, FALSE, TRUE, "mm_activate1");
        m_Flags.set(flActive | flNeedChangeCapture, TRUE);

        m_Flags.set(flRestoreCursor, GetUICursor().IsVisible());

        if (!ReloadUI())
            return;

        m_Flags.set(flRestoreConsole, Console->bVisible);

        if (b_is_single)
            m_Flags.set(flRestorePause, Device.Paused());

        Console->Hide();

        if (b_is_single)
        {
            m_Flags.set(flRestorePauseStr, bShowPauseString);
            bShowPauseString = FALSE;
            if (!m_Flags.test(flRestorePause))
                Device.Pause(TRUE, TRUE, FALSE, "mm_activate2");
        }

        if (g_pGameLevel)
        {
            if (b_is_single)
            {
                Device.seqFrame.Remove(g_pGameLevel);
            }
            Device.seqRender.Remove(g_pGameLevel);
            CCameraManager::ResetPP();
        };
        Device.seqRender.Add(this, 4); // 1-console 2-cursor 3-tutorial

        Console->Execute("stat_memory");
    }
    else
    {
        m_deactivated_frame = Device.dwFrame;
        m_Flags.set(flActive, FALSE);
        m_Flags.set(flNeedChangeCapture, TRUE);

        Device.seqRender.Remove(this);

        bool b = !!Console->bVisible;
        if (b)
        {
            Console->Hide();
        }

        IR_Release();
        if (b)
        {
            Console->Show();
        }

        if (m_startDialog->IsShown())
            m_startDialog->HideDialog();

        CleanInternals();
        if (g_pGameLevel)
        {
            if (b_is_single)
            {
                Device.seqFrame.Add(g_pGameLevel);
            }
            Device.seqRender.Add(g_pGameLevel);
        };
        if (m_Flags.test(flRestoreConsole))
            Console->Show();

        if (b_is_single)
        {
            if (!m_Flags.test(flRestorePause))
                Device.Pause(FALSE, TRUE, FALSE, "mm_deactivate1");

            bShowPauseString = m_Flags.test(flRestorePauseStr);
        }

        if (m_Flags.test(flRestoreCursor))
            GetUICursor().Show();

        Device.Pause(FALSE, TRUE, TRUE, "mm_deactivate2");

        /* Xottab_DUTY:

           Original code does Device.Reset() twice
           if we are in main menu and game level exist

           The first time is normal reset
           The second one happens when 
           we are closing main menu

           Probable reason is that level needs to be precached

           I changed Mixed and Debug to do precache only
           instead of resetting entire Device

           XXX: find any problems that may happen
        */
        if (m_Flags.test(flNeedVidRestart))
        {
            m_Flags.set(flNeedVidRestart, FALSE);
#ifdef NDEBUG
            Device.Reset();
#else
            // Do only a precache for Debug and Mixed
            Device.PreCache(20, true, false);
#endif
        }
    }
}

bool CMainMenu::ReloadUI()
{
    if (m_startDialog)
    {
        if (m_startDialog->IsShown())
            m_startDialog->HideDialog();
        CleanInternals();
    }
    IFactoryObject* dlg = NEW_INSTANCE(TEXT2CLSID("MAIN_MNU"));
    if (!dlg)
    {
        m_Flags.set(flActive | flNeedChangeCapture, FALSE);
        return false;
    }
    xr_delete(m_startDialog);
    m_startDialog = smart_cast<CUIDialogWnd*>(dlg);
    VERIFY(m_startDialog);
    m_startDialog->m_bWorkInPause = true;
    m_startDialog->ShowDialog(true);
    return true;
}

bool CMainMenu::IsActive() const { return m_Flags.test(flActive); }
bool CMainMenu::CanSkipSceneRendering() { return IsActive() && !m_Flags.test(flGameSaveScreenshot); }

// IInputReceiver
void CMainMenu::IR_OnMousePress(int btn)
{
    if (!IsActive())
        return;

    IR_OnKeyboardPress(MouseButtonToKey[btn]);
};

void CMainMenu::IR_OnMouseRelease(int btn)
{
    if (!IsActive())
        return;

    IR_OnKeyboardRelease(MouseButtonToKey[btn]);
};

void CMainMenu::IR_OnMouseHold(int btn)
{
    if (!IsActive())
        return;

    IR_OnKeyboardHold(MouseButtonToKey[btn]);
};

void CMainMenu::IR_OnMouseMove(int x, int y)
{
    if (!IsActive())
        return;
    CDialogHolder::IR_UIOnMouseMove(x, y);
};

void CMainMenu::IR_OnMouseStop(int x, int y){};

bool IWantMyMouseBackScreamed = false;
void CMainMenu::IR_OnKeyboardPress(int dik)
{
    if (!IsActive())
        return;

    if (IsBinded(kCONSOLE, dik))
    {
        Console->Show();
        return;
    }

    if ((pInput->iGetAsyncKeyState(SDL_SCANCODE_LALT) || pInput->iGetAsyncKeyState(SDL_SCANCODE_RALT))
        && (pInput->iGetAsyncKeyState(SDL_SCANCODE_LGUI) || pInput->iGetAsyncKeyState(SDL_SCANCODE_RGUI)))
    {
        IWantMyMouseBackScreamed = true;
        pInput->GrabInput(false);
        Device.AllowWindowDrag = true;
#if SDL_VERSION_ATLEAST(2,0,5)
        SDL_SetWindowOpacity(Device.m_sdlWnd, 0.9f);
#endif
    }

    if (SDL_SCANCODE_F12 == dik)
    {
        GEnv.Render->Screenshot();
        return;
    }

    CDialogHolder::IR_UIOnKeyboardPress(dik);
};

void CMainMenu::IR_OnKeyboardRelease(int dik)
{
    if (!IsActive())
        return;

    if (IWantMyMouseBackScreamed)
    {
        IWantMyMouseBackScreamed = false;
        pInput->GrabInput(true);
        Device.AllowWindowDrag = false;
#if SDL_VERSION_ATLEAST(2,0,5)
        SDL_SetWindowOpacity(Device.m_sdlWnd, 1.f);
#endif
    }


    CDialogHolder::IR_UIOnKeyboardRelease(dik);
};

void CMainMenu::IR_OnKeyboardHold(int dik)
{
    if (!IsActive())
        return;

    CDialogHolder::IR_UIOnKeyboardHold(dik);
};

void CMainMenu::IR_OnTextInput(pcstr text)
{
    if (!IsActive())
        return;

    CDialogHolder::IR_UIOnTextInput(text);
}

void CMainMenu::IR_OnMouseWheel(int x, int y)
{
    if (!IsActive())
        return;

    CDialogHolder::IR_UIOnMouseWheel(x, y);
}

void CMainMenu::IR_OnControllerPress(int btn)
{
    if (!IsActive())
        return;

    IR_OnKeyboardPress(ControllerButtonToKey[btn]);
}

void CMainMenu::IR_OnControllerRelease(int btn)
{
    if (!IsActive())
        return;

    IR_OnKeyboardRelease(ControllerButtonToKey[btn]);
}

bool CMainMenu::OnRenderPPUI_query() { return IsActive() && !m_Flags.test(flGameSaveScreenshot) && b_shniaganeed_pp; }
extern void draw_wnds_rects();
void CMainMenu::OnRender()
{
    if (m_Flags.test(flGameSaveScreenshot))
        return;

    if (g_pGameLevel)
        GEnv.Render->Calculate();

    GEnv.Render->Render();
    if (!OnRenderPPUI_query())
    {
        DoRenderDialogs();
        UI().RenderFont();
        draw_wnds_rects();
    }
}

void CMainMenu::OnRenderPPUI_main()
{
    if (!IsActive())
        return;

    if (m_Flags.test(flGameSaveScreenshot))
        return;

    UI().pp_start();

    if (OnRenderPPUI_query())
    {
        DoRenderDialogs();
        UI().RenderFont();
    }

    UI().pp_stop();
}

void CMainMenu::OnRenderPPUI_PP()
{
    if (!IsActive())
        return;

    if (m_Flags.test(flGameSaveScreenshot))
        return;

    UI().pp_start();

    xr_vector<CUIWindow*>::iterator it = m_pp_draw_wnds.begin();
    for (; it != m_pp_draw_wnds.end(); ++it)
    {
        (*it)->Draw();
    }
    UI().pp_stop();
}
/*
void CMainMenu::StartStopMenu(CUIDialogWnd* pDialog, bool bDoHideIndicators)
{
    pDialog->m_bWorkInPause = true;
    CDialogHolder::StartStopMenu(pDialog, bDoHideIndicators);
}*/

// pureFrame
void CMainMenu::OnFrame()
{
    if (m_Flags.test(flNeedChangeCapture))
    {
        m_Flags.set(flNeedChangeCapture, FALSE);
        if (m_Flags.test(flActive))
            IR_Capture();
        else
            IR_Release();
    }
    CDialogHolder::OnFrame();

    // screenshot stuff
    if (m_Flags.test(flGameSaveScreenshot) && Device.dwFrame > m_screenshotFrame)
    {
        m_Flags.set(flGameSaveScreenshot, FALSE);
        GEnv.Render->Screenshot(IRender::SM_FOR_GAMESAVE, m_screenshot_name);

        if (g_pGameLevel && m_Flags.test(flActive))
        {
            Device.seqFrame.Remove(g_pGameLevel);
            Device.seqRender.Remove(g_pGameLevel);
        };

        if (m_Flags.test(flRestoreConsole))
            Console->Show();
    }

#ifdef XR_PLATFORM_WINDOWS
    if (IsActive() || m_sPDProgress.IsInProgress)
    {
        GSUpdateStatus status = m_pGameSpyFull->Update();
        if (status != GSUpdateStatus::ConnectingToMaster)
            Hide_CTMS_Dialog();
        switch (status)
        {
        case GSUpdateStatus::MasterUnreachable:
        case GSUpdateStatus::Unknown: SetErrorDialog(ErrMasterServerConnectFailed); break;
        case GSUpdateStatus::OutOfService: SetErrorDialog(ErrGSServiceFailed); break;
        }
        m_atlas_submit_queue->update();
    }
#endif

    if (IsActive())
    {
        CheckForErrorDlg();
        if (m_wasForceReloaded)
        {
            m_wasForceReloaded = false;
            m_startDialog->SendMessage(m_startDialog, MAIN_MENU_RELOADED, NULL);
        }
    }
}

void CMainMenu::OnDeviceCreate() {}
void CMainMenu::Screenshot(IRender::ScreenshotMode mode, LPCSTR name)
{
    if (mode != IRender::SM_FOR_GAMESAVE)
    {
        GEnv.Render->Screenshot(mode, name);
    }
    else
    {
        m_Flags.set(flGameSaveScreenshot, TRUE);
        xr_strcpy(m_screenshot_name, name);
        if (g_pGameLevel && m_Flags.test(flActive))
        {
            Device.seqFrame.Add(g_pGameLevel);
            Device.seqRender.Add(g_pGameLevel);
        };
        m_screenshotFrame = Device.dwFrame + 1;
        m_Flags.set(flRestoreConsole, Console->bVisible);
        Console->Hide();
    }
}

void CMainMenu::RegisterPPDraw(CUIWindow* w)
{
    UnregisterPPDraw(w);
    m_pp_draw_wnds.push_back(w);
}

void CMainMenu::UnregisterPPDraw(CUIWindow* w)
{
    m_pp_draw_wnds.erase(std::remove(m_pp_draw_wnds.begin(), m_pp_draw_wnds.end(), w), m_pp_draw_wnds.end());
}

void CMainMenu::SetErrorDialog(EErrorDlg ErrDlg) { m_NeedErrDialog = ErrDlg; };
void CMainMenu::CheckForErrorDlg()
{
    if (m_NeedErrDialog == ErrNoError)
        return;
    m_pMB_ErrDlgs[m_NeedErrDialog]->ShowDialog(false);
    m_NeedErrDialog = ErrNoError;
};

void CMainMenu::SwitchToMultiplayerMenu() { m_startDialog->Dispatch(2, 1); };
void CMainMenu::DestroyInternal(bool bForce)
{
    if (m_startDialog && ((m_deactivated_frame < Device.dwFrame + 4) || bForce))
        xr_delete(m_startDialog);
}

void CMainMenu::OnPatchCheck(bool success, LPCSTR VersionName, LPCSTR URL)
{
    if (!success)
    {
        m_pMB_ErrDlgs[NoNewPatch]->ShowDialog(false);
        return;
    }
    if (m_sPDProgress.IsInProgress)
        return;

    if (m_pMB_ErrDlgs[NewPatchFound])
    {
        delete_data(m_pMB_ErrDlgs[NewPatchFound]);
        m_pMB_ErrDlgs[NewPatchFound] = NULL;
    }
    if (!m_pMB_ErrDlgs[NewPatchFound])
    {
        m_pMB_ErrDlgs[NewPatchFound] = xr_new<CUIMessageBoxEx>();
        m_pMB_ErrDlgs[NewPatchFound]->InitMessageBox("msg_box_new_patch");

        shared_str tmpText;
        tmpText.printf(m_pMB_ErrDlgs[NewPatchFound]->GetText(), VersionName, URL);
        m_pMB_ErrDlgs[NewPatchFound]->SetText(*tmpText);
    }
    m_sPatchURL = URL;

    Register(m_pMB_ErrDlgs[NewPatchFound]);
    m_pMB_ErrDlgs[NewPatchFound]->AddCallbackStr(
        "button_yes", MESSAGE_BOX_YES_CLICKED, CUIWndCallback::void_function(this, &CMainMenu::OnDownloadPatch));
    m_pMB_ErrDlgs[NewPatchFound]->ShowDialog(false);
};

void CMainMenu::OnDownloadPatch(CUIWindow*, void*)
{
#ifdef XR_PLATFORM_WINDOWS
    CGameSpy_Available GSA;
    shared_str result_string;
    if (!GSA.CheckAvailableServices(result_string))
    {
        Msg(*result_string);
        return;
    };

    LPCSTR fileName = *m_sPatchURL;
    if (!fileName)
        return;

    string4096 FilePath = "";
    char* FileName = NULL;
    GetFullPathName(fileName, 4096, FilePath, &FileName);
    string_path fname;
    if (FS.path_exist("$downloads$"))
    {
        FS.update_path(fname, "$downloads$", FileName);
        m_sPatchFileName = fname;
    }
    else
        m_sPatchFileName.printf("downloads" DELIMITER "%s", FileName);

    m_sPDProgress.IsInProgress = true;
    m_sPDProgress.Progress = 0;
    m_sPDProgress.FileName = m_sPatchFileName;
    m_sPDProgress.Status = "";
    CGameSpy_HTTP::CompletionCallback completionCallback;
    completionCallback.bind(this, &CMainMenu::OnDownloadPatchResult);
    CGameSpy_HTTP::ProgressCallback progressCallback;
    progressCallback.bind(this, &CMainMenu::OnDownloadPatchProgress);
    m_pGameSpyFull->GetGameSpyHTTP()->DownloadFile(
        *m_sPatchURL, *m_sPatchFileName, completionCallback, progressCallback);
#endif
}

void CMainMenu::OnDownloadPatchResult(bool success)
{
    m_sPDProgress.IsInProgress = false;
    auto dialogId = success ? PatchDownloadSuccess : PatchDownloadError;
    m_pMB_ErrDlgs[dialogId]->ShowDialog(false);
};

void CMainMenu::OnSessionTerminate(LPCSTR reason)
{
    if (m_NeedErrDialog == SessionTerminate && (Device.dwTimeGlobal - m_start_time) < 8000)
        return;

    m_start_time = Device.dwTimeGlobal;
    LPCSTR str = StringTable().translate("ui_st_kicked_by_server").c_str();
    pstr text;

    if (reason && xr_strlen(reason) && reason[0] == '@')
    {
        STRCONCAT(text, reason + 1);
    }
    else
    {
        STRCONCAT(text, str, " ", reason);
    }

    m_pMB_ErrDlgs[SessionTerminate]->SetText(StringTable().translate(text).c_str());
    SetErrorDialog(CMainMenu::SessionTerminate);
}

void CMainMenu::OnLoadError(LPCSTR module)
{
    LPCSTR str = StringTable().translate("ui_st_error_loading").c_str();
    string1024 Text;
    strconcat(sizeof(Text), Text, str, " ");
    xr_strcat(Text, sizeof(Text), module);
    m_pMB_ErrDlgs[LoadingError]->SetText(Text);
    SetErrorDialog(CMainMenu::LoadingError);
}

void CMainMenu::OnDownloadPatchProgress(u64 bytesReceived, u64 totalSize)
{
    m_sPDProgress.Progress = (float(bytesReceived) / float(totalSize)) * 100.0f;
};

extern ENGINE_API string512 g_sLaunchOnExit_app;
extern ENGINE_API string512 g_sLaunchOnExit_params;
extern ENGINE_API string_path g_sLaunchWorkingFolder;
void CMainMenu::OnRunDownloadedPatch(CUIWindow*, void*)
{
    xr_strcpy(g_sLaunchOnExit_app, *m_sPatchFileName);
    xr_strcpy(g_sLaunchOnExit_params, "");
    xr_strcpy(g_sLaunchWorkingFolder, "");
    Console->Execute("quit");
}

void CMainMenu::CancelDownload()
{
#ifdef XR_PLATFORM_WINDOWS
    m_pGameSpyFull->GetGameSpyHTTP()->StopDownload();
    m_sPDProgress.IsInProgress = false;
#endif
}

void CMainMenu::SetNeedVidRestart() { m_Flags.set(flNeedVidRestart, TRUE); }
void CMainMenu::OnDeviceReset()
{
    if (IsActive() && g_pGameLevel)
        SetNeedVidRestart();
}

void CMainMenu::OnUIReset()
{
    const bool main_menu_is_active = IsActive();
    VERIFY2(main_menu_is_active, "Trying to reload main menu while it's inactive. That's unsupported.");
    if (!main_menu_is_active)
        return;
    ReloadUI();
    m_wasForceReloaded = true;
}

// -------------------------------------------------------------------------------------------------

LPCSTR AddHyphens(LPCSTR c)
{
    static string64 buf;

    u32 sz = xr_strlen(c);
    u32 j = 0;

    for (u32 i = 1; i <= 3; ++i)
    {
        buf[i * 5 - 1] = '-';
    }

    for (u32 i = 0; i < sz; ++i)
    {
        j = i + iFloor(i / 4.0f);
        buf[j] = c[i];
    }
    buf[sz + iFloor(sz / 4.0f)] = 0;

    return buf;
}

LPCSTR DelHyphens(LPCSTR c)
{
    static string64 buf;

    u32 sz = xr_strlen(c);
    u32 sz1 = _min(iFloor(sz / 4.0f), 3);

    u32 j = 0;
    for (u32 i = 0; i < sz - sz1; ++i)
    {
        j = i + iFloor(i / 4.0f);
        buf[i] = c[j];
    }
    buf[sz - sz1] = 0;

    return buf;
}

// extern	int VerifyClientCheck(const char *key, unsigned short cskey);

bool CMainMenu::IsCDKeyIsValid()
{
#ifdef XR_PLATFORM_WINDOWS
    if (!m_pGameSpyFull || !m_pGameSpyFull->GetGameSpyHTTP())
        return false;
    string64 CDKey = "";
    GetCDKey_FromRegistry(CDKey);

#ifndef DEMO_BUILD
    if (!xr_strlen(CDKey))
        return true;
#endif

    int GameID = 0;
    for (int i = 0; i < 4; i++)
    {
        GetGameID(&GameID, i);
        if (VerifyClientCheck(CDKey, (unsigned short)(GameID)) == 1)
            return true;
    };
    return false;
#else
    return true;
#endif
}

bool CMainMenu::ValidateCDKey()
{
    if (IsCDKeyIsValid())
        return true;
    SetErrorDialog(CMainMenu::ErrCDKeyInvalid);
    return false;
}

void CMainMenu::Show_CTMS_Dialog()
{
    if (!m_pMB_ErrDlgs[ConnectToMasterServer])
        return;
    if (m_pMB_ErrDlgs[ConnectToMasterServer]->IsShown())
        return;
    m_pMB_ErrDlgs[ConnectToMasterServer]->ShowDialog(false);
}

void CMainMenu::Hide_CTMS_Dialog()
{
    if (!m_pMB_ErrDlgs[ConnectToMasterServer])
        return;
    if (!m_pMB_ErrDlgs[ConnectToMasterServer]->IsShown())
        return;
    m_pMB_ErrDlgs[ConnectToMasterServer]->HideDialog();
}

void CMainMenu::OnConnectToMasterServerOkClicked(CUIWindow*, void*) { Hide_CTMS_Dialog(); }
LPCSTR CMainMenu::GetGSVer()
{
    static string256 buff;
    xr_strcpy(buff, GetGameVersion());
    return buff;
}

LPCSTR CMainMenu::GetPlayerName()
{
#ifdef XR_PLATFORM_WINDOWS
    gamespy_gp::login_manager* l_mngr = GetLoginMngr();
    gamespy_gp::profile const* tmp_prof = l_mngr ? l_mngr->get_current_profile() : NULL;

    if (tmp_prof)
    {
        m_player_name = tmp_prof->unique_nick();
    }
    else
#endif
    {
        string512 name;
        GetPlayerName_FromRegistry(name, sizeof(name));
        m_player_name = name;
    }
    return m_player_name.c_str();
}

LPCSTR CMainMenu::GetCDKeyFromRegistry()
{
    string512 key = { 0 };
    GetCDKey_FromRegistry(key);
    m_cdkey._set(key);
    return m_cdkey.c_str();
}

void CMainMenu::Show_DownloadMPMap(LPCSTR text, LPCSTR url)
{
    m_downloaded_mp_map_url._set(url);

    CUIMessageBoxEx* downloadMsg = m_pMB_ErrDlgs[DownloadMPMap];
    if (downloadMsg)
    {
        m_pMB_ErrDlgs[DownloadMPMap]->SetText(text);
        m_pMB_ErrDlgs[DownloadMPMap]->SetTextEditURL(url);

        m_pMB_ErrDlgs[DownloadMPMap]->ShowDialog(false);
    }
    else
    {
        OnDownloadMPMap(nullptr, nullptr);
    }
}

void CMainMenu::OnDownloadMPMap_CopyURL(CUIWindow* w, void* d)
{
    LPCSTR url = m_downloaded_mp_map_url.c_str();
    os_clipboard::copy_to_clipboard(url);
}

void CMainMenu::OnDownloadMPMap(CUIWindow* w, void* d)
{
    LPCSTR url = m_downloaded_mp_map_url.c_str();
#ifdef XR_PLATFORM_WINDOWS
    LPCSTR params = NULL;
    STRCONCAT(params, "/C start ", url);
    ShellExecute(0, "open", "cmd.exe", params, NULL, SW_SHOW);
#else
    std::string command = "xdg-open " + std::string{url};
    system(command.c_str());
#endif
}

demo_info const* CMainMenu::GetDemoInfo(LPCSTR file_name)
{
    if (!m_demo_info_loader)
    {
        m_demo_info_loader = xr_new<demo_info_loader>();
    }
    return m_demo_info_loader->get_demofile_info(file_name);
}
