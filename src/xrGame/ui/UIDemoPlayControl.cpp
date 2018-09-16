#include "stdafx.h"
#include "UIXmlInit.h"
#include "UI3tButton.h"
#include "UIProgressBar.h"
#include "UIPropertiesBox.h"
#include "UIListBox.h"
#include "UIListBoxItem.h"
#include "UIDemoPlayControl.h"
#include "UICursor.h"
#include "xrEngine/XR_IOConsole.h"
#include "string_table.h"
#include "Level.h"
#include "demoinfo.h"
#include "demoplay_control.h"

CUIDemoPlayControl::CUIDemoPlayControl()
{
    m_progress_bar = new CUIProgressBar();
    AttachChild(m_progress_bar);
    m_background = new CUIStatic();
    AttachChild(m_background);
    m_restart_btn = new CUI3tButton();
    AttachChild(m_restart_btn);
    m_decrease_speed_btn = new CUI3tButton();
    AttachChild(m_decrease_speed_btn);
    m_play_pause_btn = new CUI3tButton();
    AttachChild(m_play_pause_btn);
    m_increase_speed_btn = new CUI3tButton();
    AttachChild(m_increase_speed_btn);
    m_rewind_until_btn = new CUI3tButton();
    AttachChild(m_rewind_until_btn);
    m_repeat_rewind_btn = new CUI3tButton();
    AttachChild(m_repeat_rewind_btn);
    m_static_demo_status = new CUITextWnd();
    AttachChild(m_static_demo_status);

    m_all_players = new CUIPropertiesBox();
    AttachChild(m_all_players);
    m_all_players->SetWindowName("all_players_pbox");

    m_rewind_type = new CUIPropertiesBox(m_all_players);
    AttachChild(m_rewind_type);
    m_rewind_type->SetWindowName("rewind_types_pbox");

    m_players_store = NULL;
    m_players = NULL;
    m_demo_play_control = NULL;
}

CUIDemoPlayControl::~CUIDemoPlayControl()
{
    xr_delete(m_progress_bar);
    xr_delete(m_background);
    xr_delete(m_restart_btn);
    xr_delete(m_decrease_speed_btn);
    xr_delete(m_play_pause_btn);
    xr_delete(m_increase_speed_btn);
    xr_delete(m_rewind_until_btn);
    xr_delete(m_repeat_rewind_btn);
    xr_delete(m_static_demo_status);
    xr_delete(m_rewind_type);
    xr_delete(m_all_players);

    xr_delete(m_players);
    xr_free(m_players_store);
}

void CUIDemoPlayControl::Init()
{
    CUIXml xml_doc;
    xml_doc.Load(CONFIG_PATH, UI_PATH, "demo_play_control.xml");

    CUIXmlInit::InitWindow(xml_doc, "demo_play_control", 0, this);

    CUIXmlInit::InitStatic(xml_doc, "demo_play_control:background", 0, m_background);

    CUIXmlInit::Init3tButton(xml_doc, "demo_play_control:btn_restart", 0, m_restart_btn);
    CUIXmlInit::Init3tButton(xml_doc, "demo_play_control:btn_decrease_speed", 0, m_decrease_speed_btn);
    CUIXmlInit::Init3tButton(xml_doc, "demo_play_control:btn_play_pause", 0, m_play_pause_btn);
    CUIXmlInit::Init3tButton(xml_doc, "demo_play_control:btn_increase_speed", 0, m_increase_speed_btn);
    CUIXmlInit::Init3tButton(xml_doc, "demo_play_control:btn_rewind_until", 0, m_rewind_until_btn);
    CUIXmlInit::Init3tButton(xml_doc, "demo_play_control:btn_repeat_rewind", 0, m_repeat_rewind_btn);
    CUIXmlInit::InitProgressBar(xml_doc, "demo_play_control:progress", 0, m_progress_bar);
    CUIXmlInit::InitTextWnd(xml_doc, "demo_play_control:static_demo_status", 0, m_static_demo_status);
    CUIWindow tmp_prop_boxes_wnd;
    CUIXmlInit::InitWindow(xml_doc, "demo_play_control:rewind_property_boxes", 0, &tmp_prop_boxes_wnd);

    Register(m_restart_btn);
    AddCallback(m_restart_btn, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIDemoPlayControl::OnRestart));
    Register(m_decrease_speed_btn);
    AddCallback(m_decrease_speed_btn, BUTTON_CLICKED,
        CUIWndCallback::void_function(this, &CUIDemoPlayControl::OnDecresaseSpeed));
    Register(m_play_pause_btn);
    AddCallback(
        m_play_pause_btn, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIDemoPlayControl::OnPlayPause));
    Register(m_increase_speed_btn);
    AddCallback(m_increase_speed_btn, BUTTON_CLICKED,
        CUIWndCallback::void_function(this, &CUIDemoPlayControl::OnIncreaseSpeed));
    Register(m_rewind_until_btn);
    AddCallback(
        m_rewind_until_btn, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIDemoPlayControl::OnRewindUntil));
    Register(m_repeat_rewind_btn);

    AddCallback(m_rewind_type, PROPERTY_CLICKED,
        CUIWndCallback::void_function(this, &CUIDemoPlayControl::OnRewindTypeSelected));
    AddCallback(m_all_players, PROPERTY_CLICKED,
        CUIWndCallback::void_function(this, &CUIDemoPlayControl::OnRewindPlayerSelected));

    AddCallback(
        m_repeat_rewind_btn, BUTTON_CLICKED, CUIWndCallback::void_function(this, &CUIDemoPlayControl::OnRepeatRewind));

    InitRewindTypeList();
    InitAllPlayers();

    Register(m_rewind_type);
    Register(m_all_players);

    m_last_curr_pos = GetWndPos();
    m_last_curr_pos.add(m_restart_btn->GetWndPos());

    m_rewind_type_pos = m_background->GetWndPos();
    m_rewind_type_pos.x += m_background->GetWidth() - m_rewind_type->GetWidth() - 14.0f;
    m_rewind_type_pos.y -= m_rewind_type->GetHeight();

    tmp_prop_boxes_wnd.GetWndRect(m_pbox_rect);

    m_demo_play_control = Level().GetDemoPlayControl();
    R_ASSERT(m_demo_play_control);

    // m_play_pause_btn->SetText("Pause");
}

void CUIDemoPlayControl::InitRewindTypeList()
{
    CStringTable st;
    m_rewind_type->InitPropertiesBox(Fvector2().set(0, 0), Fvector2().set(100, 200));
    m_rewind_type->AddItem(st.translate("mpd_rewind_until_start").c_str(), NULL, eRewindUntilStart);
    m_rewind_type->AddItem(st.translate("mpd_rewind_until_kill").c_str(), NULL, eRewindUntilKill);
    m_rewind_type->AddItem(st.translate("mpd_rewind_until_die").c_str(), NULL, eRewindUntilDie);
    m_rewind_type->AddItem(st.translate("mpd_rewind_until_arttake").c_str(), NULL, eRewindUntilArtTake);
    m_rewind_type->AddItem(st.translate("mpd_rewind_until_artdrop").c_str(), NULL, eRewindUntilArtDrop);
    m_rewind_type->AddItem(st.translate("mpd_rewind_until_artdeliver").c_str(), NULL, eRewindUntilArtDeliver);
    m_rewind_type->AutoUpdateSize();
    m_rewind_type->Hide();
}

void CUIDemoPlayControl::InitAllPlayers()
{
    demo_info* tmp_demo_info = Level().GetDemoInfo();
    VERIFY(tmp_demo_info);
    u32 players_count = tmp_demo_info->get_players_count();

    VERIFY(!m_players_store && !m_players);
    m_players_store = xr_malloc(players_count * sizeof(shared_str));
    m_players = new players_collection_t(m_players_store, players_count);

    CStringTable st;
    m_all_players->InitPropertiesBox(Fvector2().set(0, 0), Fvector2().set(100, 200));
    m_all_players->AddItem(st.translate("mpd_any_player").c_str(), NULL, 0); // warning ! zero tag means Any player !

    m_players->clear();
    for (u32 i = 0; i != players_count; ++i)
    {
        demo_player_info const* tmp_player = tmp_demo_info->get_player(i);
        R_ASSERT(tmp_player);
        LPCSTR tmp_player_name = tmp_player->get_name();
        R_ASSERT(tmp_player_name);
        m_players->push_back(shared_str(tmp_player_name));
        m_all_players->AddItem(tmp_player_name, NULL, i + 1); // warning ! player_index = tag - 1 !!!
    }
    m_all_players->AutoUpdateSize();
    m_all_players->Hide();
}

bool CUIDemoPlayControl::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
    if ((dik == SDL_SCANCODE_LCTRL) && (keyboard_action == WINDOW_KEY_RELEASED))
    {
        m_last_curr_pos = GetUICursor().GetCursorPosition();
        m_rewind_type->Hide();
        HideDialog();
        return true;
    }
    return inherited::OnKeyboardAction(dik, keyboard_action);
}

void CUIDemoPlayControl::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
    CUIWndCallback::OnEvent(pWnd, msg, pData);
    inherited::SendMessage(pWnd, msg, pData);
}

void CUIDemoPlayControl::OnPlayPause(CUIWindow* w, void* d)
{
    StopRewind();
    if (Device.Paused())
    {
        // m_play_pause_btn->SetText("Pause");
        Device.Pause(FALSE, TRUE, TRUE, "mpdemoplay ctrl unpause");
        return;
    }
    // m_play_pause_btn->SetText("Play");
    Device.Pause(TRUE, TRUE, TRUE, "mpdemoplay ctrl pause");
}

void CUIDemoPlayControl::OnRestart(CUIWindow* w, void* d)
{
    StopRewind();
    Console->Execute("mpdemoplay_restart");
}
void CUIDemoPlayControl::OnIncreaseSpeed(CUIWindow* w, void* d)
{
    StopRewind();
    Console->Execute("mpdemoplay_mulspeed");
}
void CUIDemoPlayControl::OnDecresaseSpeed(CUIWindow* w, void* d)
{
    StopRewind();
    Console->Execute("mpdemoplay_divspeed");
}

void CUIDemoPlayControl::OnRewindUntil(CUIWindow* w, void* d) { m_rewind_type->Show(m_pbox_rect, m_rewind_type_pos); }
void CUIDemoPlayControl::OnRewindTypeSelected(CUIWindow* w, void* d)
{
    VERIFY(m_rewind_type);

    CUIListBoxItem* tmp_item = m_rewind_type->GetClickedItem();
    VERIFY(tmp_item);
    u32 tmp_item_tag = tmp_item->GetTAG();

    m_last_rewind_type = static_cast<ERewindTypeTags>(tmp_item_tag);

    if (tmp_item_tag == eRewindUntilStart)
    {
        OnRepeatRewind(NULL, NULL);
        m_rewind_type->Hide();
        return;
    }

    m_rewind_type->ShowSubMenu();
}

void CUIDemoPlayControl::OnRewindPlayerSelected(CUIWindow* w, void* d)
{
    VERIFY(m_all_players);
    CUIListBoxItem* tmp_item = m_all_players->GetClickedItem();
    VERIFY(tmp_item);

    u32 tmp_item_tag = tmp_item->GetTAG();
    if (tmp_item_tag == 0)
    {
        m_last_rewind_target = "";
        OnRepeatRewind(NULL, NULL);
        return;
    }
    u32 player_index = tmp_item_tag - 1;
    R_ASSERT(player_index < m_players->size());

    m_last_rewind_target = m_players->at(player_index);
    OnRepeatRewind(NULL, NULL);
}

void CUIDemoPlayControl::OnRepeatRewind(CUIWindow* w, void* d)
{
    VERIFY(m_demo_play_control);
    StopRewind();
    demoplay_control::user_callback_t tmpcb;
    tmpcb.bind(this, &CUIDemoPlayControl::UIStopRewindCb);
    bool rewind_result = false;

    switch (m_last_rewind_type)
    {
    case eRewindUntilStart:
    {
        rewind_result = m_demo_play_control->rewind_until(demoplay_control::on_round_start, shared_str(), tmpcb);
    };
    break;
    case eRewindUntilKill:
    {
        rewind_result = m_demo_play_control->rewind_until(demoplay_control::on_kill, m_last_rewind_target, tmpcb);
    };
    break;
    case eRewindUntilDie:
    {
        rewind_result = m_demo_play_control->rewind_until(demoplay_control::on_die, m_last_rewind_target, tmpcb);
    };
    break;
    case eRewindUntilArtTake:
    {
        rewind_result =
            m_demo_play_control->rewind_until(demoplay_control::on_artefactcapturing, m_last_rewind_target, tmpcb);
    };
    break;
    case eRewindUntilArtDrop:
    {
        rewind_result =
            m_demo_play_control->rewind_until(demoplay_control::on_artefactloosing, m_last_rewind_target, tmpcb);
    };
    break;
    case eRewindUntilArtDeliver:
    {
        rewind_result =
            m_demo_play_control->rewind_until(demoplay_control::on_artefactdelivering, m_last_rewind_target, tmpcb);
    };
    break;
    default: NODEFAULT;
    }; // switch (m_last_rewind_query)

    if (rewind_result)
        UIStartRewind();
}

void CUIDemoPlayControl::Update()
{
    LPCSTR demo_play_string = NULL;
    string32 demo_pos;
    string32 demo_speed;
    // st.translate("demo play active : ").c_str() (need to translate ?)
    CStringTable st;

    xr_sprintf(demo_pos, ": %2d %%, ", int(Level().GetDemoPlayPos() * 100));
    xr_sprintf(demo_speed, ": %1.1fx", Level().GetDemoPlaySpeed());

    STRCONCAT(demo_play_string, Device.Paused() ? st.translate("mpdemoplay_paused") : st.translate("mpdemoplay_active"),
        demo_pos, st.translate("mpdemoplay_speed"), demo_speed);
    // m_game_ui->SetDemoPlayCaption(demo_play_string);
    m_progress_bar->SetProgressPos(Level().GetDemoPlayPos());
    m_static_demo_status->SetText(demo_play_string);
    inherited::Update();
}

void CUIDemoPlayControl::StopRewind()
{
    VERIFY(m_demo_play_control);
    m_demo_play_control->stop_rewind();
    UIStopRewindCb();
}

void CUIDemoPlayControl::UIStartRewind()
{
    m_increase_speed_btn->Enable(false);
    m_decrease_speed_btn->Enable(false);
    m_repeat_rewind_btn->Enable(false);
}

void CUIDemoPlayControl::UIStopRewindCb()
{
    m_increase_speed_btn->Enable(true);
    m_decrease_speed_btn->Enable(true);
    m_repeat_rewind_btn->Enable(true);
}
