#include "stdafx.h"
#include "UIMPAdminMenu.h"
#include "UIMPPlayersAdm.h"
#include "UIMPServerAdm.h"
#include "UIMPChangeMapAdm.h"
#include "UIXmlInit.h"
#include "Common/object_broker.h"
#include "UITabControl.h"
#include "UIStatic.h"
#include "UI3tButton.h"
#include "UIMessageBox.h"
#include "UIMessageBoxEx.h"
#include "xrEngine/xr_ioconsole.h"

CUIMpAdminMenu::CUIMpAdminMenu()
{
    xml_doc = NULL;
    m_pActiveDialog = NULL;
    m_sActiveSection = "";

    m_pBack = new CUIStatic();
    m_pBack->SetAutoDelete(true);
    AttachChild(m_pBack);

    m_pTabControl = new CUITabControl();
    m_pTabControl->SetAutoDelete(true);
    AttachChild(m_pTabControl);

    m_pPlayersAdm = new CUIMpPlayersAdm();
    m_pPlayersAdm->SetAutoDelete(false);

    m_pServerAdm = new CUIMpServerAdm();
    m_pServerAdm->SetAutoDelete(false);

    m_pChangeMapAdm = new CUIMpChangeMapAdm();
    m_pChangeMapAdm->SetAutoDelete(false);

    m_pClose = new CUI3tButton();
    m_pClose->SetAutoDelete(true);
    AttachChild(m_pClose);

    m_pMessageBoxLogin = new CUIMessageBoxEx();
    m_pMessageBoxOk = new CUIMessageBoxEx();
    Init();
}

CUIMpAdminMenu::~CUIMpAdminMenu()
{
    xr_delete(xml_doc);
    delete_data(m_pPlayersAdm);
    delete_data(m_pServerAdm);
    delete_data(m_pChangeMapAdm);
    delete_data(m_pMessageBoxLogin);
    delete_data(m_pMessageBoxOk);
}

void CUIMpAdminMenu::Init()
{
    if (!xml_doc)
        xml_doc = new CUIXml();

    xml_doc->Load(CONFIG_PATH, UI_PATH, "ui_mp_admin_menu.xml");

    CUIXmlInit::InitWindow(*xml_doc, "admin_menu", 0, this);
    CUIXmlInit::InitStatic(*xml_doc, "admin_menu:background", 0, m_pBack);
    CUIXmlInit::InitTabControl(*xml_doc, "admin_menu:tab_control", 0, m_pTabControl);
    m_pPlayersAdm->Init(*xml_doc);
    m_pServerAdm->Init(*xml_doc);
    m_pChangeMapAdm->Init(*xml_doc);
    m_pTabControl->SetActiveTab("players");
    SetActiveSubdialog("players");
    CUIXmlInit::Init3tButton(*xml_doc, "admin_menu:close_button", 0, m_pClose);

    m_pMessageBoxLogin->InitMessageBox("message_box_ra_login");
    m_pMessageBoxLogin->func_on_ok = CUIWndCallback::void_function(this, &CUIMpAdminMenu::RemoteAdminLogin);
    m_pMessageBoxOk->InitMessageBox("message_box_error");
}

void CUIMpAdminMenu::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    switch (msg)
    {
    case TAB_CHANGED:
    {
        if (pWnd == m_pTabControl)
            SetActiveSubdialog(m_pTabControl->GetActiveId());
        break;
    }
    case BUTTON_CLICKED:
    {
        if (pWnd == m_pClose)
            HideDialog();
        break;
    }
    default:
    {
        R_ASSERT(m_pActiveDialog);
        m_pActiveDialog->SendMessage(pWnd, msg, pData);
    }
    };
}
bool CUIMpAdminMenu::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (dik == SDL_SCANCODE_ESCAPE && keyboard_action == WINDOW_KEY_PRESSED)
    {
        if (m_pActiveDialog == m_pServerAdm && m_pServerAdm->IsBackBtnShown())
            m_pServerAdm->OnBackBtn();
        else
            HideDialog();

        return true;
    }

    return CUIDialogWnd::OnKeyboardAction(dik, keyboard_action);
}

void CUIMpAdminMenu::SetActiveSubdialog(const shared_str& section)
{
    if (m_sActiveSection == section)
        return;

    if (m_pActiveDialog)
    {
        DetachChild(m_pActiveDialog);
        m_pActiveDialog->Show(false);
    }

    if (section == "players")
        m_pActiveDialog = m_pPlayersAdm;
    else if (section == "server")
        m_pActiveDialog = m_pServerAdm;
    else if (section == "change_map")
        m_pActiveDialog = m_pChangeMapAdm;

    R_ASSERT(m_pActiveDialog);
    AttachChild(m_pActiveDialog);
    m_pActiveDialog->Show(true);
    m_sActiveSection = section;
}

void CUIMpAdminMenu::ShowMessageBox(CUIMessageBox::E_MESSAGEBOX_STYLE style, LPCSTR reason)
{
    switch (style)
    {
    case CUIMessageBox::MESSAGEBOX_RA_LOGIN: { m_pMessageBoxLogin->ShowDialog(true);
    }
    break;
    case CUIMessageBox::MESSAGEBOX_OK:
    {
        m_pMessageBoxOk->SetText(reason);
        m_pMessageBoxOk->ShowDialog(true);
    }
    break;
    }
}

void CUIMpAdminMenu::RemoteAdminLogin(CUIWindow*, void*)
{
    string512 tmp_string;
    xr_sprintf(tmp_string, "ra login %s %s", m_pMessageBoxLogin->m_pMessageBox->GetUserPassword(),
        m_pMessageBoxLogin->m_pMessageBox->GetPassword());
    Console->Execute(tmp_string);
}
