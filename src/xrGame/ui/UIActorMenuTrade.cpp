#include "pch_script.h"
#include "UIActorMenu.h"
#include "UITradeBar.h"
#include "UIWeightBar.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "UICharacterInfo.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "UICellItem.h"
#include "UIInventoryUtilities.h"
#include "UICellItemFactory.h"
#include "InventoryOwner.h"
#include "Inventory.h"
#include "trade.h"
#include "Entity.h"
#include "Actor.h"
#include "Weapon.h"
#include "trade_parameters.h"
#include "inventory_item_object.h"
#include "string_table.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "UIGameSP.h"
#include "UITalkWnd.h"

void CUIActorMenu::InitTradeMode()
{
    ShowIfExist(m_pTradeWnd, true);
    m_pLists[eInventoryBagList]->Show(false);
    GetModeSpecificPartnerInfo()->Show(true);
    m_PartnerMoney->Show(true);
    ShowIfExist(m_pQuickSlot, true);

    m_pLists[eTradeActorBagList]->Show(true);
    m_pLists[eTradeActorList]->Show(true);
    m_pLists[eTradePartnerBagList]->Show(true);
    m_pLists[eTradePartnerList]->Show(true);

    m_ActorTradeBar->Show(true);
    m_PartnerTradeBar->Show(true);
    ShowIfExist(m_LeftBackground, true);

    m_PartnerWeightBar->Show(true);
    ShowIfExist(m_trade_button, true);
    ShowIfExist(m_trade_buy_button, true);
    ShowIfExist(m_trade_sell_button, true);

    VERIFY(m_pPartnerInvOwner);
    m_pPartnerInvOwner->StartTrading();

    InitInventoryContents(m_pLists[eTradeActorBagList]);
    InitPartnerInventoryContents();

    m_actor_trade = m_pActorInvOwner->GetTrade();
    m_partner_trade = m_pPartnerInvOwner->GetTrade();
    VERIFY(m_actor_trade);
    VERIFY(m_partner_trade);
    m_actor_trade->StartTradeEx(m_pPartnerInvOwner);
    m_partner_trade->StartTradeEx(m_pActorInvOwner);

    UpdatePrices();
}
bool is_item_in_list(CUIDragDropListEx* pList, PIItem item)
{
    for (u16 i = 0; i < pList->ItemsCount(); i++)
    {
        CUICellItem* cell_item = pList->GetItemIdx(i);
        for (u16 k = 0; k < cell_item->ChildsCount(); k++)
        {
            CUICellItem* inv_cell_item = cell_item->Child(k);
            if ((PIItem)inv_cell_item->m_pData == item)
                return true;
        }
        if ((PIItem)cell_item->m_pData == item)
            return true;
    }
    return false;
}

void CUIActorMenu::InitPartnerInventoryContents()
{
    m_pLists[eTradePartnerBagList]->ClearAll(true);

    TIItemContainer items_list;
    m_pPartnerInvOwner->inventory().AddAvailableItems(items_list, true);
    std::sort(items_list.begin(), items_list.end(), InventoryUtilities::GreaterRoomInRuck);

    TIItemContainer::iterator itb = items_list.begin();
    TIItemContainer::iterator ite = items_list.end();
    for (; itb != ite; ++itb)
    {
        if (!is_item_in_list(m_pLists[eTradePartnerList], *itb))
        {
            CUICellItem* itm = create_cell_item(*itb);
            m_pLists[eTradePartnerBagList]->SetItem(itm);
        }
    }
    m_trade_partner_inventory_state = m_pPartnerInvOwner->inventory().ModifyFrame();
}

void CUIActorMenu::ColorizeItem(CUICellItem* itm, bool colorize)
{
    if (colorize)
    {
        itm->SetTextureColor(color_rgba(255, 100, 100, 255));
    }
    else
    {
        itm->SetTextureColor(color_rgba(255, 255, 255, 255));
    }
}

void CUIActorMenu::DeInitTradeMode()
{
    if (m_actor_trade)
    {
        m_actor_trade->StopTrade();
    }
    if (m_partner_trade)
    {
        m_partner_trade->StopTrade();
    }
    if (m_pPartnerInvOwner)
    {
        m_pPartnerInvOwner->StopTrading();
    }

    ShowIfExist(m_pTradeWnd, false);
    m_pLists[eInventoryBagList]->Show(true);
    GetModeSpecificPartnerInfo()->Show(false);
    m_PartnerMoney->Show(false);

    m_pLists[eTradeActorBagList]->Show(false);
    m_pLists[eTradeActorList]->Show(false);
    m_pLists[eTradePartnerBagList]->Show(false);
    m_pLists[eTradePartnerList]->Show(false);

    m_ActorTradeBar->Show(false);
    m_PartnerTradeBar->Show(false);
    ShowIfExist(m_LeftBackground, false);

    m_PartnerWeightBar->Show(false);
    ShowIfExist(m_trade_button, false);
    ShowIfExist(m_trade_buy_button, false);
    ShowIfExist(m_trade_sell_button, false);

    if (!CurrentGameUI())
        return;

    if (CurrentGameUI())
    {
        CurrentGameUI()->RemoveCustomStatic("not_enough_money_mine");
        CurrentGameUI()->RemoveCustomStatic("not_enough_money_other");
    }

    //только если находимся в режиме single
    CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
    if (!pGameSP)
        return;

    if (pGameSP->TalkMenu->IsShown())
    {
        pGameSP->TalkMenu->NeedUpdateQuestions();
    }
}

bool CUIActorMenu::ToActorTrade(CUICellItem* itm, bool b_use_cursor_pos)
{
    PIItem iitem = (PIItem)itm->m_pData;
    if (!CanMoveToPartner(iitem))
    {
        return false;
    }

    //	if(m_pActorInvOwner->inventory().CanPutInRuck(iitem))
    {
        CUIDragDropListEx* old_owner = itm->OwnerList();
        CUIDragDropListEx* new_owner = NULL;
        EDDListType old_owner_type = GetListType(old_owner);
        if (old_owner_type == iQuickSlot)
            return false;

        if (b_use_cursor_pos)
        {
            new_owner = CUIDragDropListEx::m_drag_item->BackList();
            VERIFY(new_owner == m_pLists[eTradeActorList]);
        }
        else
            new_owner = m_pLists[eTradeActorList];

        bool result = (old_owner_type != iActorBag) ? m_pActorInvOwner->inventory().Ruck(iitem) : true;
        VERIFY(result);
        CUICellItem* i = old_owner->RemoveItem(itm, (old_owner == new_owner));

        if (b_use_cursor_pos)
            new_owner->SetItem(i, old_owner->GetDragItemPosition());
        else
            new_owner->SetItem(i);

        if (old_owner_type != iActorBag)
        {
            SendEvent_Item2Ruck(iitem, m_pActorInvOwner->object_id());
        }
        return true;
    }
}

bool CUIActorMenu::ToPartnerTrade(CUICellItem* itm, bool b_use_cursor_pos)
{
    PIItem iitem = (PIItem)itm->m_pData;
    SInvItemPlace pl;
    pl.type = eItemPlaceRuck;
    if (!m_pPartnerInvOwner->AllowItemToTrade(iitem, pl))
    {
        /// R_ASSERT2( 0, make_string( "Partner can`t cell item (%s)", iitem->NameItem() ) );
        Msg("! Partner can`t cell item (%s)", iitem->NameItem());
        return false;
    }

    CUIDragDropListEx* old_owner = itm->OwnerList();
    CUIDragDropListEx* new_owner = NULL;

    if (b_use_cursor_pos)
    {
        new_owner = CUIDragDropListEx::m_drag_item->BackList();
        VERIFY(new_owner == m_pLists[eTradePartnerList]);
    }
    else
        new_owner = m_pLists[eTradePartnerList];

    CUICellItem* i = old_owner->RemoveItem(itm, (old_owner == new_owner));

    if (b_use_cursor_pos)
        new_owner->SetItem(i, old_owner->GetDragItemPosition());
    else
        new_owner->SetItem(i);

    UpdatePrices();
    return true;
}

bool CUIActorMenu::ToPartnerTradeBag(CUICellItem* itm, bool b_use_cursor_pos)
{
    CUIDragDropListEx* old_owner = itm->OwnerList();
    CUIDragDropListEx* new_owner = NULL;

    if (b_use_cursor_pos)
    {
        new_owner = CUIDragDropListEx::m_drag_item->BackList();
        VERIFY(new_owner == m_pLists[eTradePartnerBagList]);
    }
    else
        new_owner = m_pLists[eTradePartnerBagList];

    CUICellItem* i = old_owner->RemoveItem(itm, (old_owner == new_owner));

    if (b_use_cursor_pos)
        new_owner->SetItem(i, old_owner->GetDragItemPosition());
    else
        new_owner->SetItem(i);

    return true;
}

float CUIActorMenu::CalcItemsWeight(CUIDragDropListEx* pList)
{
    float res = 0.0f;

    for (u32 i = 0; i < pList->ItemsCount(); ++i)
    {
        CUICellItem* itm = pList->GetItemIdx(i);
        PIItem iitem = (PIItem)itm->m_pData;
        res += iitem->Weight();
        for (u32 j = 0; j < itm->ChildsCount(); ++j)
        {
            PIItem jitem = (PIItem)itm->Child(j)->m_pData;
            res += jitem->Weight();
        }
    }
    return res;
}

u32 CUIActorMenu::CalcItemsPrice(CUIDragDropListEx* pList, CTrade* pTrade, bool bBuying)
{
    u32 res = 0;
    for (u32 i = 0; i < pList->ItemsCount(); ++i)
    {
        CUICellItem* itm = pList->GetItemIdx(i);
        PIItem iitem = (PIItem)itm->m_pData;
        res += pTrade->GetItemPrice(iitem, bBuying);
        for (u32 j = 0; j < itm->ChildsCount(); ++j)
        {
            PIItem jitem = (PIItem)itm->Child(j)->m_pData;
            res += pTrade->GetItemPrice(jitem, bBuying);
        }
    }

    return res;
}

bool CUIActorMenu::CanMoveToPartner(PIItem pItem)
{
    if (!pItem->CanTrade())
        return false;

    if (!m_pPartnerInvOwner->trade_parameters().enabled(CTradeParameters::action_buy(0), pItem->object().cNameSect()))
    {
        return false;
    }

    if (pItem->GetCondition() < m_pPartnerInvOwner->trade_parameters().buy_item_condition_factor)
        return false;

    float r1 = CalcItemsWeight(m_pLists[eTradeActorList]); // actor
    float r2 = CalcItemsWeight(m_pLists[eTradePartnerList]); // partner
    float itmWeight = pItem->Weight();
    float partner_inv_weight = m_pPartnerInvOwner->inventory().CalcTotalWeight();
    float partner_max_weight = m_pPartnerInvOwner->MaxCarryWeight();

    if (partner_inv_weight - r2 + r1 + itmWeight > partner_max_weight)
    {
        return false;
    }
    return true;
}

void CUIActorMenu::UpdateActor()
{
    if (IsGameTypeSingle())
    {
        string64 buf;
        xr_sprintf(buf, "%d RU", m_pActorInvOwner->get_money());
        m_ActorMoney->SetText(buf);
        if (m_ActorMoney != m_TradeActorMoney)
            m_TradeActorMoney->SetText(buf);
    }
    else
    {
        UpdateActorMP();
    }

    CActor* actor = smart_cast<CActor*>(m_pActorInvOwner);
    if (actor)
    {
        CWeapon* wp = smart_cast<CWeapon*>(actor->inventory().ActiveItem());
        if (wp)
        {
            wp->ForceUpdateAmmo();
        }
    } // actor

    m_ActorWeightBar->UpdateData(m_pActorInvOwner);
}

void CUIActorMenu::UpdatePartnerBag()
{
    /*CBaseMonster* monster = smart_cast<CBaseMonster*>(m_pPartnerInvOwner);
    if (monster || m_pPartnerInvOwner->use_simplified_visual())
    {
        m_PartnerWeight->SetText("");
    }
    else*/ if (m_pPartnerInvOwner->InfinitiveMoney())
    {
        m_PartnerMoney->SetText("--- RU");
    }
    else
    {
        string64 buf;
        xr_sprintf(buf, "%d RU", m_pPartnerInvOwner->get_money());
        m_PartnerMoney->SetText(buf);
    }

    const float total = CalcItemsWeight(m_pLists[eTradePartnerBagList]);
    m_PartnerWeightBar->UpdateData(total);
}

void CUIActorMenu::UpdatePrices()
{
    UpdateActor();
    UpdatePartnerBag();

    const u32 actor_price = CalcItemsPrice(m_pLists[eTradeActorList], m_partner_trade, true);
    const u32 partner_price = CalcItemsPrice(m_pLists[eTradePartnerList], m_partner_trade, false);

    const float actor_weight = CalcItemsWeight(m_pLists[eTradeActorList]);
    const float partner_weight = CalcItemsWeight(m_pLists[eTradePartnerList]);

    m_ActorTradeBar->UpdateData(actor_price, actor_weight);
    m_PartnerTradeBar->UpdateData(partner_price, partner_weight);
}

void CUIActorMenu::OnBtnPerformTrade(CUIWindow* w, void* d)
{
    if (m_pLists[eTradeActorList]->ItemsCount() == 0 && m_pLists[eTradePartnerList]->ItemsCount() == 0)
    {
        return;
    }

    int actor_money = (int)m_pActorInvOwner->get_money();
    int partner_money = (int)m_pPartnerInvOwner->get_money();
    int actor_price = (int)CalcItemsPrice(m_pLists[eTradeActorList], m_partner_trade, true);
    int partner_price = (int)CalcItemsPrice(m_pLists[eTradePartnerList], m_partner_trade, false);

    int delta_price = actor_price - partner_price;
    actor_money += delta_price;
    partner_money -= delta_price;

    if ((actor_money >= 0) && (partner_money >= 0) && (actor_price >= 0 || partner_price > 0))
    {
        m_partner_trade->OnPerformTrade(partner_price, actor_price);

        TransferItems(m_pLists[eTradeActorList], m_pLists[eTradePartnerBagList], m_partner_trade, true);
        TransferItems(m_pLists[eTradePartnerList], m_pLists[eTradeActorBagList], m_partner_trade, false);
    }
    else
    {
        if (actor_money < 0)
        {
            ShowMessage("not_enough_money_actor", "not_enough_money_mine", 2.0f);
        }
        else if (partner_money < 0)
        {
            ShowMessage("not_enough_money_partner", "not_enough_money_other", 2.0f);
        }
        else
        {
            ShowMessage("trade_dont_make", "trade_dont_make", 2.0f);
        }
    }
    SetCurrentItem(nullptr);

    UpdateItemsPlace();
}

void CUIActorMenu::OnBtnPerformTradeBuy(CUIWindow* w, void* d)
{
    if (m_pLists[eTradePartnerList]->ItemsCount() == 0)
    {
        return;
    }

    int actor_money = (int)m_pActorInvOwner->get_money();
    int partner_money = (int)m_pPartnerInvOwner->get_money();
    int actor_price = 0; //(int)CalcItemsPrice( m_pLists[eTradeActorList],   m_partner_trade, true  );
    int partner_price = (int)CalcItemsPrice(m_pLists[eTradePartnerList], m_partner_trade, false);

    int delta_price = actor_price - partner_price;
    actor_money += delta_price;
    partner_money -= delta_price;

    if ((actor_money >= 0) /*&& ( partner_money >= 0 )*/ && (actor_price >= 0 || partner_price > 0))
    {
        m_partner_trade->OnPerformTrade(partner_price, actor_price);

        //		TransferItems( m_pLists[eTradeActorList],   m_pLists[eTradePartnerBagList], m_partner_trade, true );
        TransferItems(m_pLists[eTradePartnerList], m_pLists[eTradeActorBagList], m_partner_trade, false);
    }
    else
    {
        if (actor_money < 0)
        {
            ShowMessage("not_enough_money_actor", "not_enough_money_mine", 2.0f);
        }
        // else if ( partner_money < 0 )
        //{
        //	ShowMessage( "not_enough_money_partner", "not_enough_money_other", 2.0f );
        //}
        else
        {
            ShowMessage("trade_dont_make", "trade_dont_make", 2.0f);
        }
    }
    SetCurrentItem(NULL);

    UpdateItemsPlace();
}
void CUIActorMenu::OnBtnPerformTradeSell(CUIWindow* w, void* d)
{
    if (m_pLists[eTradeActorList]->ItemsCount() == 0)
    {
        return;
    }

    int actor_money = (int)m_pActorInvOwner->get_money();
    int partner_money = (int)m_pPartnerInvOwner->get_money();
    int actor_price = (int)CalcItemsPrice(m_pLists[eTradeActorList], m_partner_trade, true);
    int partner_price = 0; //(int)CalcItemsPrice( m_pLists[eTradePartnerList], m_partner_trade, false );

    int delta_price = actor_price - partner_price;
    actor_money += delta_price;
    partner_money -= delta_price;

    if ((actor_money >= 0) && (partner_money >= 0) && (actor_price >= 0 || partner_price > 0))
    {
        m_partner_trade->OnPerformTrade(partner_price, actor_price);

        TransferItems(m_pLists[eTradeActorList], m_pLists[eTradePartnerBagList], m_partner_trade, true);
        //		TransferItems( m_pLists[eTradePartnerList],	m_pLists[eTradeActorBagList],	m_partner_trade, false );
    }
    else
    {
        /*		if ( actor_money < 0 )
		{
			ShowMessage( "not_enough_money_actor", "not_enough_money_mine", 2.0f );
		}
		else */ if (partner_money < 0)
        {
            ShowMessage("not_enough_money_partner", "not_enough_money_other", 2.0f);
        }
        else
        {
            ShowMessage("trade_dont_make", "trade_dont_make", 2.0f);
        }
    }
    SetCurrentItem(NULL);

    UpdateItemsPlace();
}

void CUIActorMenu::TransferItems(
    CUIDragDropListEx* pSellList, CUIDragDropListEx* pBuyList, CTrade* pTrade, bool bBuying)
{
    while (pSellList->ItemsCount())
    {
        CUICellItem* cell_item = pSellList->RemoveItem(pSellList->GetItemIdx(0), false);
        PIItem item = (PIItem)cell_item->m_pData;
        pTrade->TransferItem(item, bBuying);

        if (bBuying)
        {
            SInvItemPlace pl;
            pl.type = eItemPlaceRuck;
            if (pTrade->pThis.inv_owner->CInventoryOwner::AllowItemToTrade(item, pl))
            {
                pBuyList->SetItem(cell_item);
            }
        }
        else
        {
            pBuyList->SetItem(cell_item);
        }
    }
    pTrade->pThis.inv_owner->set_money(pTrade->pThis.inv_owner->get_money(), true);
    pTrade->pPartner.inv_owner->set_money(pTrade->pPartner.inv_owner->get_money(), true);
}

//Alundaio: Donate current item while in trade menu
void CUIActorMenu::DonateCurrentItem(CUICellItem* cell_item)
{
    if (!m_partner_trade || !m_pLists[eTradePartnerList])
        return;

    CUIDragDropListEx* invlist = GetListByType(iActorBag);
    if (!invlist->IsOwner(cell_item))
        return;

    PIItem item = static_cast<PIItem>(cell_item->m_pData);
    if (!item)
        return;

    CUICellItem* itm = invlist->RemoveItem(cell_item, false);

    m_partner_trade->TransferItem(item, true, true);

    m_pLists[eTradePartnerList]->SetItem(itm);

    SetCurrentItem(nullptr);
    UpdateItemsPlace();
}
//-Alundaio
