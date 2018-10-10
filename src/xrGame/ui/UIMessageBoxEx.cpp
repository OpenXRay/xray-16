#include "StdAfx.h"
#include "xrUICore/MessageBox/UIMessageBox.h"
#include "UIMessageBoxEx.h"
#include "UIDialogHolder.h"

CUIMessageBoxEx::CUIMessageBoxEx()
{
    m_pMessageBox = new CUIMessageBox();
    m_pMessageBox->SetWindowName("msg_box");
    //	m_pMessageBox->SetAutoDelete(true);
    AttachChild(m_pMessageBox);
}

CUIMessageBoxEx::~CUIMessageBoxEx() { xr_delete(m_pMessageBox); }
void CUIMessageBoxEx::InitMessageBox(LPCSTR xml_template)
{
    // CUIDialogWnd::SetWndRect(Frect().set(0.0f,0.0f,1024.0f,768.0f));
    m_pMessageBox->InitMessageBox(xml_template);

    SetWndPos(m_pMessageBox->GetWndPos());
    SetWndSize(m_pMessageBox->GetWndSize());
    m_pMessageBox->SetWndPos(Fvector2().set(0, 0));

    AddCallback(
        m_pMessageBox, MESSAGE_BOX_YES_CLICKED, CUIWndCallback::void_function(this, &CUIMessageBoxEx::OnOKClicked));
    CUIMessageBox::E_MESSAGEBOX_STYLE style = m_pMessageBox->GetBoxStyle();
    if (style == CUIMessageBox::MESSAGEBOX_YES_NO || style == CUIMessageBox::MESSAGEBOX_QUIT_WINDOWS ||
        style == CUIMessageBox::MESSAGEBOX_QUIT_GAME)
        AddCallback(
            m_pMessageBox, MESSAGE_BOX_NO_CLICKED, CUIWndCallback::void_function(this, &CUIMessageBoxEx::OnNOClicked));
}

void CUIMessageBoxEx::OnOKClicked(CUIWindow* w, void* d)
{
    if (!func_on_ok.empty())
    {
        func_on_ok(w, d);
    }
}

void CUIMessageBoxEx::OnNOClicked(CUIWindow* w, void* d)
{
    if (!func_on_no.empty())
    {
        func_on_no(w, d);
    }
}

void CUIMessageBoxEx::SetText(LPCSTR text) { m_pMessageBox->SetText(text); }
LPCSTR CUIMessageBoxEx::GetText() { return m_pMessageBox->GetText(); }
void CUIMessageBoxEx::SendMessage(CUIWindow* pWnd, s16 msg, void* pData /* = NULL */)
{
    CUIWndCallback::OnEvent(pWnd, msg, pData);
    if (pWnd == m_pMessageBox)
    {
        switch (msg)
        {
        case MESSAGE_BOX_OK_CLICKED:
        case MESSAGE_BOX_YES_CLICKED:
        case MESSAGE_BOX_NO_CLICKED:
        case MESSAGE_BOX_CANCEL_CLICKED:
        case MESSAGE_BOX_QUIT_WIN_CLICKED:
        case MESSAGE_BOX_QUIT_GAME_CLICKED: HideDialog();
        default: break;
        }

        if (GetMessageTarget())
            GetMessageTarget()->SendMessage(this, msg, pData);
    }
}

LPCSTR CUIMessageBoxEx::GetHost() { return m_pMessageBox->GetHost(); }
LPCSTR CUIMessageBoxEx::GetPassword() { return m_pMessageBox->GetPassword(); }
bool CUIMessageBoxEx::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (keyboard_action == WINDOW_KEY_PRESSED)
    {
        if (dik == SDL_SCANCODE_KP_ENTER || dik == SDL_SCANCODE_RETURN || dik == SDL_SCANCODE_SPACE)
        {
            m_pMessageBox->OnYesOk();
            return true;
            /*
                    }else
                        if ( dik == SDL_SCANCODE_ESCAPE )
                    {
                        CUIMessageBox::E_MESSAGEBOX_STYLE style = m_pMessageBox->GetBoxStyle();
                        if(style != CUIMessageBox::MESSAGEBOX_INFO)
                            HideDialog();
                        return true;
            */
        }
        else
            return CUIDialogWnd::OnKeyboardAction(dik, keyboard_action);
    }
    return CUIDialogWnd::OnKeyboardAction(dik, keyboard_action);
}

void CUIMessageBoxEx::SetTextEditURL(LPCSTR text) { m_pMessageBox->SetTextEditURL(text); }
LPCSTR CUIMessageBoxEx::GetTextEditURL() { return m_pMessageBox->GetTextEditURL(); }
