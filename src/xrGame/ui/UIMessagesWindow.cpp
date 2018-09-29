// File:		UIMessagesWindow.h
// Description:	Window with MP chat and Game Log ( with PDA messages in single and Kill Messages in MP)
// Created:		22.04.2005
// Author:		Serge Vynnychenko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#include "StdAfx.h"
#include "UIMessagesWindow.h"
#include "UIGameLog.h"
#include "UIChatWnd.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "UIInventoryUtilities.h"
#include "game_news.h"
#include "UIPdaMsgListItem.h"
#include "xrGame/game_type.h"

CUIMessagesWindow::CUIMessagesWindow() : m_pChatLog(NULL), m_pChatWnd(NULL), m_pGameLog(NULL)
{
    Init(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);
}

CUIMessagesWindow::~CUIMessagesWindow() {}
void CUIMessagesWindow::AddLogMessage(KillMessageStruct& msg) { m_pGameLog->AddLogMessage(msg); }
void CUIMessagesWindow::AddLogMessage(const shared_str& msg) { m_pGameLog->AddLogMessage(*msg); }
void CUIMessagesWindow::PendingMode(bool const is_pending_mode)
{
    if (is_pending_mode)
    {
        if (m_in_pending_mode)
            return;

        m_pChatWnd->PendingMode(is_pending_mode);
        m_pChatLog->SetWndRect(m_pending_chat_log_rect);
        m_in_pending_mode = true;
        return;
    }
    if (!m_in_pending_mode)
        return;

    m_pChatWnd->PendingMode(is_pending_mode);
    m_pChatLog->SetWndRect(m_inprogress_chat_log_rect);
    m_in_pending_mode = false;
}

#define CHAT_LOG_LIST_PENDING "chat_log_list_pending"
void CUIMessagesWindow::Init(float x, float y, float width, float height)
{
    CUIXml xml;
    xml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "messages_window.xml");
    m_pGameLog = new CUIGameLog();
    m_pGameLog->SetAutoDelete(true);
    m_pGameLog->Show(true);
    AttachChild(m_pGameLog);
    if (IsGameTypeSingle())
    {
        CUIXmlInit::InitScrollView(xml, "sp_log_list", 0, m_pGameLog);
    }
    else
    {
        u32 color;
        CGameFont* pFont;

        m_pChatLog = new CUIGameLog();
        m_pChatLog->SetAutoDelete(true);
        m_pChatLog->Show(true);
        AttachChild(m_pChatLog);
        m_pChatWnd = new CUIChatWnd();
        m_pChatWnd->SetAutoDelete(true);
        AttachChild(m_pChatWnd);

        CUIXmlInit::InitScrollView(xml, "mp_log_list", 0, m_pGameLog);
        CUIXmlInit::InitFont(xml, "mp_log_list:font", 0, color, pFont);
        m_pGameLog->SetTextAtrib(pFont, color);

        CUIXmlInit::InitScrollView(xml, "chat_log_list", 0, m_pChatLog);
        m_inprogress_chat_log_rect = m_pChatLog->GetWndRect();

        m_in_pending_mode = false;

        XML_NODE pending_chat_list = xml.NavigateToNode(CHAT_LOG_LIST_PENDING);

        if (pending_chat_list)
        {
            m_pending_chat_log_rect.x1 = xml.ReadAttribFlt(CHAT_LOG_LIST_PENDING, 0, "x");
            m_pending_chat_log_rect.y1 = xml.ReadAttribFlt(CHAT_LOG_LIST_PENDING, 0, "y");
            m_pending_chat_log_rect.x2 = xml.ReadAttribFlt(CHAT_LOG_LIST_PENDING, 0, "width");
            m_pending_chat_log_rect.y2 = xml.ReadAttribFlt(CHAT_LOG_LIST_PENDING, 0, "height");
            m_pending_chat_log_rect.rb.add(m_pending_chat_log_rect.lt);
        }
        else
            m_pending_chat_log_rect = m_inprogress_chat_log_rect;

        CUIXmlInit::InitFont(xml, "chat_log_list:font", 0, color, pFont);
        m_pChatLog->SetTextAtrib(pFont, color);

        m_pChatWnd->Init(xml);
    }
}

void CUIMessagesWindow::AddIconedPdaMessage(GAME_NEWS_DATA* news)
{
    CUIPdaMsgListItem* pItem = m_pGameLog->AddPdaMessage();

    LPCSTR time_str =
        InventoryUtilities::GetTimeAsString(news->receive_time, InventoryUtilities::etpTimeToMinutes).c_str();
    pItem->UITimeText.SetText(time_str);
    pItem->UITimeText.AdjustWidthToText();
    Fvector2 p = pItem->UICaptionText.GetWndPos();
    p.x = pItem->UITimeText.GetWndPos().x + pItem->UITimeText.GetWidth() + 3.0f;
    pItem->UICaptionText.SetWndPos(p);
    pItem->UICaptionText.SetTextST(news->news_caption.c_str());
    pItem->UIMsgText.SetTextST(news->news_text.c_str());
    pItem->UIMsgText.AdjustHeightToText();

    pItem->SetColorAnimation(
        "ui_main_msgs_short", LA_ONLYALPHA | LA_TEXTCOLOR | LA_TEXTURECOLOR, float(news->show_time));
    pItem->UIIcon.InitTexture(news->texture_name.c_str());

    float h1 = _max(pItem->UIIcon.GetHeight(), pItem->UIMsgText.GetWndPos().y + pItem->UIMsgText.GetHeight());
    pItem->SetHeight(h1 + 3.0f);

    m_pGameLog->SendMessage(pItem, CHILD_CHANGED_SIZE);
}

void CUIMessagesWindow::AddChatMessage(shared_str msg, shared_str author) { m_pChatLog->AddChatMessage(*msg, *author); }
/*
void CUIMessagesWindow::SetChatOwner(game_cl_GameState* owner)
{
    if (m_pChatWnd)
        m_pChatWnd->SetOwner(owner);
}
*/
void CUIMessagesWindow::Show(bool show)
{
    if (m_pChatWnd)
        m_pChatWnd->Show(show);
    if (m_pGameLog)
        m_pGameLog->Show(show);
    if (m_pChatLog)
        m_pChatLog->Show(show);
}
