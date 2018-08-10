// File:		UIMessagesWindow.h
// Description:	Window with MP chat and Game Log ( with PDA messages in single and Kill Messages in MP)
// Created:		22.04.2005
// Author:		Serge Vynnychenko
// Mail:		narrator@gsc-game.kiev.ua
//
// Copyright 2005 GSC Game World

#pragma once

#include "xrUICore/Windows/UIWindow.h"

class CUIGameLog;
class CUIChatWnd;
struct GAME_NEWS_DATA;
struct KillMessageStruct;

class CUIMessagesWindow : public CUIWindow
{
public:
    CUIMessagesWindow();
    virtual ~CUIMessagesWindow();

    void AddIconedPdaMessage(GAME_NEWS_DATA* news);

    void AddLogMessage(const shared_str& msg);
    void AddLogMessage(KillMessageStruct& msg);
    void AddChatMessage(shared_str msg, shared_str author);
    //.	void				SetChatOwner					(game_cl_GameState* owner);
    void PendingMode(bool const is_in_pending_mode);
    CUIChatWnd* GetChatWnd() { return m_pChatWnd; }
    virtual void Show(bool show);

protected:
    virtual void Init(float x, float y, float width, float height);

    CUIGameLog* m_pChatLog;
    CUIChatWnd* m_pChatWnd;
    CUIGameLog* m_pGameLog;
    bool m_in_pending_mode;

    Frect m_pending_chat_log_rect;
    Frect m_inprogress_chat_log_rect;
};
