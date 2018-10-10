#include "pch_script.h"
#include "UIGameSP.h"
#include "Actor.h"
#include "Level.h"
#include "xrEngine/xr_input.h"
#include "xrNetServer/NET_Messages.h"

#ifdef DEBUG
#include "attachable_item.h"
#endif

#include "game_cl_single.h"
#include "xr_level_controller.h"
#include "ActorCondition.h"
#include "xrEngine/XR_IOConsole.h"
#include "Common/object_broker.h"
#include "GametaskManager.h"
#include "GameTask.h"

#include "ui/UIActorMenu.h"
#include "ui/UIPdaWnd.h"
#include "ui/UITalkWnd.h"
#include "xrUICore/MessageBox/UIMessageBox.h"

CUIGameSP::CUIGameSP() : m_game(NULL), m_game_objective(NULL)
{
    TalkMenu = new CUITalkWnd();
    UIChangeLevelWnd = new CChangeLevelWnd();
}

CUIGameSP::~CUIGameSP()
{
    delete_data(TalkMenu);
    delete_data(UIChangeLevelWnd);
}

void CUIGameSP::HideShownDialogs()
{
    HideActorMenu();
    HidePdaMenu();
    CUIDialogWnd* mir = TopInputReceiver();
    if (mir && mir == TalkMenu)
    {
        mir->HideDialog();
    }
}

void CUIGameSP::SetClGame(game_cl_GameState* g)
{
    inherited::SetClGame(g);
    m_game = smart_cast<game_cl_Single*>(g);
    R_ASSERT(m_game);
}
#ifdef DEBUG
void attach_adjust_mode_keyb(int dik);
void attach_draw_adjust_mode();
void hud_adjust_mode_keyb(int dik);
void hud_draw_adjust_mode();
#endif

void CUIGameSP::OnFrame()
{
    inherited::OnFrame();

    if (Device.Paused())
        return;

    if (m_game_objective)
    {
        bool b_remove = false;
        int dik = get_action_dik(kSCORES, 0);
        if (dik && !pInput->iGetAsyncKeyState(dik))
            b_remove = true;

        dik = get_action_dik(kSCORES, 1);
        if (!b_remove && dik && !pInput->iGetAsyncKeyState(dik))
            b_remove = true;

        if (b_remove)
        {
            RemoveCustomStatic("main_task");
            RemoveCustomStatic("secondary_task");
            m_game_objective = NULL;
        }
    }
}

bool CUIGameSP::IR_UIOnKeyboardPress(int dik)
{
    if (inherited::IR_UIOnKeyboardPress(dik))
        return true;
    if (Device.Paused())
        return false;

#ifdef DEBUG
    hud_adjust_mode_keyb(dik);
    attach_adjust_mode_keyb(dik);
#endif

    CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(Level().CurrentEntity());
    if (!pInvOwner)
        return false;
    CEntityAlive* EA = smart_cast<CEntityAlive*>(Level().CurrentEntity());
    if (!EA || !EA->g_Alive())
        return false;

    CActor* pActor = smart_cast<CActor*>(pInvOwner);
    if (!pActor)
        return false;

    if (!pActor->g_Alive())
        return false;

    switch (get_binded_action(dik))
    {
    case kACTIVE_JOBS:
    {
        if (!pActor->inventory_disabled())
            ShowPdaMenu();
        break;
    }

    case kINVENTORY:
    {
        if (!pActor->inventory_disabled())
            ShowActorMenu();

        break;
    }

    case kSCORES:
        if (!pActor->inventory_disabled())
        {
            m_game_objective = AddCustomStatic("main_task", true);
            CGameTask* t1 = Level().GameTaskManager().ActiveTask();
            m_game_objective->m_static->TextItemControl()->SetTextST((t1) ? t1->m_Title.c_str() : "st_no_active_task");

            if (t1 && t1->m_Description.c_str())
            {
                StaticDrawableWrapper* sm2 = AddCustomStatic("secondary_task", true);
                sm2->m_static->TextItemControl()->SetTextST(t1->m_Description.c_str());
            }
        }
        break;
    }

    return false;
}
#ifdef DEBUG
void CUIGameSP::Render()
{
    inherited::Render();
    hud_draw_adjust_mode();
    attach_draw_adjust_mode();
}
#endif

void CUIGameSP::StartTrade(CInventoryOwner* pActorInv, CInventoryOwner* pOtherOwner)
{
    //.	if( MainInputReceiver() )	return;

    ActorMenu->SetActor(pActorInv);
    ActorMenu->SetPartner(pOtherOwner);

    ActorMenu->SetMenuMode(mmTrade);
    ActorMenu->ShowDialog(true);
}

void CUIGameSP::StartUpgrade(CInventoryOwner* pActorInv, CInventoryOwner* pMech)
{
    //.	if( MainInputReceiver() )	return;

    ActorMenu->SetActor(pActorInv);
    ActorMenu->SetPartner(pMech);

    ActorMenu->SetMenuMode(mmUpgrade);
    ActorMenu->ShowDialog(true);
}

void CUIGameSP::StartTalk(bool disable_break)
{
    RemoveCustomStatic("main_task");
    RemoveCustomStatic("secondary_task");

    TalkMenu->b_disable_break = disable_break;
    TalkMenu->ShowDialog(true);
}

void CUIGameSP::StartCarBody(CInventoryOwner* pActorInv, CInventoryOwner* pOtherOwner) // Deadbody search
{
    if (TopInputReceiver())
        return;

    ActorMenu->SetActor(pActorInv);
    ActorMenu->SetPartner(pOtherOwner);

    ActorMenu->SetMenuMode(mmDeadBodySearch);
    ActorMenu->ShowDialog(true);
}

void CUIGameSP::StartCarBody(CInventoryOwner* pActorInv, CInventoryBox* pBox) // Deadbody search
{
    if (TopInputReceiver())
        return;

    ActorMenu->SetActor(pActorInv);
    ActorMenu->SetInvBox(pBox);
    VERIFY(pBox);

    ActorMenu->SetMenuMode(mmDeadBodySearch);
    ActorMenu->ShowDialog(true);
}

extern ENGINE_API BOOL bShowPauseString;
void CUIGameSP::ChangeLevel(GameGraph::_GRAPH_ID game_vert_id, u32 level_vert_id, Fvector pos, Fvector ang,
    Fvector pos2, Fvector ang2, bool b_use_position_cancel, const shared_str& message_str, bool b_allow_change_level)
{
    if (TopInputReceiver() != UIChangeLevelWnd)
    {
        UIChangeLevelWnd->m_game_vertex_id = game_vert_id;
        UIChangeLevelWnd->m_level_vertex_id = level_vert_id;
        UIChangeLevelWnd->m_position = pos;
        UIChangeLevelWnd->m_angles = ang;
        UIChangeLevelWnd->m_position_cancel = pos2;
        UIChangeLevelWnd->m_angles_cancel = ang2;
        UIChangeLevelWnd->m_b_position_cancel = b_use_position_cancel;
        UIChangeLevelWnd->m_b_allow_change_level = b_allow_change_level;
        UIChangeLevelWnd->m_message_str = message_str;

        UIChangeLevelWnd->ShowDialog(true);
    }
}

CChangeLevelWnd::CChangeLevelWnd()
{
    m_messageBox = new CUIMessageBox();
    m_messageBox->SetAutoDelete(true);
    AttachChild(m_messageBox);
}

void CChangeLevelWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    if (pWnd == m_messageBox)
    {
        if (msg == MESSAGE_BOX_YES_CLICKED)
        {
            OnOk();
        }
        else if (msg == MESSAGE_BOX_NO_CLICKED || msg == MESSAGE_BOX_OK_CLICKED)
        {
            OnCancel();
        }
    }
    else
        inherited::SendMessage(pWnd, msg, pData);
}

void CChangeLevelWnd::OnOk()
{
    HideDialog();
    NET_Packet p;
    p.w_begin(M_CHANGE_LEVEL);
    p.w(&m_game_vertex_id, sizeof(m_game_vertex_id));
    p.w(&m_level_vertex_id, sizeof(m_level_vertex_id));
    p.w_vec3(m_position);
    p.w_vec3(m_angles);

    Level().Send(p, net_flags(TRUE));
}

void CChangeLevelWnd::OnCancel()
{
    HideDialog();
    if (m_b_position_cancel)
        Actor()->MoveActor(m_position_cancel, m_angles_cancel);
}

bool CChangeLevelWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if (keyboard_action == WINDOW_KEY_PRESSED)
    {
        if (is_binded(kQUIT, dik))
            OnCancel();
        return true;
    }
    return inherited::OnKeyboardAction(dik, keyboard_action);
}

// Не инициализировалась форма, поскольку виртуальная функция отличалась набором аргуметов 
//morrey 
bool g_block_pause = false;

void CChangeLevelWnd::Show(bool status)
{
    inherited::Show(status);
    if (status)
    {
        m_messageBox->InitMessageBox(
            m_b_allow_change_level ? "message_box_change_level" : "message_box_change_level_disabled");
        SetWndPos(m_messageBox->GetWndPos());
        m_messageBox->SetWndPos(Fvector2().set(0.0f, 0.0f));
        SetWndSize(m_messageBox->GetWndSize());

        m_messageBox->SetText(m_message_str.c_str());

        g_block_pause = true;
        Device.Pause(TRUE, TRUE, TRUE, "CChangeLevelWnd_show");
        bShowPauseString = FALSE;
    }
    else
    {
        g_block_pause = false;
        Device.Pause(FALSE, TRUE, TRUE, "CChangeLevelWnd_hide");
    }
}

//old 
void CChangeLevelWnd::Hide()
{
    g_block_pause = false;
    Device.Pause(FALSE, TRUE, TRUE, "CChangeLevelWnd_hide");
}
//morrey 
