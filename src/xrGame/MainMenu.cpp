#include "stdafx.h"
#include "MainMenu.h"
#include "UI/UIDialogWnd.h"
#include "ui/UIMessageBoxEx.h"
#include "xrEngine/xr_IOConsole.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/CameraManager.h"
#include "xr_Level_controller.h"
#include "ui\UITextureMaster.h"
#include "ui\UIXmlInit.h"
#include <dinput.h>
#include "ui\UIBtnHint.h"
#include "UICursor.h"
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

#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

#include "Common/object_broker.h"

#include "account_manager.h"
#include "login_manager.h"
#include "profile_store.h"
#include "stats_submitter.h"
#include "atlas_submit_queue.h"

// fwd. decl.
extern ENGINE_API BOOL bShowPauseString;

//#define DEMO_BUILD

string128 ErrMsgBoxTemplate[] = {"message_box_invalid_pass", "message_box_invalid_host", "message_box_session_full",
    "message_box_server_reject", "message_box_cdkey_in_use", "message_box_cdkey_disabled", "message_box_cdkey_invalid",
    "message_box_different_version", "message_box_gs_service_not_available",
    "message_box_sb_master_server_connect_failed", "msg_box_no_new_patch", "msg_box_new_patch",
    "msg_box_patch_download_error", "msg_box_patch_download_success", "msg_box_connect_to_master_server",
    "msg_box_kicked_by_server", "msg_box_error_loading", "message_box_download_level"};

extern bool b_shniaganeed_pp;

CMainMenu* MainMenu() { return (CMainMenu*)g_pGamePersistent->m_pMainMenu; };
//----------------------------------------------------------------------------------
#define INIT_MSGBOX(_box, _template)     \
    {                                    \
        _box = new CUIMessageBoxEx();    \
        _box->InitMessageBox(_template); \
    }
//----------------------------------------------------------------------------------

CMainMenu::CMainMenu()
{
    m_Flags.zero();
    m_startDialog = NULL;
    m_screenshotFrame = u32(-1);
    g_pGamePersistent->m_pMainMenu = this;
    if (Device.b_is_Ready)
        OnDeviceCreate();
    ReadTextureInfo();
    CUIXmlInit::InitColorDefs();
    g_btnHint = NULL;
    g_statHint = NULL;
    m_deactivated_frame = 0;

    m_sPatchURL = "";
    m_pGameSpyFull = NULL;
    m_account_mngr = NULL;
    m_login_mngr = NULL;
    m_profile_store = NULL;
    m_stats_submitter = NULL;
    m_atlas_submit_queue = NULL;

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
        g_btnHint = new CUIButtonHint();
        g_statHint = new CUIButtonHint();
        m_pGameSpyFull = new CGameSpy_Full();

        for (u32 i = 0; i < u32(ErrMax); i++)
        {
            CUIMessageBoxEx* pNewErrDlg;
            INIT_MSGBOX(pNewErrDlg, ErrMsgBoxTemplate[i]);
            m_pMB_ErrDlgs.push_back(pNewErrDlg);
        }

        m_pMB_ErrDlgs[PatchDownloadSuccess]->AddCallbackStr("button_yes", MESSAGE_BOX_YES_CLICKED,
            CUIWndCallback::void_function(this, &CMainMenu::OnRunDownloadedPatch));
        m_pMB_ErrDlgs[PatchDownloadSuccess]->AddCallbackStr("button_yes", MESSAGE_BOX_OK_CLICKED,
            CUIWndCallback::void_function(this, &CMainMenu::OnConnectToMasterServerOkClicked));

        m_pMB_ErrDlgs[DownloadMPMap]->AddCallbackStr("button_copy", MESSAGE_BOX_COPY_CLICKED,
            CUIWndCallback::void_function(this, &CMainMenu::OnDownloadMPMap_CopyURL));
        m_pMB_ErrDlgs[DownloadMPMap]->AddCallbackStr(
            "button_yes", MESSAGE_BOX_YES_CLICKED, CUIWndCallback::void_function(this, &CMainMenu::OnDownloadMPMap));

        m_account_mngr = new gamespy_gp::account_manager(m_pGameSpyFull->GetGameSpyGP());
        m_login_mngr = new gamespy_gp::login_manager(m_pGameSpyFull);
        m_profile_store = new gamespy_profile::profile_store(m_pGameSpyFull);
        m_stats_submitter = new gamespy_profile::stats_submitter(m_pGameSpyFull);
        m_atlas_submit_queue = new atlas_submit_queue(m_stats_submitter);
    }

    Device.seqFrame.Add(this, REG_PRIORITY_LOW - 1000);
}

CMainMenu::~CMainMenu()
{
    Device.seqFrame.Remove(this);
    xr_delete(g_btnHint);
    xr_delete(g_statHint);
    xr_delete(m_startDialog);
    g_pGamePersistent->m_pMainMenu = NULL;

    xr_delete(m_account_mngr);
    xr_delete(m_login_mngr);
    xr_delete(m_profile_store);
    xr_delete(m_stats_submitter);
    xr_delete(m_atlas_submit_queue);

    xr_delete(m_pGameSpyFull);

    xr_delete(m_demo_info_loader);
    delete_data(m_pMB_ErrDlgs);
}

void CMainMenu::ReadTextureInfo()
{
    string_path buf;
    FS_FileSet fset;
    FS.file_list(fset, "$game_config$", FS_ListFiles, strconcat(sizeof(buf), buf, UI_PATH, "\\", "textures_descr\\*.xml"));
    for (const auto& file : fset)
    {
        string_path fn1, fn2, fn3;
        _splitpath(file.name.c_str(), fn1, fn2, fn3, 0);
        xr_strcat(fn3, ".xml");

        CUITextureMaster::ParseShTexInfo(fn3);
    }
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

        if (m_Flags.test(flNeedVidRestart))
        {
            m_Flags.set(flNeedVidRestart, FALSE);
            Console->Execute("vid_restart");
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

    m_activatedScreenRatio = (float)Device.dwWidth / (float)Device.dwHeight > (UI_BASE_WIDTH / UI_BASE_HEIGHT + 0.01f);
    return true;
}

bool CMainMenu::IsActive() { return !!m_Flags.test(flActive); }
bool CMainMenu::CanSkipSceneRendering() { return IsActive() && !m_Flags.test(flGameSaveScreenshot); }
// IInputReceiver
static int mouse_button_2_key[] = {MOUSE_1, MOUSE_2, MOUSE_3};
void CMainMenu::IR_OnMousePress(int btn)
{
    if (!IsActive())
        return;

    IR_OnKeyboardPress(mouse_button_2_key[btn]);
};

void CMainMenu::IR_OnMouseRelease(int btn)
{
    if (!IsActive())
        return;

    IR_OnKeyboardRelease(mouse_button_2_key[btn]);
};

void CMainMenu::IR_OnMouseHold(int btn)
{
    if (!IsActive())
        return;

    IR_OnKeyboardHold(mouse_button_2_key[btn]);
};

void CMainMenu::IR_OnMouseMove(int x, int y)
{
    if (!IsActive())
        return;
    CDialogHolder::IR_UIOnMouseMove(x, y);
};

void CMainMenu::IR_OnMouseStop(int x, int y){};

void CMainMenu::IR_OnKeyboardPress(int dik)
{
    if (!IsActive())
        return;

    if (is_binded(kCONSOLE, dik))
    {
        Console->Show();
        return;
    }
    if (DIK_F12 == dik)
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

    CDialogHolder::IR_UIOnKeyboardRelease(dik);
};

void CMainMenu::IR_OnKeyboardHold(int dik)
{
    if (!IsActive())
        return;

    CDialogHolder::IR_UIOnKeyboardHold(dik);
};

void CMainMenu::IR_OnMouseWheel(int direction)
{
    if (!IsActive())
        return;

    CDialogHolder::IR_UIOnMouseWheel(direction);
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

    if (IsActive())
    {
        CheckForErrorDlg();
        bool b_is_16_9 = (float)Device.dwWidth / (float)Device.dwHeight > (UI_BASE_WIDTH / UI_BASE_HEIGHT + 0.01f);
        if (b_is_16_9 != m_activatedScreenRatio)
        {
            ReloadUI();
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
        INIT_MSGBOX(m_pMB_ErrDlgs[NewPatchFound], "msg_box_new_patch");

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
        m_sPatchFileName.printf("downloads\\%s", FileName);

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
    CStringTable st;
    LPCSTR str = st.translate("ui_st_kicked_by_server").c_str();
    LPSTR text;

    if (reason && xr_strlen(reason) && reason[0] == '@')
    {
        STRCONCAT(text, reason + 1);
    }
    else
    {
        STRCONCAT(text, str, " ", reason);
    }

    m_pMB_ErrDlgs[SessionTerminate]->SetText(st.translate(text).c_str());
    SetErrorDialog(CMainMenu::SessionTerminate);
}

void CMainMenu::OnLoadError(LPCSTR module)
{
    LPCSTR str = CStringTable().translate("ui_st_error_loading").c_str();
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
    m_pGameSpyFull->GetGameSpyHTTP()->StopDownload();
    m_sPDProgress.IsInProgress = false;
}

void CMainMenu::SetNeedVidRestart() { m_Flags.set(flNeedVidRestart, TRUE); }
void CMainMenu::OnDeviceReset()
{
    if (IsActive() && g_pGameLevel)
        SetNeedVidRestart();
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
        if (VerifyClientCheck(CDKey, unsigned short(GameID)) == 1)
            return true;
    };
    return false;
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
    gamespy_gp::login_manager* l_mngr = GetLoginMngr();
    gamespy_gp::profile const* tmp_prof = l_mngr ? l_mngr->get_current_profile() : NULL;

    if (tmp_prof)
    {
        m_player_name = tmp_prof->unique_nick();
    }
    else
    {
        string512 name;
        GetPlayerName_FromRegistry(name, sizeof(name));
        m_player_name = name;
    }
    return m_player_name.c_str();
}

LPCSTR CMainMenu::GetCDKeyFromRegistry()
{
    string512 key;
    GetCDKey_FromRegistry(key);
    m_cdkey._set(key);
    return m_cdkey.c_str();
}

void CMainMenu::Show_DownloadMPMap(LPCSTR text, LPCSTR url)
{
    VERIFY(m_pMB_ErrDlgs[DownloadMPMap]);

    m_downloaded_mp_map_url._set(url);

    m_pMB_ErrDlgs[DownloadMPMap]->SetText(text);
    m_pMB_ErrDlgs[DownloadMPMap]->SetTextEditURL(url);

    m_pMB_ErrDlgs[DownloadMPMap]->ShowDialog(false);
}

void CMainMenu::OnDownloadMPMap_CopyURL(CUIWindow* w, void* d)
{
    LPCSTR url = m_downloaded_mp_map_url.c_str();
    os_clipboard::copy_to_clipboard(url);
}

void CMainMenu::OnDownloadMPMap(CUIWindow* w, void* d)
{
    LPCSTR url = m_downloaded_mp_map_url.c_str();
    LPCSTR params = NULL;
    STRCONCAT(params, "/C start ", url);
    ShellExecute(0, "open", "cmd.exe", params, NULL, SW_SHOW);
}

demo_info const* CMainMenu::GetDemoInfo(LPCSTR file_name)
{
    if (!m_demo_info_loader)
    {
        m_demo_info_loader = new demo_info_loader();
    }
    return m_demo_info_loader->get_demofile_info(file_name);
}
