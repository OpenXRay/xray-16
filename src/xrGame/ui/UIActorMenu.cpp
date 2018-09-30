#include "StdAfx.h"
#include "UIActorMenu.h"
#include "UIActorStateInfo.h"
#include "Actor.h"
#include "UIGameSP.h"
#include "Inventory.h"
#include "inventory_item.h"
#include "InventoryBox.h"
#include "Common/object_broker.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "UIInventoryUtilities.h"
#include "game_cl_base.h"
#include "Weapon.h"
#include "WeaponMagazinedWGrenade.h"
#include "WeaponAmmo.h"
#include "Silencer.h"
#include "Scope.h"
#include "GrenadeLauncher.h"
#include "trade_parameters.h"
#include "ActorHelmet.h"
#include "CustomOutfit.h"
#include "CustomDetector.h"
#include "eatable_item.h"
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/Cursor/UICursor.h"
#include "UICellItem.h"
#include "UICharacterInfo.h"
#include "UIItemInfo.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "UIInventoryUpgradeWnd.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "xrUICore/Buttons/UIBtnHint.h"
#include "UIMessageBoxEx.h"
#include "xrUICore/PropertiesBox/UIPropertiesBox.h"
#include "UIMainIngameWnd.h"
#include "trade.h"

void CUIActorMenu::SetActor(CInventoryOwner* io)
{
    R_ASSERT(!IsShown());
    m_last_time = Device.dwTimeGlobal;
    m_pActorInvOwner = io;

    if (IsGameTypeSingle())
    {
        if (io)
            m_ActorCharacterInfo->InitCharacter(m_pActorInvOwner->object_id());
        else
            m_ActorCharacterInfo->ClearInfo();
    }
    else
    {
        UpdateActorMP();
    }
}

void CUIActorMenu::SetPartner(CInventoryOwner* io)
{
    R_ASSERT(!IsShown());
    m_pPartnerInvOwner = io;
    if (m_pPartnerInvOwner)
    {
        if (m_pPartnerInvOwner->use_simplified_visual())
            m_PartnerCharacterInfo->ClearInfo();
        else
            m_PartnerCharacterInfo->InitCharacter(m_pPartnerInvOwner->object_id());

        SetInvBox(NULL);
    }
    else
        m_PartnerCharacterInfo->ClearInfo();
}

void CUIActorMenu::SetInvBox(CInventoryBox* box)
{
    R_ASSERT(!IsShown());
    m_pInvBox = box;
    if (box)
    {
        m_pInvBox->set_in_use(true);
        SetPartner(NULL);
    }
}

void CUIActorMenu::SetMenuMode(EMenuMode mode)
{
    SetCurrentItem(NULL);
    m_hint_wnd->set_text(NULL);

    if (mode != m_currMenuMode)
    {
        switch (m_currMenuMode)
        {
        case mmUndefined: break;
        case mmInventory: DeInitInventoryMode(); break;
        case mmTrade: DeInitTradeMode(); break;
        case mmUpgrade: DeInitUpgradeMode(); break;
        case mmDeadBodySearch: DeInitDeadBodySearchMode(); break;
        default: R_ASSERT(0); break;
        }

        CurrentGameUI()->UIMainIngameWnd->ShowZoneMap(false);

        m_currMenuMode = mode;
        switch (mode)
        {
        case mmUndefined:
#ifdef DEBUG
            Msg("* now is Undefined mode");
#endif // #ifdef DEBUG
            ResetMode();
            break;
        case mmInventory: InitInventoryMode();
#ifdef DEBUG
            Msg("* now is Inventory mode");
#endif // #ifdef DEBUG
            break;
        case mmTrade: InitTradeMode();
#ifdef DEBUG
            Msg("* now is Trade mode");
#endif // #ifdef DEBUG
            break;
        case mmUpgrade: InitUpgradeMode();
#ifdef DEBUG
            Msg("* now is Upgrade mode");
#endif // #ifdef DEBUG
            break;
        case mmDeadBodySearch: InitDeadBodySearchMode();
#ifdef DEBUG
            Msg("* now is DeadBodySearch mode");
#endif // #ifdef DEBUG
            break;
        default: R_ASSERT(0); break;
        }
        UpdateConditionProgressBars();
        CurModeToScript();
    } // if

    if (m_pActorInvOwner)
    {
        UpdateOutfit();
        UpdateActor();
    }
    UpdateButtonsLayout();
}

void CUIActorMenu::PlaySnd(eActorMenuSndAction a)
{
    if (sounds[a]._handle())
        sounds[a].play(NULL, sm_2D);
}

void CUIActorMenu::SendMessage(CUIWindow* pWnd, s16 msg, void* pData) { CUIWndCallback::OnEvent(pWnd, msg, pData); }
void CUIActorMenu::Show(bool status)
{
    inherited::Show(status);
    if (status)
    {
        SetMenuMode(m_currMenuMode);
        PlaySnd(eSndOpen);
        m_ActorStateInfo->UpdateActorInfo(m_pActorInvOwner);
    }
    else
    {
        PlaySnd(eSndClose);
        SetMenuMode(mmUndefined);
    }
    m_ActorStateInfo->Show(status);
}

void CUIActorMenu::Draw()
{
    CurrentGameUI()->UIMainIngameWnd->DrawZoneMap();
    CurrentGameUI()->UIMainIngameWnd->DrawMainIndicatorsForInventory();

    inherited::Draw();
    m_ItemInfo->Draw();
    m_hint_wnd->Draw();
}

void CUIActorMenu::Update()
{
    { // all mode
        m_last_time = Device.dwTimeGlobal;
        m_ActorStateInfo->UpdateActorInfo(m_pActorInvOwner);
    }

    switch (m_currMenuMode)
    {
    case mmUndefined: break;
    case mmInventory:
    {
        //			m_clock_value->TextItemControl()->SetText( InventoryUtilities::GetGameTimeAsString(
        // InventoryUtilities::etpTimeToMinutes ).c_str() );
        CurrentGameUI()->UIMainIngameWnd->UpdateZoneMap();
        break;
    }
    case mmTrade:
    {
        if (m_pPartnerInvOwner->inventory().ModifyFrame() != m_trade_partner_inventory_state)
            InitPartnerInventoryContents();
        CheckDistance();
        break;
    }
    case mmUpgrade:
    {
        UpdateUpgradeItem();
        CheckDistance();
        break;
    }
    case mmDeadBodySearch:
    {
        // Alundaio: remove distance check when opening inventory boxes
        //CheckDistance(); 
        break;
    }
    default: R_ASSERT(0); break;
    }

    inherited::Update();
    m_ItemInfo->Update();
    m_hint_wnd->Update();
}

bool CUIActorMenu::StopAnyMove() // true = актёр не идёт при открытом меню
{
    switch (m_currMenuMode)
    {
    case mmInventory: return false;
    case mmUndefined:
    case mmTrade:
    case mmUpgrade:
    case mmDeadBodySearch: return true;
    }
    return true;
}

void CUIActorMenu::CheckDistance()
{
    CGameObject* pActorGO = smart_cast<CGameObject*>(m_pActorInvOwner);
    CGameObject* pPartnerGO = smart_cast<CGameObject*>(m_pPartnerInvOwner);
    CGameObject* pBoxGO = smart_cast<CGameObject*>(m_pInvBox);
    VERIFY(pActorGO && (pPartnerGO || pBoxGO));

    if (pPartnerGO)
    {
        if ((pActorGO->Position().distance_to(pPartnerGO->Position()) > 3.0f) &&
            !m_pPartnerInvOwner->NeedOsoznanieMode())
        {
            g_btnHint->Discard();
            HideDialog();
        }
    }
    else // pBoxGO
    {
        VERIFY(pBoxGO);
        if (pActorGO->Position().distance_to(pBoxGO->Position()) > 3.0f)
        {
            g_btnHint->Discard();
            HideDialog();
        }
    }
}

EDDListType CUIActorMenu::GetListType(CUIDragDropListEx* l)
{
    if (l == m_pInventoryBagList)
        return iActorBag;
    if (l == m_pInventoryBeltList)
        return iActorBelt;

    if (l == m_pInventoryAutomaticList)
        return iActorSlot;
    if (l == m_pInventoryPistolList)
        return iActorSlot;
    if (l == m_pInventoryOutfitList)
        return iActorSlot;
    if (l == m_pInventoryHelmetList)
        return iActorSlot;
    if (l == m_pInventoryDetectorList)
        return iActorSlot;

    if (l == m_pTradeActorBagList)
        return iActorBag;
    if (l == m_pTradeActorList)
        return iActorTrade;
    if (l == m_pTradePartnerBagList)
        return iPartnerTradeBag;
    if (l == m_pTradePartnerList)
        return iPartnerTrade;
    if (l == m_pDeadBodyBagList)
        return iDeadBodyBag;

    if (l == m_pQuickSlot)
        return iQuickSlot;
    if (l == m_pTrashList)
        return iTrashSlot;

    R_ASSERT(0);

    return iInvalid;
}

CUIDragDropListEx* CUIActorMenu::GetListByType(EDDListType t)
{
    switch (t)
    {
    case iActorBag:
    {
        if (m_currMenuMode == mmTrade)
            return m_pTradeActorBagList;
        else
            return m_pInventoryBagList;
    }
    break;
    case iDeadBodyBag: { return m_pDeadBodyBagList;
    }
    break;
    case iActorBelt: { return m_pInventoryBeltList;
    }
    break;
    default: { R_ASSERT("invalid call");
    }
    break;
    }
    return NULL;
}

CUICellItem* CUIActorMenu::CurrentItem() { return m_pCurrentCellItem; }
PIItem CUIActorMenu::CurrentIItem() { return (m_pCurrentCellItem) ? (PIItem)m_pCurrentCellItem->m_pData : NULL; }
void CUIActorMenu::SetCurrentItem(CUICellItem* itm)
{
    m_repair_mode = false;
    m_pCurrentCellItem = itm;
    if (!itm)
    {
        InfoCurItem(NULL);
    }
    TryHidePropertiesBox();

    if (m_currMenuMode == mmUpgrade)
    {
        SetupUpgradeItem();
    }
}

void CUIActorMenu::InfoCurItem(CUICellItem* cell_item)
{
    if (!cell_item)
    {
        m_ItemInfo->InitItem(NULL);
        return;
    }
    PIItem current_item = (PIItem)cell_item->m_pData;

    PIItem compare_item = NULL;
    u16 compare_slot = current_item->BaseSlot();
    if (compare_slot != NO_ACTIVE_SLOT)
    {
        compare_item = m_pActorInvOwner->inventory().ItemFromSlot(compare_slot);
    }

    if (GetMenuMode() == mmTrade)
    {
        CInventoryOwner* item_owner = smart_cast<CInventoryOwner*>(current_item->m_pInventory->GetOwner());
        u32 item_price = u32(-1);
        if (item_owner && item_owner == m_pActorInvOwner)
            item_price = m_partner_trade->GetItemPrice(current_item, true);
        else
            item_price = m_partner_trade->GetItemPrice(current_item, false);

        // if(item_price>500)
        //	item_price = iFloor(item_price/10+0.5f)*10;

        CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(current_item);
        if (ammo)
        {
            for (u32 j = 0; j < cell_item->ChildsCount(); ++j)
            {
                u32 tmp_price = 0;
                PIItem jitem = (PIItem)cell_item->Child(j)->m_pData;
                CInventoryOwner* ammo_owner = smart_cast<CInventoryOwner*>(jitem->m_pInventory->GetOwner());
                if (ammo_owner && ammo_owner == m_pActorInvOwner)
                    tmp_price = m_partner_trade->GetItemPrice(jitem, true);
                else
                    tmp_price = m_partner_trade->GetItemPrice(jitem, false);

                // if(tmp_price>500)
                //	tmp_price = iFloor(tmp_price/10+0.5f)*10;

                item_price += tmp_price;
            }
        }

        if (!current_item->CanTrade() || (!m_pPartnerInvOwner->trade_parameters().enabled(
                                              CTradeParameters::action_buy(0), current_item->object().cNameSect()) &&
                                             item_owner && item_owner == m_pActorInvOwner))
            m_ItemInfo->InitItem(cell_item, compare_item, u32(-1), "st_no_trade_tip_1");
        else if (current_item->GetCondition() < m_pPartnerInvOwner->trade_parameters().buy_item_condition_factor)
            m_ItemInfo->InitItem(cell_item, compare_item, u32(-1), "st_no_trade_tip_2");
        else
            m_ItemInfo->InitItem(cell_item, compare_item, item_price);
    }
    else
        m_ItemInfo->InitItem(cell_item, compare_item, u32(-1));

    //	m_ItemInfo->InitItem	( current_item, compare_item );
    float dx_pos = GetWndRect().left;
    fit_in_rect(m_ItemInfo, Frect().set(0.0f, 0.0f, UI_BASE_WIDTH - dx_pos, UI_BASE_HEIGHT), 10.0f, dx_pos);
}

void CUIActorMenu::UpdateItemsPlace()
{
    switch (m_currMenuMode)
    {
    case mmUndefined: break;
    case mmInventory: break;
    case mmTrade: UpdatePrices(); break;
    case mmUpgrade: SetupUpgradeItem(); break;
    case mmDeadBodySearch: UpdateDeadBodyBag(); break;
    default: R_ASSERT(0); break;
    }

    if (m_pActorInvOwner)
    {
        UpdateOutfit();
        UpdateActor();
    }
}

// ================================================================

void CUIActorMenu::clear_highlight_lists()
{
    m_InvSlot2Highlight->Show(false);
    m_InvSlot3Highlight->Show(false);
    m_HelmetSlotHighlight->Show(false);
    m_OutfitSlotHighlight->Show(false);
    m_DetectorSlotHighlight->Show(false);
    for (u8 i = 0; i < 4; i++)
        m_QuickSlotsHighlight[i]->Show(false);
    for (u8 i = 0; i < e_af_count; i++)
        m_ArtefactSlotsHighlight[i]->Show(false);

    m_pInventoryBagList->clear_select_armament();

    switch (m_currMenuMode)
    {
    case mmUndefined: break;
    case mmInventory: break;
    case mmTrade:
        m_pTradeActorBagList->clear_select_armament();
        m_pTradeActorList->clear_select_armament();
        m_pTradePartnerBagList->clear_select_armament();
        m_pTradePartnerList->clear_select_armament();
        break;
    case mmUpgrade: break;
    case mmDeadBodySearch: m_pDeadBodyBagList->clear_select_armament(); break;
    }
    m_highlight_clear = true;
}
void CUIActorMenu::highlight_item_slot(CUICellItem* cell_item)
{
    PIItem item = (PIItem)cell_item->m_pData;
    if (!item)
        return;

    if (CUIDragDropListEx::m_drag_item)
        return;

    CWeapon* weapon = smart_cast<CWeapon*>(item);
    CHelmet* helmet = smart_cast<CHelmet*>(item);
    CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(item);
    CCustomDetector* detector = smart_cast<CCustomDetector*>(item);
    CEatableItem* eatable = smart_cast<CEatableItem*>(item);
    CArtefact* artefact = smart_cast<CArtefact*>(item);

    u16 slot_id = item->BaseSlot();
    if (weapon && (slot_id == INV_SLOT_2 || slot_id == INV_SLOT_3))
    {
        m_InvSlot2Highlight->Show(true);
        m_InvSlot3Highlight->Show(true);
        return;
    }
    if (helmet && slot_id == HELMET_SLOT)
    {
        m_HelmetSlotHighlight->Show(true);
        return;
    }
    if (outfit && slot_id == OUTFIT_SLOT)
    {
        m_OutfitSlotHighlight->Show(true);
        return;
    }
    if (detector && slot_id == DETECTOR_SLOT)
    {
        m_DetectorSlotHighlight->Show(true);
        return;
    }
    if (eatable)
    {
        if (cell_item->OwnerList() && GetListType(cell_item->OwnerList()) == iQuickSlot)
            return;

        for (u8 i = 0; i < 4; i++)
            m_QuickSlotsHighlight[i]->Show(true);
        return;
    }
    if (artefact)
    {
        if (cell_item->OwnerList() && GetListType(cell_item->OwnerList()) == iActorBelt)
            return;

        Ivector2 cap = m_pInventoryBeltList->CellsCapacity();
        for (u8 i = 0; i < cap.x; i++)
            m_ArtefactSlotsHighlight[i]->Show(true);
        return;
    }
}
void CUIActorMenu::set_highlight_item(CUICellItem* cell_item)
{
    PIItem item = (PIItem)cell_item->m_pData;
    if (!item)
    {
        return;
    }
    highlight_item_slot(cell_item);

    switch (m_currMenuMode)
    {
    case mmUndefined:
    case mmInventory:
    case mmUpgrade:
    {
        highlight_armament(item, m_pInventoryBagList);
        break;
    }
    case mmTrade:
    {
        highlight_armament(item, m_pTradeActorBagList);
        highlight_armament(item, m_pTradeActorList);
        highlight_armament(item, m_pTradePartnerBagList);
        highlight_armament(item, m_pTradePartnerList);
        break;
    }
    case mmDeadBodySearch:
    {
        highlight_armament(item, m_pInventoryBagList);
        highlight_armament(item, m_pDeadBodyBagList);
        break;
    }
    }
    m_highlight_clear = false;
}

void CUIActorMenu::highlight_armament(PIItem item, CUIDragDropListEx* ddlist)
{
    ddlist->clear_select_armament();
    highlight_ammo_for_weapon(item, ddlist);
    highlight_weapons_for_ammo(item, ddlist);
    highlight_weapons_for_addon(item, ddlist);
}

void CUIActorMenu::highlight_ammo_for_weapon(PIItem weapon_item, CUIDragDropListEx* ddlist)
{
    VERIFY(weapon_item);
    VERIFY(ddlist);
    static xr_vector<shared_str> ammo_types;
    ammo_types.clear();

    CWeapon* weapon = smart_cast<CWeapon*>(weapon_item);
    if (!weapon)
    {
        return;
    }
    ammo_types.assign(weapon->m_ammoTypes.begin(), weapon->m_ammoTypes.end());

    CWeaponMagazinedWGrenade* wg = smart_cast<CWeaponMagazinedWGrenade*>(weapon_item);
    if (wg)
    {
        if (wg->IsGrenadeLauncherAttached() && wg->m_ammoTypes2.size())
        {
            ammo_types.insert(ammo_types.end(), wg->m_ammoTypes2.begin(), wg->m_ammoTypes2.end());
        }
    }

    if (ammo_types.size() == 0)
    {
        return;
    }
    xr_vector<shared_str>::iterator ite = ammo_types.end();

    u32 const cnt = ddlist->ItemsCount();
    for (u32 i = 0; i < cnt; ++i)
    {
        CUICellItem* ci = ddlist->GetItemIdx(i);
        PIItem item = (PIItem)ci->m_pData;
        if (!item)
        {
            continue;
        }
        CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(item);
        if (!ammo)
        {
            highlight_addons_for_weapon(weapon_item, ci);
            continue; // for i
        }
        shared_str const& ammo_name = item->object().cNameSect();

        xr_vector<shared_str>::iterator itb = ammo_types.begin();
        for (; itb != ite; ++itb)
        {
            if (ammo_name._get() == (*itb)._get())
            {
                ci->m_select_armament = true;
                break; // itb
            }
        }
    } // for i
}

void CUIActorMenu::highlight_weapons_for_ammo(PIItem ammo_item, CUIDragDropListEx* ddlist)
{
    VERIFY(ammo_item);
    VERIFY(ddlist);
    CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(ammo_item);
    if (!ammo)
    {
        return;
    }

    shared_str const& ammo_name = ammo_item->object().cNameSect();

    u32 const cnt = ddlist->ItemsCount();
    for (u32 i = 0; i < cnt; ++i)
    {
        CUICellItem* ci = ddlist->GetItemIdx(i);
        PIItem item = (PIItem)ci->m_pData;
        if (!item)
        {
            continue;
        }
        CWeapon* weapon = smart_cast<CWeapon*>(item);
        if (!weapon)
        {
            continue;
        }

        xr_vector<shared_str>::iterator itb = weapon->m_ammoTypes.begin();
        xr_vector<shared_str>::iterator ite = weapon->m_ammoTypes.end();
        for (; itb != ite; ++itb)
        {
            if (ammo_name._get() == (*itb)._get())
            {
                ci->m_select_armament = true;
                break; // for itb
            }
        }

        CWeaponMagazinedWGrenade* wg = smart_cast<CWeaponMagazinedWGrenade*>(item);
        if (!wg || !wg->IsGrenadeLauncherAttached() || !wg->m_ammoTypes2.size())
        {
            continue; // for i
        }
        itb = wg->m_ammoTypes2.begin();
        ite = wg->m_ammoTypes2.end();
        for (; itb != ite; ++itb)
        {
            if (ammo_name._get() == (*itb)._get())
            {
                ci->m_select_armament = true;
                break; // for itb
            }
        }
    } // for i
}

bool CUIActorMenu::highlight_addons_for_weapon(PIItem weapon_item, CUICellItem* ci)
{
    PIItem item = (PIItem)ci->m_pData;
    if (!item)
    {
        return false;
    }

    CScope* pScope = smart_cast<CScope*>(item);
    if (pScope && weapon_item->CanAttach(pScope))
    {
        ci->m_select_armament = true;
        return true;
    }

    CSilencer* pSilencer = smart_cast<CSilencer*>(item);
    if (pSilencer && weapon_item->CanAttach(pSilencer))
    {
        ci->m_select_armament = true;
        return true;
    }

    CGrenadeLauncher* pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(item);
    if (pGrenadeLauncher && weapon_item->CanAttach(pGrenadeLauncher))
    {
        ci->m_select_armament = true;
        return true;
    }
    return false;
}

void CUIActorMenu::highlight_weapons_for_addon(PIItem addon_item, CUIDragDropListEx* ddlist)
{
    VERIFY(addon_item);
    VERIFY(ddlist);

    CScope* pScope = smart_cast<CScope*>(addon_item);
    CSilencer* pSilencer = smart_cast<CSilencer*>(addon_item);
    CGrenadeLauncher* pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(addon_item);

    if (!pScope && !pSilencer && !pGrenadeLauncher)
    {
        return;
    }

    u32 const cnt = ddlist->ItemsCount();
    for (u32 i = 0; i < cnt; ++i)
    {
        CUICellItem* ci = ddlist->GetItemIdx(i);
        PIItem item = (PIItem)ci->m_pData;
        if (!item)
        {
            continue;
        }
        CWeapon* weapon = smart_cast<CWeapon*>(item);
        if (!weapon)
        {
            continue;
        }

        if (pScope && weapon->CanAttach(pScope))
        {
            ci->m_select_armament = true;
            continue;
        }
        if (pSilencer && weapon->CanAttach(pSilencer))
        {
            ci->m_select_armament = true;
            continue;
        }
        if (pGrenadeLauncher && weapon->CanAttach(pGrenadeLauncher))
        {
            ci->m_select_armament = true;
            continue;
        }

    } // for i
}

// -------------------------------------------------------------------

void CUIActorMenu::ClearAllLists()
{
    m_pInventoryBagList->ClearAll(true);

    m_pInventoryBeltList->ClearAll(true);
    m_pInventoryOutfitList->ClearAll(true);
    m_pInventoryHelmetList->ClearAll(true);
    m_pInventoryDetectorList->ClearAll(true);
    m_pInventoryPistolList->ClearAll(true);
    m_pInventoryAutomaticList->ClearAll(true);
    m_pQuickSlot->ClearAll(true);

    m_pTradeActorBagList->ClearAll(true);
    m_pTradeActorList->ClearAll(true);
    m_pTradePartnerBagList->ClearAll(true);
    m_pTradePartnerList->ClearAll(true);
    m_pDeadBodyBagList->ClearAll(true);
}

void CUIActorMenu::CallMessageBoxYesNo(LPCSTR text)
{
    m_message_box_yes_no->SetText(text);
    m_message_box_yes_no->func_on_ok = CUIWndCallback::void_function(this, &CUIActorMenu::OnMesBoxYes);
    m_message_box_yes_no->func_on_no = CUIWndCallback::void_function(this, &CUIActorMenu::OnMesBoxNo);
    m_message_box_yes_no->ShowDialog(false);
}

void CUIActorMenu::CallMessageBoxOK(LPCSTR text)
{
    m_message_box_ok->SetText(text);
    m_message_box_ok->ShowDialog(false);
}

void CUIActorMenu::ResetMode()
{
    ClearAllLists();
    m_pMouseCapturer = NULL;
    m_UIPropertiesBox->Hide();
    SetCurrentItem(NULL);
}

void CUIActorMenu::UpdateActorMP()
{
    if (!&Level() || !Level().game || !Game().local_player || !m_pActorInvOwner || IsGameTypeSingle())
    {
        m_ActorCharacterInfo->ClearInfo();
        m_ActorMoney->SetText("");
        return;
    }

    int money = Game().local_player->money_for_round;

    string64 buf;
    xr_sprintf(buf, "%d RU", money);
    m_ActorMoney->SetText(buf);

    m_ActorCharacterInfo->InitCharacterMP(Game().local_player->getName(), "ui_npc_u_nebo_1");
}

bool CUIActorMenu::CanSetItemToList(PIItem item, CUIDragDropListEx* l, u16& ret_slot)
{
    u16 item_slot = item->BaseSlot();
    if (GetSlotList(item_slot) == l)
    {
        ret_slot = item_slot;
        return true;
    }

    if (item_slot == INV_SLOT_3 && l == m_pInventoryPistolList)
    {
        ret_slot = INV_SLOT_2;
        return true;
    }

    if (item_slot == INV_SLOT_2 && l == m_pInventoryAutomaticList)
    {
        ret_slot = INV_SLOT_3;
        return true;
    }

    return false;
}
void CUIActorMenu::UpdateConditionProgressBars()
{
    PIItem itm = m_pActorInvOwner->inventory().ItemFromSlot(INV_SLOT_2);
    if (itm)
    {
        m_WeaponSlot1_progress->SetProgressPos(iCeil(itm->GetCondition() * 15.0f) / 15.0f);
    }
    else
        m_WeaponSlot1_progress->SetProgressPos(0);

    itm = m_pActorInvOwner->inventory().ItemFromSlot(INV_SLOT_3);
    if (itm)
        m_WeaponSlot2_progress->SetProgressPos(iCeil(itm->GetCondition() * 15.0f) / 15.0f);
    else
        m_WeaponSlot2_progress->SetProgressPos(0);

    itm = m_pActorInvOwner->inventory().ItemFromSlot(OUTFIT_SLOT);
    if (itm)
        m_Outfit_progress->SetProgressPos(iCeil(itm->GetCondition() * 15.0f) / 15.0f);
    else
        m_Outfit_progress->SetProgressPos(0);

    itm = m_pActorInvOwner->inventory().ItemFromSlot(HELMET_SLOT);
    if (itm)
        m_Helmet_progress->SetProgressPos(iCeil(itm->GetCondition() * 15.0f) / 15.0f);
    else
        m_Helmet_progress->SetProgressPos(0);
}
