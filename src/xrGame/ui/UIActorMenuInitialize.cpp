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
#include "UIOutfitSlot.h"

constexpr cpcstr ACTOR_MENU_XML      = "actor_menu.xml";
constexpr cpcstr ACTOR_MENU_ITEM_XML = "actor_menu_item.xml";

constexpr cpcstr INVENTORY_XML       = "inventory_new.xml";
constexpr cpcstr INVENTORY_ITEM_XML  = "inventory_item.xml";

constexpr cpcstr TRADE_XML           = "trade.xml";
constexpr cpcstr TRADE_CHARACTER_XML = "trade_character.xml";
constexpr cpcstr TRADE_ITEM_XML      = "trade_item.xml";

constexpr cpcstr CAR_BODY_XML        = "carbody_new.xml";
constexpr cpcstr CARBODY_ITEM_XML    = "carbody_item.xml";

CUIActorMenu::CUIActorMenu()
    : m_currMenuMode(mmUndefined), m_PartnerWeight_end_x(), m_last_time(u32(-1)),
      m_repair_mode(false), m_item_info_view(false), m_highlight_clear(true),
      m_trade_partner_inventory_state(0)
{
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
    m_UIPropertiesBox = new CUIPropertiesBox();
    m_UIPropertiesBox->InitPropertiesBox(Fvector2().set(0, 0), Fvector2().set(300, 300));
    m_UIPropertiesBox->SetWindowName("property_box");

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

    m_ActorStateInfo = new ui_actor_state_wnd();
    m_ActorStateInfo->SetAutoDelete(true);

    if (ShadowOfChernobylMode)
    {
        CUIXml inventoryXml, tradeXml, carbodyXml;
        inventoryXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, INVENTORY_XML);
        tradeXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, TRADE_XML);
        carbodyXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, CAR_BODY_XML);

        InitializeInventoryMode(inventoryXml);
        InitializeTradeMode(tradeXml);
        InitializeSearchLootMode(carbodyXml);
        InitSounds(inventoryXml);
    }
    else
    {
        CUIXml actorMenuXml;
        actorMenuXml.Load(CONFIG_PATH, UI_PATH, UI_PATH_DEFAULT, ACTOR_MENU_XML);
        InitializeUniversal(actorMenuXml);
        InitializeUpgradeMode(actorMenuXml);
        InitSounds(actorMenuXml);
    }
    InitCallbacks();
    InitAllowedDrops();

    AttachChild(m_UIPropertiesBox);
    m_UIPropertiesBox->Hide();

    SetCurrentItem(nullptr);
    SetActor(nullptr);
    SetPartner(nullptr);
    SetInvBox(nullptr);

    DeInitInventoryMode();
    DeInitTradeMode();
    DeInitUpgradeMode();
    DeInitDeadBodySearchMode();
}

void CUIActorMenu::InitializeUniversal(CUIXml& uiXml)
{
    CUIXmlInit::InitWindow(uiXml, "main", 0, this);
    m_hint_wnd = UIHelper::CreateHint(uiXml, "hint_wnd");

    m_LeftBackground = UIHelper::CreateStatic(uiXml, "left_background", this);

    m_ActorCharacterInfo = new CUICharacterInfo();
    m_TradeActorCharacterInfo = m_ActorCharacterInfo;
    m_SearchLootActorCharacterInfo = m_ActorCharacterInfo;
    m_ActorCharacterInfo->SetAutoDelete(true);
    AttachChild(m_ActorCharacterInfo);
    m_ActorCharacterInfo->InitCharacterInfo(&uiXml, "actor_ch_info");

    m_PartnerCharacterInfo = new CUICharacterInfo();
    m_TradePartnerCharacterInfo = m_PartnerCharacterInfo;
    m_SearchLootPartnerCharacterInfo = m_PartnerCharacterInfo;
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

    m_ActorMoney = UIHelper::CreateStatic(uiXml, "actor_money_static", this);
    m_TradeActorMoney = m_ActorMoney;
    m_PartnerMoney = UIHelper::CreateStatic(uiXml, "partner_money_static", this);

    constexpr std::tuple<eActorMenuListType, cpcstr, cpcstr, cpcstr, cpcstr, bool> inventory_lists[] =
    {
        // { id,                   "xml_section_name",         "condition_indicator,  "highlighter",             "blocker", is_it_critical_and_required }
        { eInventoryPistolList,    "dragdrop_pistol",          "progess_bar_weapon1", "inv_slot2_highlight",     nullptr,            true },
        { eInventoryAutomaticList, "dragdrop_automatic",       "progess_bar_weapon2", "inv_slot3_highlight",     nullptr,            true },

        { eInventoryOutfitList,    "dragdrop_outfit",          "progess_bar_outfit",  "outfit_slot_highlight",   nullptr,            true },
        { eInventoryHelmetList,    "dragdrop_helmet",          "progess_bar_helmet",  "helmet_slot_highlight",   "helmet_over",      false },

        { eInventoryBeltList,      "dragdrop_belt",            nullptr,               "artefact_slot_highlight", "belt_list_over",   true },
        { eInventoryDetectorList,  "dragdrop_detector",        nullptr,               "detector_slot_highlight", nullptr,            true },

        { eInventoryBagList,       "dragdrop_bag",             nullptr,               nullptr,                   nullptr,            true },

        { eTradeActorList,         "dragdrop_actor_trade",     nullptr,               nullptr,                   nullptr,            true },
        { eTradeActorBagList,      "dragdrop_actor_trade_bag", nullptr,               nullptr,                   nullptr,            true },

        { eTradePartnerList,       "dragdrop_partner_trade",   nullptr,               nullptr,                   nullptr,            true },
        { eTradePartnerBagList,    "dragdrop_partner_bag",     nullptr,               nullptr,                   nullptr,            true },

        { eSearchLootBagList,      "dragdrop_deadbody_bag",    nullptr,               nullptr,                   nullptr,            true },
        { eSearchLootActorBagList, nullptr,                    nullptr,               nullptr,                   nullptr,            false },

        { eTrashList,              "dragdrop_trash",           nullptr,               nullptr,                   nullptr,            false },
    };
    static_assert(std::size(inventory_lists) == eListCount,
        "All lists should be listed in the tuple above.");

    for (auto [id, section, conditionIndicator, highlight, block, critical] : inventory_lists)
    {
        if (!section)
            continue;
        CUIDragDropListEx*& list = m_pLists[id];

        list = UIHelper::CreateDragDropListEx(uiXml, section, this, critical);
        if (!list)
            continue;

        if (conditionIndicator)
        {
            m_pLists[id]->SetConditionIndicator(UIHelper::CreateProgressBar(uiXml, conditionIndicator, nullptr, false));
        }
        if (highlight)
        {
            const float dx = uiXml.ReadAttribFlt(highlight, 0, "dx", 0.0f);
            const float dy = uiXml.ReadAttribFlt(highlight, 0, "dy", 0.0f);
            m_pLists[id]->SetHighlighter(UIHelper::CreateStatic(uiXml, highlight, nullptr, false), { dx, dy });
        }
        if (block)
        {
            const float dx = uiXml.ReadAttribFlt(block, 0, "dx", 0.0f);
            const float dy = uiXml.ReadAttribFlt(block, 0, "dy", 0.0f);
            m_pLists[id]->SetBlocker(UIHelper::CreateStatic(uiXml, block, nullptr, false), { dx, dy });
        }
    }
    m_pLists[eSearchLootActorBagList] = m_pLists[eInventoryBagList];

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

    m_trade_button = UIHelper::Create3tButton(uiXml, "trade_button", this, false);
    m_trade_buy_button = UIHelper::Create3tButton(uiXml, "trade_buy_button", this, false);
    m_trade_sell_button = UIHelper::Create3tButton(uiXml, "trade_sell_button", this, false);
    m_takeall_button = UIHelper::Create3tButton(uiXml, "takeall_button", this);
    m_exit_button = UIHelper::Create3tButton(uiXml, "exit_button", this);

    m_clock_value = UIHelper::CreateStatic(uiXml, "clock_value", this, false);

    m_ActorStateInfo->init_from_xml(uiXml, "actor_state_info");
    AttachChild(m_ActorStateInfo);

    m_ItemInfo = new CUIItemInfo();
    m_ItemInfo->InitItemInfo(ACTOR_MENU_ITEM_XML);
    //-	m_ItemInfo->SetAutoDelete			(true);
    //-	AttachChild							(m_ItemInfo);
}

void CUIActorMenu::InitializeUpgradeMode(CUIXml& /*uiXml*/)
{
    m_pUpgradeWnd = new CUIInventoryUpgradeWnd();
    if (!m_pUpgradeWnd->Init())
        xr_delete(m_pUpgradeWnd);
    else
    {
        AttachChild(m_pUpgradeWnd);
        m_pUpgradeWnd->SetAutoDelete(true);
    }

    if (ai().get_alife())
    {
        m_upgrade_info = new UIInvUpgradeInfo();
        m_upgrade_info->SetAutoDelete(true);
        AttachChild(m_upgrade_info);
        m_upgrade_info->init_from_xml(ACTOR_MENU_ITEM_XML);
    }
}

void CUIActorMenu::InitializeInventoryMode(CUIXml& uiXml)
{
    m_pInventoryWnd = UIHelper::CreateNormalWindow(uiXml, "main", this);

    UIHelper::CreateStatic(uiXml, "belt_slots", m_pInventoryWnd);
    UIHelper::CreateStatic(uiXml, "back", m_pInventoryWnd);
    UIHelper::CreateStatic(uiXml, "bottom_static", m_pInventoryWnd);
    CUIStatic* bagWnd = UIHelper::CreateStatic(uiXml, "bag_static", m_pInventoryWnd);

    m_ActorMoney = UIHelper::CreateStatic(uiXml, "money_static", m_pInventoryWnd);

    CUIStatic* descWnd = UIHelper::CreateStatic(uiXml, "descr_static", m_pInventoryWnd);
    m_ItemInfoInventoryMode = new CUIItemInfo();
    m_ItemInfoInventoryMode->SetAutoDelete(true);
    descWnd->AttachChild(m_ItemInfoInventoryMode);
    m_ItemInfoInventoryMode->InitItemInfo({ 0.f, 0.f }, descWnd->GetWndSize(), INVENTORY_ITEM_XML);

    CUIFrameWindow* personalWnd = UIHelper::CreateFrameWindow(uiXml, "character_frame_window", m_pInventoryWnd);
    personalWnd->AttachChild(m_ActorStateInfo);

    UIHelper::CreateStatic(uiXml, "static_personal", personalWnd);
    m_ActorStateInfo->init_from_xml(uiXml);
    AttachChild(m_ActorStateInfo);

    std::tuple<eActorMenuListType, cpcstr, CUIWindow*> inventory_lists[] =
    {
        // { id,                   "xml_section_name",   parent }
        { eInventoryPistolList,    "dragdrop_pistol",    m_pInventoryWnd },
        { eInventoryAutomaticList, "dragdrop_automatic", m_pInventoryWnd },
        { eInventoryOutfitList,    "dragdrop_outfit",    m_pInventoryWnd },
        { eInventoryBeltList,      "dragdrop_belt",      m_pInventoryWnd },
        { eInventoryBagList,       "dragdrop_bag",       bagWnd },
    };
    for (auto [id, section, parent] : inventory_lists)
    {
        if (id != eInventoryOutfitList)
            m_pLists[id] = UIHelper::CreateDragDropListEx(uiXml, section, parent);
        else // eInventoryOutfitList
        {
            CUIOutfitDragDropList* list = new CUIOutfitDragDropList();
            list->SetAutoDelete(true);
            parent->AttachChild(list);
            CUIXmlInit::InitDragDropListEx(uiXml, section, 0, list);
            m_pLists[id] = list;
        }
    }

    CUIStatic* time = UIHelper::CreateStatic(uiXml, "time_static", m_pInventoryWnd);
    m_clock_value = UIHelper::CreateStatic(uiXml, "time_static_str", time);

    m_exit_button = UIHelper::Create3tButton(uiXml, "exit_button", m_pInventoryWnd);
}

void CUIActorMenu::InitializeTradeMode(CUIXml& uiXml)
{
    m_pTradeWnd = UIHelper::CreateNormalWindow(uiXml, "main", this);

    UIHelper::CreateStatic(uiXml, "top_background", m_pTradeWnd);
    UIHelper::CreateStatic(uiXml, "bottom_background", m_pTradeWnd);

    CUIStatic* actorIcon = UIHelper::CreateStatic(uiXml, "static_icon", 0, m_pTradeWnd);
    CUIStatic* partnerIcon = UIHelper::CreateStatic(uiXml, "static_icon", 1, m_pTradeWnd);

    m_TradeActorCharacterInfo = new CUICharacterInfo();
    m_TradePartnerCharacterInfo = new CUICharacterInfo();

    actorIcon->AttachChild(m_TradeActorCharacterInfo);
    m_TradeActorCharacterInfo->SetAutoDelete(true);
    m_TradeActorCharacterInfo->InitCharacterInfo({ 0.f, 0.f }, actorIcon->GetWndSize(), TRADE_CHARACTER_XML);

    partnerIcon->AttachChild(m_TradePartnerCharacterInfo);
    m_TradePartnerCharacterInfo->SetAutoDelete(true);
    m_TradePartnerCharacterInfo->InitCharacterInfo({ 0.f, 0.f }, partnerIcon->GetWndSize(), TRADE_CHARACTER_XML);

    CUIStatic* actorBagWnd = UIHelper::CreateStatic(uiXml, "our_bag_static", m_pTradeWnd);
    CUIStatic* partnerBagWnd = UIHelper::CreateStatic(uiXml, "others_bag_static", m_pTradeWnd);

    m_TradeActorMoney = UIHelper::CreateStatic(uiXml, "our_money_static", actorBagWnd);
    m_PartnerMoney = UIHelper::CreateStatic(uiXml, "other_money_static", partnerBagWnd);

    CUIStatic* actorTradeWnd = UIHelper::CreateStatic(uiXml, "static", 0, m_pTradeWnd);
    CUIStatic* partnerTradeWnd = UIHelper::CreateStatic(uiXml, "static", 1, m_pTradeWnd);

    std::tuple<eActorMenuListType, int, CUIStatic*> inventory_lists[] =
    {
        // { id,                   index, parent }
        { eTradeActorList,         2,     actorTradeWnd },
        { eTradeActorBagList,      0,     actorBagWnd },
                                              
        { eTradePartnerList,       3,     partnerTradeWnd },
        { eTradePartnerBagList,    1,     partnerBagWnd },
    };
    for (auto [id, index, parent] : inventory_lists)
    {
        m_pLists[id] = UIHelper::CreateDragDropListEx(uiXml, "dragdrop_list", index, parent);
    }

    CUIStatic* descWnd = UIHelper::CreateStatic(uiXml, "desc_static", m_pTradeWnd);
    m_ItemInfoTradeMode = new CUIItemInfo();
    m_ItemInfoTradeMode->SetAutoDelete(true);
    descWnd->AttachChild(m_ItemInfoTradeMode);
    m_ItemInfoTradeMode->InitItemInfo({ 0.f, 0.f }, descWnd->GetWndSize(), TRADE_ITEM_XML);

    m_trade_button = UIHelper::Create3tButton(uiXml, "button", 0, m_pTradeWnd);
    CUI3tButton* toTalkBtn = UIHelper::Create3tButton(uiXml, "button", 1, m_pTradeWnd);
    RegisterCallback(toTalkBtn, BUTTON_CLICKED,
        CUIWndCallback::void_function(this, &CUIActorMenu::OnBtnExitClicked));
}

void CUIActorMenu::InitializeSearchLootMode(CUIXml& uiXml)
{
    m_pSearchLootWnd = UIHelper::CreateNormalWindow(uiXml, "main", this);

    UIHelper::CreateStatic(uiXml, "top_background", m_pSearchLootWnd);
    UIHelper::CreateStatic(uiXml, "bottom_background", m_pSearchLootWnd);

    CUIStatic* actorIcon = UIHelper::CreateStatic(uiXml, "static_icon", 0, m_pSearchLootWnd);
    CUIStatic* partnerIcon = UIHelper::CreateStatic(uiXml, "static_icon", 1, m_pSearchLootWnd);

    m_SearchLootActorCharacterInfo = new CUICharacterInfo();
    m_SearchLootPartnerCharacterInfo = new CUICharacterInfo();

    actorIcon->AttachChild(m_SearchLootActorCharacterInfo);
    m_SearchLootActorCharacterInfo->SetAutoDelete(true);
    m_SearchLootActorCharacterInfo->InitCharacterInfo({ 0.f, 0.f }, actorIcon->GetWndSize(), TRADE_CHARACTER_XML);

    partnerIcon->AttachChild(m_SearchLootPartnerCharacterInfo);
    m_SearchLootPartnerCharacterInfo->SetAutoDelete(true);
    m_SearchLootPartnerCharacterInfo->InitCharacterInfo({ 0.f, 0.f }, partnerIcon->GetWndSize(), TRADE_CHARACTER_XML);

    CUIStatic* actorBagWnd = UIHelper::CreateStatic(uiXml, "our_bag_static", m_pSearchLootWnd);
    CUIStatic* partnerBagWnd = UIHelper::CreateStatic(uiXml, "others_bag_static", m_pSearchLootWnd);

    m_pLists[eSearchLootActorBagList] = UIHelper::CreateDragDropListEx(uiXml, "dragdrop_list_our", actorBagWnd);
    m_pLists[eSearchLootBagList] = UIHelper::CreateDragDropListEx(uiXml, "dragdrop_list_other", partnerBagWnd);

    // Item info
    CUIFrameWindow* descWnd = UIHelper::CreateFrameWindow(uiXml, "frame_window", m_pSearchLootWnd);
    UIHelper::CreateStatic(uiXml, "descr_static", descWnd);
    m_ItemInfoSearchLootMode = new CUIItemInfo();
    m_ItemInfoSearchLootMode->SetAutoDelete(true);
    descWnd->AttachChild(m_ItemInfoSearchLootMode);
    m_ItemInfoSearchLootMode->InitItemInfo({ 0.f, 0.f }, descWnd->GetWndSize(), CARBODY_ITEM_XML);

    m_takeall_button = UIHelper::Create3tButton(uiXml, "take_all_btn", m_pSearchLootWnd);
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

void CUIActorMenu::RegisterCallback(CUIWindow* window, s16 event, const CUIWndCallback::void_function& callback)
{
    if (!window)
        return;
    Register(window);
    AddCallback(window, event, std::forward<const CUIWndCallback::void_function&>(callback));
}

void CUIActorMenu::InitCallbacks()
{
    RegisterCallback(m_trade_button, BUTTON_CLICKED,
        CUIWndCallback::void_function(this, &CUIActorMenu::OnBtnPerformTrade));

    RegisterCallback(m_trade_buy_button, BUTTON_CLICKED,
        CUIWndCallback::void_function(this, &CUIActorMenu::OnBtnPerformTradeBuy));

    RegisterCallback(m_trade_sell_button, BUTTON_CLICKED,
        CUIWndCallback::void_function(this, &CUIActorMenu::OnBtnPerformTradeSell));

    RegisterCallback(m_takeall_button, BUTTON_CLICKED,
        CUIWndCallback::void_function(this, &CUIActorMenu::TakeAllFromPartner));

    RegisterCallback(m_exit_button, BUTTON_CLICKED,
        CUIWndCallback::void_function(this, &CUIActorMenu::OnBtnExitClicked));

    RegisterCallback(m_UIPropertiesBox, PROPERTY_CLICKED,
        CUIWndCallback::void_function(this, &CUIActorMenu::ProcessPropertiesBoxClicked));

    RegisterCallback(m_pUpgradeWnd ? m_pUpgradeWnd->m_btn_repair : nullptr, BUTTON_CLICKED,
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

    BindDragDropListEvents(m_pLists[eSearchLootBagList]);

    if (m_pLists[eTrashList])
    {
        m_pLists[eTrashList]->m_f_item_drop = CUIDragDropListEx::DRAG_CELL_EVENT(this, &CUIActorMenu::OnItemDrop);
        m_pLists[eTrashList]->m_f_drag_event = CUIDragDropListEx::DRAG_ITEM_EVENT(this, &CUIActorMenu::OnDragItemOnTrash);
    }

    BindDragDropListEvents(m_pQuickSlot);
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
}
