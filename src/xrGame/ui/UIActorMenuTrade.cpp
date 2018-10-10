#include "pch_script.h"
#include "UIActorMenu.h"
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
    m_pInventoryBagList->Show(false);
    m_PartnerCharacterInfo->Show(true);
    m_PartnerMoney->Show(true);
    m_pQuickSlot->Show(true);

    m_pTradeActorBagList->Show(true);
    m_pTradeActorList->Show(true);
    m_pTradePartnerBagList->Show(true);
    m_pTradePartnerList->Show(true);

    m_RightDelimiter->Show(true);
    m_LeftDelimiter->Show(true);
    m_LeftBackground->Show(true);

    m_PartnerBottomInfo->Show(true);
    m_PartnerWeight->Show(true);
    m_trade_buy_button->Show(true);
    m_trade_sell_button->Show(true);

    VERIFY(m_pPartnerInvOwner);
    m_pPartnerInvOwner->StartTrading();

    InitInventoryContents(m_pTradeActorBagList);
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
    m_pTradePartnerBagList->ClearAll(true);

    TIItemContainer items_list;
    m_pPartnerInvOwner->inventory().AddAvailableItems(items_list, true);
    std::sort(items_list.begin(), items_list.end(), InventoryUtilities::GreaterRoomInRuck);

    TIItemContainer::iterator itb = items_list.begin();
    TIItemContainer::iterator ite = items_list.end();
    for (; itb != ite; ++itb)
    {
        if (!is_item_in_list(m_pTradePartnerList, *itb))
        {
            CUICellItem* itm = create_cell_item(*itb);
            m_pTradePartnerBagList->SetItem(itm);
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

    m_pInventoryBagList->Show(true);
    m_PartnerCharacterInfo->Show(false);
    m_PartnerMoney->Show(false);

    m_pTradeActorBagList->Show(false);
    m_pTradeActorList->Show(false);
    m_pTradePartnerBagList->Show(false);
    m_pTradePartnerList->Show(false);

    m_RightDelimiter->Show(false);
    m_LeftDelimiter->Show(false);
    m_LeftBackground->Show(false);

    m_PartnerBottomInfo->Show(false);
    m_PartnerWeight->Show(false);
    m_trade_buy_button->Show(false);
    m_trade_sell_button->Show(false);

    if (!CurrentGameUI())
        return;
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
            VERIFY(new_owner == m_pTradeActorList);
        }
        else
            new_owner = m_pTradeActorList;

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
        VERIFY(new_owner == m_pTradePartnerList);
    }
    else
        new_owner = m_pTradePartnerList;

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
        VERIFY(new_owner == m_pTradePartnerBagList);
    }
    else
        new_owner = m_pTradePartnerBagList;

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

    float r1 = CalcItemsWeight(m_pTradeActorList); // actor
    float r2 = CalcItemsWeight(m_pTradePartnerList); // partner
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

    InventoryUtilities::UpdateWeightStr(*m_ActorWeight, *m_ActorWeightMax, m_pActorInvOwner);

    m_ActorWeight->AdjustWidthToText();
    m_ActorWeightMax->AdjustWidthToText();
    m_ActorBottomInfo->AdjustWidthToText();

    Fvector2 pos = m_ActorWeight->GetWndPos();
    pos.x = m_ActorWeightMax->GetWndPos().x - m_ActorWeight->GetWndSize().x - 5.0f;
    m_ActorWeight->SetWndPos(pos);
    pos.x = pos.x - m_ActorBottomInfo->GetWndSize().x - 5.0f;
    m_ActorBottomInfo->SetWndPos(pos);
}

void CUIActorMenu::UpdatePartnerBag()
{
    string64 buf;

    CBaseMonster* monster = smart_cast<CBaseMonster*>(m_pPartnerInvOwner);
    if (monster || m_pPartnerInvOwner->use_simplified_visual())
    {
        m_PartnerWeight->SetText("");
    }
    else if (m_pPartnerInvOwner->InfinitiveMoney())
    {
        m_PartnerMoney->SetText("--- RU");
    }
    else
    {
        xr_sprintf(buf, "%d RU", m_pPartnerInvOwner->get_money());
        m_PartnerMoney->SetText(buf);
    }

    LPCSTR kg_str = StringTable().translate("st_kg").c_str();
    float total = CalcItemsWeight(m_pTradePartnerBagList);
    xr_sprintf(buf, "%.1f %s", total, kg_str);
    m_PartnerWeight->SetText(buf);
    m_PartnerWeight->AdjustWidthToText();

    Fvector2 pos = m_PartnerWeight->GetWndPos();
    pos.x = m_PartnerWeight_end_x - m_PartnerWeight->GetWndSize().x - 5.0f;
    m_PartnerWeight->SetWndPos(pos);
    pos.x = pos.x - m_PartnerBottomInfo->GetWndSize().x - 5.0f;
    m_PartnerBottomInfo->SetWndPos(pos);
}

void CUIActorMenu::UpdatePrices()
{
    LPCSTR kg_str = StringTable().translate("st_kg").c_str();

    UpdateActor();
    UpdatePartnerBag();
    u32 actor_price = CalcItemsPrice(m_pTradeActorList, m_partner_trade, true);
    u32 partner_price = CalcItemsPrice(m_pTradePartnerList, m_partner_trade, false);

    string64 buf;
    xr_sprintf(buf, "%d RU", actor_price);
    m_ActorTradePrice->SetText(buf);
    m_ActorTradePrice->AdjustWidthToText();
    xr_sprintf(buf, "%d RU", partner_price);
    m_PartnerTradePrice->SetText(buf);
    m_PartnerTradePrice->AdjustWidthToText();

    float actor_weight = CalcItemsWeight(m_pTradeActorList);
    float partner_weight = CalcItemsWeight(m_pTradePartnerList);

    xr_sprintf(buf, "(%.1f %s)", actor_weight, kg_str);
    m_ActorTradeWeightMax->SetText(buf);
    xr_sprintf(buf, "(%.1f %s)", partner_weight, kg_str);
    m_PartnerTradeWeightMax->SetText(buf);

    Fvector2 pos = m_ActorTradePrice->GetWndPos();
    pos.x = m_ActorTradeWeightMax->GetWndPos().x - m_ActorTradePrice->GetWndSize().x - 5.0f;
    m_ActorTradePrice->SetWndPos(pos);
    //	pos.x = pos.x - m_ActorTradeCaption->GetWndSize().x - 5.0f;
    //	m_ActorTradeCaption->SetWndPos( pos );

    pos = m_PartnerTradePrice->GetWndPos();
    pos.x = m_PartnerTradeWeightMax->GetWndPos().x - m_PartnerTradePrice->GetWndSize().x - 5.0f;
    m_PartnerTradePrice->SetWndPos(pos);
    //	pos.x = pos.x - m_PartnerTradeCaption->GetWndSize().x - 5.0f;
    //	m_PartnerTradeCaption->SetWndPos( pos );
}

void CUIActorMenu::OnBtnPerformTradeBuy(CUIWindow* w, void* d)
{
    if (m_pTradePartnerList->ItemsCount() == 0)
    {
        return;
    }

    int actor_money = (int)m_pActorInvOwner->get_money();
    int partner_money = (int)m_pPartnerInvOwner->get_money();
    int actor_price = 0; //(int)CalcItemsPrice( m_pTradeActorList,   m_partner_trade, true  );
    int partner_price = (int)CalcItemsPrice(m_pTradePartnerList, m_partner_trade, false);

    int delta_price = actor_price - partner_price;
    actor_money += delta_price;
    partner_money -= delta_price;

    if ((actor_money >= 0) /*&& ( partner_money >= 0 )*/ && (actor_price >= 0 || partner_price > 0))
    {
        m_partner_trade->OnPerformTrade(partner_price, actor_price);

        //		TransferItems( m_pTradeActorList,   m_pTradePartnerBagList, m_partner_trade, true );
        TransferItems(m_pTradePartnerList, m_pTradeActorBagList, m_partner_trade, false);
    }
    else
    {
        if (actor_money < 0)
        {
            CallMessageBoxOK("not_enough_money_actor");
        }
        // else if ( partner_money < 0 )
        //{
        //	CallMessageBoxOK( "not_enough_money_partner" );
        //}
        else
        {
            CallMessageBoxOK("trade_dont_make");
        }
    }
    SetCurrentItem(NULL);

    UpdateItemsPlace();
}
void CUIActorMenu::OnBtnPerformTradeSell(CUIWindow* w, void* d)
{
    if (m_pTradeActorList->ItemsCount() == 0)
    {
        return;
    }

    int actor_money = (int)m_pActorInvOwner->get_money();
    int partner_money = (int)m_pPartnerInvOwner->get_money();
    int actor_price = (int)CalcItemsPrice(m_pTradeActorList, m_partner_trade, true);
    int partner_price = 0; //(int)CalcItemsPrice( m_pTradePartnerList, m_partner_trade, false );

    int delta_price = actor_price - partner_price;
    actor_money += delta_price;
    partner_money -= delta_price;

    if ((actor_money >= 0) && (partner_money >= 0) && (actor_price >= 0 || partner_price > 0))
    {
        m_partner_trade->OnPerformTrade(partner_price, actor_price);

        TransferItems(m_pTradeActorList, m_pTradePartnerBagList, m_partner_trade, true);
        //		TransferItems( m_pTradePartnerList,	m_pTradeActorBagList,	m_partner_trade, false );
    }
    else
    {
        /*		if ( actor_money < 0 )
		{
			CallMessageBoxOK( "not_enough_money_actor" );
		}
		else */ if (partner_money < 0)
        {
            CallMessageBoxOK("not_enough_money_partner");
        }
        else
        {
            CallMessageBoxOK("trade_dont_make");
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
    if (!m_partner_trade || !m_pTradePartnerList)
        return;

    CUIDragDropListEx* invlist = GetListByType(iActorBag);
    if (!invlist->IsOwner(cell_item))
        return;

    PIItem item = static_cast<PIItem>(cell_item->m_pData);
    if (!item)
        return;

    CUICellItem* itm = invlist->RemoveItem(cell_item, false);

    m_partner_trade->TransferItem(item, true, true);

    m_pTradePartnerList->SetItem(itm);

    SetCurrentItem(nullptr);
    UpdateItemsPlace();
}
//-Alundaio