#include "StdAfx.h"
#include "UIActorMenu.h"
#include "UIXmlInit.h"
#include "xrUICore/XML/xrUIXmlParser.h"
#include "UICharacterInfo.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "UIActorStateInfo.h"
#include "UIItemInfo.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "UIMessageBoxEx.h"
#include "xrUICore/PropertiesBox/UIPropertiesBox.h"
#include "xrUICore/Buttons/UI3tButton.h"

#include "UIInventoryUpgradeWnd.h"
#include "UIInvUpgradeInfo.h"

#include "ai_space.h"
#include "alife_simulator.h"
#include "Common/object_broker.h"
#include "xrUICore/Callbacks/UIWndCallback.h"
#include "UIHelper.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/ui_base.h"
#include "string_table.h"

CUIActorMenu::CUIActorMenu()
    : m_currMenuMode(mmUndefined), m_PartnerWeight_end_x(), m_last_time(u32(-1)),
      m_repair_mode(false), m_item_info_view(false), m_highlight_clear(true),
      m_trade_partner_inventory_state(0)
{
    // Xottab_DUTY: Let others can launch SOC without debugger
    // XXX: to be removed
    if (ShadowOfChernobylMode)
        return;
    Construct();
}

CUIActorMenu::~CUIActorMenu()
{
    xr_delete(m_message_box_yes_no);
    xr_delete(m_message_box_ok);
    xr_delete(m_UIPropertiesBox);
    xr_delete(m_hint_wnd);
    xr_delete(m_ItemInfo);

    ClearAllLists();
}

void CUIActorMenu::Construct()
{
    CUIXml uiXml;
    uiXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, "actor_menu.xml");

    CUIXmlInit::InitWindow(uiXml, "main", 0, this);
    m_hint_wnd = UIHelper::CreateHint(uiXml, "hint_wnd");

    m_LeftBackground = UIHelper::CreateStatic(uiXml, "left_background", this);

    m_pUpgradeWnd = new CUIInventoryUpgradeWnd();
    if (!m_pUpgradeWnd->Init())
    {
        xr_delete(m_pUpgradeWnd);
    }
    else
    {
        AttachChild(m_pUpgradeWnd);
        m_pUpgradeWnd->SetAutoDelete(true);
    }

    m_ActorCharacterInfo = new CUICharacterInfo();
    m_ActorCharacterInfo->SetAutoDelete(true);
    AttachChild(m_ActorCharacterInfo);
    m_ActorCharacterInfo->InitCharacterInfo(&uiXml, "actor_ch_info");

    m_PartnerCharacterInfo = new CUICharacterInfo();
    m_PartnerCharacterInfo->SetAutoDelete(true);
    AttachChild(m_PartnerCharacterInfo);
    m_PartnerCharacterInfo->InitCharacterInfo(&uiXml, "partner_ch_info");

    m_RightDelimiter = UIHelper::CreateStatic(uiXml, "right_delimiter", this);
    if (!CallOfPripyatMode)
    {
        m_ActorTradeCaption = UIHelper::CreateTextWnd(uiXml, "right_delimiter:trade_caption", m_RightDelimiter, false);
        if (m_ActorTradeCaption)
            m_ActorTradeCaption->AdjustWidthToText();
    }
    m_ActorTradePrice = UIHelper::CreateTextWnd(uiXml, "right_delimiter:trade_price", m_RightDelimiter);
    m_ActorTradeWeightMax = UIHelper::CreateTextWnd(uiXml, "right_delimiter:trade_weight_max", m_RightDelimiter);

    m_LeftDelimiter = UIHelper::CreateStatic(uiXml, "left_delimiter", this);
    if (!CallOfPripyatMode)
    {
        m_PartnerTradeCaption = UIHelper::CreateTextWnd(uiXml, "left_delimiter:trade_caption", m_LeftDelimiter, false);
        if (m_PartnerTradeCaption)
            m_PartnerTradeCaption->AdjustWidthToText();
    }
    m_PartnerTradePrice = UIHelper::CreateTextWnd(uiXml, "left_delimiter:trade_price", m_LeftDelimiter);
    m_PartnerTradeWeightMax = UIHelper::CreateTextWnd(uiXml, "left_delimiter:trade_weight_max", m_LeftDelimiter);

    m_ActorBottomInfo = UIHelper::CreateStatic(uiXml, "actor_weight_caption", this);
    m_ActorWeight = UIHelper::CreateTextWnd(uiXml, "actor_weight", this);
    m_ActorWeightMax = UIHelper::CreateTextWnd(uiXml, "actor_weight_max", this);
    m_ActorBottomInfo->AdjustWidthToText();

    m_PartnerBottomInfo = UIHelper::CreateStatic(uiXml, "partner_weight_caption", this);
    m_PartnerWeight = UIHelper::CreateTextWnd(uiXml, "partner_weight", this);
    m_PartnerBottomInfo->AdjustWidthToText();
    m_PartnerWeight_end_x = m_PartnerWeight->GetWndPos().x;

    constexpr std::tuple<eActorMenuListType, cpcstr, cpcstr, cpcstr, bool> inventory_lists[] =
    {
        // { id,                   "xml_section_name",         "highlighter",             "blocker", is_it_critical_and_required }
        { eInventoryPistolList,    "dragdrop_pistol",          "inv_slot2_highlight",     nullptr,            true },
        { eInventoryAutomaticList, "dragdrop_automatic",       "inv_slot3_highlight",     nullptr,            true },

        { eInventoryOutfitList,    "dragdrop_outfit",          "outfit_slot_highlight",   nullptr,            true },
        { eInventoryHelmetList,    "dragdrop_helmet",          "helmet_slot_highlight",   "helmet_over",      false },

        { eInventoryBeltList,      "dragdrop_belt",            "artefact_slot_highlight", "belt_list_over",   true },
        { eInventoryDetectorList,  "dragdrop_detector",        "detector_slot_highlight", nullptr,            true },

        { eInventoryBagList,       "dragdrop_bag",             nullptr,                   nullptr,            true },

        { eTradeActorList,         "dragdrop_actor_trade",     nullptr,                   nullptr,            true },
        { eTradeActorBagList,      "dragdrop_actor_trade_bag", nullptr,                   nullptr,            true },

        { eTradePartnerList,       "dragdrop_partner_trade",   nullptr,                   nullptr,            true },
        { eTradePartnerBagList,    "dragdrop_partner_bag",     nullptr,                   nullptr,            true },

        { eDeadBodyBagList,        "dragdrop_deadbody_bag",    nullptr,                   nullptr,            true },

        { eTrashList,              "dragdrop_trash",           nullptr,                   nullptr,            false },
    };
    static_assert(std::size(inventory_lists) == eListCount,
        "All lists should be listed in the tuple above.");

    for (const auto& listDesc : inventory_lists)
    {
        auto [id, section, highlight, block, critical] = listDesc;
        CUIDragDropListEx*& list = m_pLists[id];

        list = UIHelper::CreateDragDropListEx(uiXml, section, this, critical);
        if (list && highlight)
        {
            const float dx = uiXml.ReadAttribFlt(highlight, 0, "dx", 0.0f);
            const float dy = uiXml.ReadAttribFlt(highlight, 0, "dy", 0.0f);
            m_pLists[id]->SetHighlighter(UIHelper::CreateStatic(uiXml, highlight, nullptr, false), { dx, dy });
        }
        if (list && block)
        {
            const float dx = uiXml.ReadAttribFlt(block, 0, "dx", 0.0f);
            const float dy = uiXml.ReadAttribFlt(block, 0, "dy", 0.0f);
            m_pLists[id]->SetBlocker(UIHelper::CreateStatic(uiXml, block, nullptr, false), { dx, dy });
        }
    }
    if (m_pLists[eInventoryHelmetList])
        m_pLists[eInventoryHelmetList]->SetMaxCellsCapacity(m_pLists[eInventoryHelmetList]->CellsCapacity());

    m_pQuickSlot = UIHelper::CreateDragDropReferenceList(uiXml, "dragdrop_quick_slots", this, false);
    if (m_pQuickSlot)
    {
        m_pQuickSlot->Initialize("quick_slot%d_text", "quick_use_str_%d", &uiXml);
        const float dx = uiXml.ReadAttribFlt("quick_slot_highlight", 0, "dx", 0.0f);
        const float dy = uiXml.ReadAttribFlt("quick_slot_highlight", 0, "dy", 0.0f);
        m_pQuickSlot->SetHighlighter(UIHelper::CreateStatic(uiXml, "quick_slot_highlight", nullptr, false), { dx, dy });
    }

    m_ActorMoney = UIHelper::CreateTextWnd(uiXml, "actor_money_static", this);
    m_PartnerMoney = UIHelper::CreateTextWnd(uiXml, "partner_money_static", this);

    m_WeaponSlot1_progress = UIHelper::CreateProgressBar(uiXml, "progress_bar_weapon1", this, false);
    m_WeaponSlot2_progress = UIHelper::CreateProgressBar(uiXml, "progress_bar_weapon2", this, false);
    m_Helmet_progress = UIHelper::CreateProgressBar(uiXml, "progress_bar_helmet", this, false);
    m_Outfit_progress = UIHelper::CreateProgressBar(uiXml, "progress_bar_outfit", this, false);

    if (!m_WeaponSlot1_progress)
        m_WeaponSlot1_progress = UIHelper::CreateProgressBar(uiXml, "progess_bar_weapon1", this, false);
    if (!m_WeaponSlot2_progress)
        m_WeaponSlot2_progress = UIHelper::CreateProgressBar(uiXml, "progess_bar_weapon2", this, false);
    if (!m_Helmet_progress)
        m_Helmet_progress = UIHelper::CreateProgressBar(uiXml, "progess_bar_helmet", this, false);
    if (!m_Outfit_progress)
        m_Outfit_progress = UIHelper::CreateProgressBar(uiXml, "progess_bar_outfit", this, false);

    m_trade_button = UIHelper::Create3tButton(uiXml, "trade_button", this, false);
    m_trade_buy_button = UIHelper::Create3tButton(uiXml, "trade_buy_button", this, false);
    m_trade_sell_button = UIHelper::Create3tButton(uiXml, "trade_sell_button", this, false);
    m_takeall_button = UIHelper::Create3tButton(uiXml, "takeall_button", this);
    m_exit_button = UIHelper::Create3tButton(uiXml, "exit_button", this);

    m_clock_value = UIHelper::CreateStatic(uiXml, "clock_value", this, false);

    m_ActorStateInfo = new ui_actor_state_wnd();
    m_ActorStateInfo->init_from_xml(uiXml, "actor_state_info");
    m_ActorStateInfo->SetAutoDelete(true);
    AttachChild(m_ActorStateInfo);

    m_ItemInfo = new CUIItemInfo();
    //-	m_ItemInfo->SetAutoDelete			(true);
    //-	AttachChild							(m_ItemInfo);
    m_ItemInfo->InitItemInfo("actor_menu_item.xml");

    if (ai().get_alife())
    {
        m_upgrade_info = new UIInvUpgradeInfo();
        m_upgrade_info->SetAutoDelete(true);
        AttachChild(m_upgrade_info);
        m_upgrade_info->init_from_xml("actor_menu_item.xml");
    }

    m_message_box_yes_no = new CUIMessageBoxEx();
    if (!m_message_box_yes_no->InitMessageBox("message_box_yes_no"))
        xr_delete(m_message_box_yes_no);
    else
    {
        m_message_box_yes_no->SetAutoDelete(true);
        m_message_box_yes_no->SetText("");
    }

    m_message_box_ok = new CUIMessageBoxEx();
    if (!m_message_box_ok->InitMessageBox("message_box_ok"))
        xr_delete(m_message_box_ok);
    else
    {
        m_message_box_ok->SetAutoDelete(true);
        m_message_box_ok->SetText("");
    }

    m_UIPropertiesBox = new CUIPropertiesBox();
    m_UIPropertiesBox->InitPropertiesBox(Fvector2().set(0, 0), Fvector2().set(300, 300));
    AttachChild(m_UIPropertiesBox);
    m_UIPropertiesBox->Hide();
    m_UIPropertiesBox->SetWindowName("property_box");

    InitSounds(uiXml);
    InitCallbacks();
    InitAllowedDrops();

    SetCurrentItem(NULL);
    SetActor(NULL);
    SetPartner(NULL);
    SetInvBox(NULL);

    DeInitInventoryMode();
    DeInitTradeMode();
    DeInitUpgradeMode();
    DeInitDeadBodySearchMode();
}

void CUIActorMenu::InitSounds(CUIXml& uiXml)
{
    XML_NODE stored_root = uiXml.GetLocalRoot();
    uiXml.SetLocalRoot(uiXml.NavigateToNode("action_sounds", 0));
    GEnv.Sound->create(sounds[eSndOpen], uiXml.Read("snd_open", 0, NULL), st_Effect, sg_SourceType);
    GEnv.Sound->create(sounds[eSndClose], uiXml.Read("snd_close", 0, NULL), st_Effect, sg_SourceType);
    GEnv.Sound->create(sounds[eItemToSlot], uiXml.Read("snd_item_to_slot", 0, NULL), st_Effect, sg_SourceType);
    GEnv.Sound->create(sounds[eItemToBelt], uiXml.Read("snd_item_to_belt", 0, NULL), st_Effect, sg_SourceType);
    GEnv.Sound->create(sounds[eItemToRuck], uiXml.Read("snd_item_to_ruck", 0, NULL), st_Effect, sg_SourceType);
    GEnv.Sound->create(sounds[eProperties], uiXml.Read("snd_properties", 0, NULL), st_Effect, sg_SourceType);
    GEnv.Sound->create(sounds[eDropItem], uiXml.Read("snd_drop_item", 0, NULL), st_Effect, sg_SourceType);
    GEnv.Sound->create(sounds[eAttachAddon], uiXml.Read("snd_attach_addon", 0, NULL), st_Effect, sg_SourceType);
    GEnv.Sound->create(sounds[eDetachAddon], uiXml.Read("snd_detach_addon", 0, NULL), st_Effect, sg_SourceType);
    GEnv.Sound->create(sounds[eItemUse], uiXml.Read("snd_item_use", 0, NULL), st_Effect, sg_SourceType);
    uiXml.SetLocalRoot(stored_root);
}

void CUIActorMenu::BindDragDropListEvents(CUIDragDropListEx* lst)
{
    if (!lst)
        return;
    lst->m_f_item_drop = CUIDragDropListEx::DRAG_CELL_EVENT(this, &CUIActorMenu::OnItemDrop);
    lst->m_f_item_start_drag = CUIDragDropListEx::DRAG_CELL_EVENT(this, &CUIActorMenu::OnItemStartDrag);
    lst->m_f_item_db_click = CUIDragDropListEx::DRAG_CELL_EVENT(this, &CUIActorMenu::OnItemDbClick);
    lst->m_f_item_selected = CUIDragDropListEx::DRAG_CELL_EVENT(this, &CUIActorMenu::OnItemSelected);
    lst->m_f_item_rbutton_click = CUIDragDropListEx::DRAG_CELL_EVENT(this, &CUIActorMenu::OnItemRButtonClick);
    lst->m_f_item_focus_received = CUIDragDropListEx::DRAG_CELL_EVENT(this, &CUIActorMenu::OnItemFocusReceive);
    lst->m_f_item_focus_lost = CUIDragDropListEx::DRAG_CELL_EVENT(this, &CUIActorMenu::OnItemFocusLost);
    lst->m_f_item_focused_update = CUIDragDropListEx::DRAG_CELL_EVENT(this, &CUIActorMenu::OnItemFocusedUpdate);
}

void CUIActorMenu::InitAllowedDrops()
{
    m_allowed_drops[iTrashSlot].push_back(iActorBag);
    m_allowed_drops[iTrashSlot].push_back(iActorSlot);
    m_allowed_drops[iTrashSlot].push_back(iActorBelt);
    m_allowed_drops[iTrashSlot].push_back(iQuickSlot);

    m_allowed_drops[iActorSlot].push_back(iActorBag);
    m_allowed_drops[iActorSlot].push_back(iActorSlot);
    m_allowed_drops[iActorSlot].push_back(iActorTrade);
    m_allowed_drops[iActorSlot].push_back(iDeadBodyBag);

    m_allowed_drops[iActorBag].push_back(iActorSlot);
    m_allowed_drops[iActorBag].push_back(iActorBelt);
    m_allowed_drops[iActorBag].push_back(iActorTrade);
    m_allowed_drops[iActorBag].push_back(iDeadBodyBag);
    m_allowed_drops[iActorBag].push_back(iActorBag);
    m_allowed_drops[iActorBag].push_back(iQuickSlot);

    m_allowed_drops[iActorBelt].push_back(iActorBag);
    m_allowed_drops[iActorBelt].push_back(iActorTrade);
    m_allowed_drops[iActorBelt].push_back(iDeadBodyBag);
    m_allowed_drops[iActorBelt].push_back(iActorBelt);

    m_allowed_drops[iActorTrade].push_back(iActorSlot);
    m_allowed_drops[iActorTrade].push_back(iActorBag);
    m_allowed_drops[iActorTrade].push_back(iActorBelt);
    m_allowed_drops[iActorTrade].push_back(iActorTrade);
    m_allowed_drops[iActorTrade].push_back(iQuickSlot);

    m_allowed_drops[iPartnerTradeBag].push_back(iPartnerTrade);
    m_allowed_drops[iPartnerTradeBag].push_back(iPartnerTradeBag);
    m_allowed_drops[iPartnerTrade].push_back(iPartnerTradeBag);
    m_allowed_drops[iPartnerTrade].push_back(iPartnerTrade);

    m_allowed_drops[iDeadBodyBag].push_back(iActorSlot);
    m_allowed_drops[iDeadBodyBag].push_back(iActorBag);
    m_allowed_drops[iDeadBodyBag].push_back(iActorBelt);
    m_allowed_drops[iDeadBodyBag].push_back(iDeadBodyBag);

    m_allowed_drops[iQuickSlot].push_back(iActorBag);
    m_allowed_drops[iQuickSlot].push_back(iActorTrade);
    m_allowed_drops[iQuickSlot].push_back(iQuickSlot);
}

void CUIActorMenu::InitCallbacks()
{
    if (m_trade_button)
        Register(m_trade_button);
    if (m_trade_buy_button)
        Register(m_trade_buy_button);
    if (m_trade_sell_button)
        Register(m_trade_sell_button);
    Register(m_takeall_button);
    Register(m_exit_button);
    Register(m_UIPropertiesBox);
    if (m_pUpgradeWnd)
        Register(m_pUpgradeWnd->m_btn_repair);

    if (m_trade_button)
    {
        AddCallback(m_trade_button, BUTTON_CLICKED,
            CUIWndCallback::void_function(this, &CUIActorMenu::OnBtnPerformTrade));
    }

    if (m_trade_buy_button)
    {
        AddCallback(m_trade_buy_button, BUTTON_CLICKED,
            CUIWndCallback::void_function(this, &CUIActorMenu::OnBtnPerformTradeBuy));
    }

    if (m_trade_sell_button)
    {
        AddCallback(m_trade_sell_button, BUTTON_CLICKED,
            CUIWndCallback::void_function(this, &CUIActorMenu::OnBtnPerformTradeSell));
    }


    AddCallback(m_takeall_button, BUTTON_CLICKED,
        CUIWndCallback::void_function(this, &CUIActorMenu::TakeAllFromPartner));
    
    AddCallback(m_exit_button, BUTTON_CLICKED,
        CUIWndCallback::void_function(this, &CUIActorMenu::OnBtnExitClicked));
    
    AddCallback(m_UIPropertiesBox, PROPERTY_CLICKED,
        CUIWndCallback::void_function(this, &CUIActorMenu::ProcessPropertiesBoxClicked));
    
    AddCallback(m_pUpgradeWnd->m_btn_repair, BUTTON_CLICKED,
        CUIWndCallback::void_function(this, &CUIActorMenu::TryRepairItem));

    BindDragDropListEvents(m_pLists[eInventoryPistolList]);
    BindDragDropListEvents(m_pLists[eInventoryAutomaticList]);

    BindDragDropListEvents(m_pLists[eInventoryOutfitList]);
    BindDragDropListEvents(m_pLists[eInventoryHelmetList]);

    BindDragDropListEvents(m_pLists[eInventoryBeltList]);
    BindDragDropListEvents(m_pLists[eInventoryDetectorList]);

    BindDragDropListEvents(m_pLists[eInventoryBagList]);

    BindDragDropListEvents(m_pLists[eTradeActorBagList]);
    BindDragDropListEvents(m_pLists[eTradeActorList]);

    BindDragDropListEvents(m_pLists[eTradePartnerBagList]);
    BindDragDropListEvents(m_pLists[eTradePartnerList]);

    BindDragDropListEvents(m_pLists[eDeadBodyBagList]);

    if (m_pLists[eTrashList])
    {
        m_pLists[eTrashList]->m_f_item_drop = CUIDragDropListEx::DRAG_CELL_EVENT(this, &CUIActorMenu::OnItemDrop);
        m_pLists[eTrashList]->m_f_drag_event = CUIDragDropListEx::DRAG_ITEM_EVENT(this, &CUIActorMenu::OnDragItemOnTrash);
    }

    BindDragDropListEvents(m_pQuickSlot);
}

void CUIActorMenu::UpdateButtonsLayout()
{
    if (m_trade_button)
    {
        Fvector2 btn_exit_pos;
        if (m_trade_button->IsShown() || m_takeall_button->IsShown())
        {
            btn_exit_pos = m_trade_button->GetWndPos();
            btn_exit_pos.x += m_trade_button->GetWndSize().x;
        }
        else
        {
            btn_exit_pos = m_trade_button->GetWndPos();
            btn_exit_pos.x += m_trade_button->GetWndSize().x / 2.0f;
        }

        m_exit_button->SetWndPos(btn_exit_pos);
    }

    if (m_pQuickSlot)
        m_pQuickSlot->UpdateLabels();
    UpdateConditionProgressBars();
}
