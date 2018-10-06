#include <StdAfx.h>
#include "UIChatWnd.h"
#include "UIGameLog.h"
#include "xrUICore/EditBox/UIEditBox.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIDialogHolder.h"
#include "game_cl_base.h"
#include "xr_level_controller.h"
#include "Level.h"

CUIChatWnd::CUIChatWnd() : sendNextMessageToAll(true) {}
void CUIChatWnd::PendingMode(bool const is_pending_mode)
{
    if (is_pending_mode)
    {
        if (pendingGameMode)
            return;

        UIPrefix->SetWndRect(pending_prefix_rect);
        UIEditBox->SetWndRect(pending_edit_rect);
        pendingGameMode = true;
        return;
    }
    if (!pendingGameMode)
        return;

    UIPrefix->SetWndRect(inprogress_prefix_rect);
    UIEditBox->SetWndRect(inprogress_edit_rect);
    pendingGameMode = false;
}

const pcstr CHAT_PREFIX_PENDING = "chat_prefix_pending";
const pcstr CHAT_EDITBOX_PENDING = "chat_editbox_pending";

void CUIChatWnd::Init(CUIXml& uiXml)
{
    UIPrefix = UIHelper::CreateTextWnd(uiXml, "chat_prefix", this);
    inprogress_prefix_rect = UIPrefix->GetWndRect();

    UIEditBox = UIHelper::CreateEditBox(uiXml, "chat_edit_box", this);
    inprogress_edit_rect = UIEditBox->GetWndRect();
    UIEditBox->SetWindowName("chat_edit_box");

    pendingGameMode = false;

    R_ASSERT(uiXml.NavigateToNode(CHAT_PREFIX_PENDING));
    R_ASSERT(uiXml.NavigateToNode(CHAT_EDITBOX_PENDING));

    pending_prefix_rect.x1 = uiXml.ReadAttribFlt(CHAT_PREFIX_PENDING, 0, "x");
    pending_prefix_rect.y1 = uiXml.ReadAttribFlt(CHAT_PREFIX_PENDING, 0, "y");
    pending_prefix_rect.x2 = uiXml.ReadAttribFlt(CHAT_PREFIX_PENDING, 0, "width");
    pending_prefix_rect.y2 = uiXml.ReadAttribFlt(CHAT_PREFIX_PENDING, 0, "height");
    pending_prefix_rect.rb.add(pending_prefix_rect.lt);

    pending_edit_rect.x1 = uiXml.ReadAttribFlt(CHAT_EDITBOX_PENDING, 0, "x");
    pending_edit_rect.y1 = uiXml.ReadAttribFlt(CHAT_EDITBOX_PENDING, 0, "y");
    pending_edit_rect.x2 = uiXml.ReadAttribFlt(CHAT_EDITBOX_PENDING, 0, "width");
    pending_edit_rect.y2 = uiXml.ReadAttribFlt(CHAT_EDITBOX_PENDING, 0, "height");
    pending_edit_rect.rb.add(pending_edit_rect.lt);

    Register(UIEditBox);
    AddCallback(UIEditBox, EDIT_TEXT_COMMIT, CUIWndCallback::void_function(this, &CUIChatWnd::OnChatCommit));
    AddCallback(UIEditBox, EDIT_TEXT_CANCEL, CUIWndCallback::void_function(this, &CUIChatWnd::OnChatCancel));
}

void CUIChatWnd::SetEditBoxPrefix(LPCSTR prefix)
{
    UIPrefix->SetText(prefix);
    UIPrefix->AdjustWidthToText();
    Fvector2 _pos;
    _pos.x = UIPrefix->GetWndPos().x + UIPrefix->GetWidth() + 5.0f;
    _pos.y = UIEditBox->GetWndPos().y;
    UIEditBox->SetWndPos(_pos);
    UIEditBox->ClearText();
}

void CUIChatWnd::Show(bool status)
{
    UIEditBox->CaptureFocus(status);
    inherited::Show(status);
}

void CUIChatWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData) { CUIWndCallback::OnEvent(pWnd, msg, pData); }
void CUIChatWnd::OnChatCommit(CUIWindow* w, void* d)
{
    Game().ChatSay(UIEditBox->GetText(), sendNextMessageToAll);
    HideDialog();
}

void CUIChatWnd::OnChatCancel(CUIWindow* w, void* d) { HideDialog(); }
