#include "stdafx.h"
#include "UIMpTradeWnd.h"
#include "UIMpItemsStoreWnd.h"

#include "UICellItem.h"
#include "UIDragDropListEx.h"
#include "UICellCustomItems.h"
#include "game_cl_deathmatch.h"
#include "game_cl_capture_the_artefact.h"

bool CUIMpTradeWnd::TryToSellItem(SBuyItemInfo* sell_itm, bool do_destroy, SBuyItemInfo*& itm_res)
{
    const bool is_helper_item = sell_itm->m_cell_item->IsHelper();
    u32 _item_cost = 0;
    if (!is_helper_item)
    {
        SellItemAddons(sell_itm, at_scope);
        SellItemAddons(sell_itm, at_silencer);
        SellItemAddons(sell_itm, at_glauncher);

        _item_cost = m_item_mngr->GetItemCost(sell_itm->m_name_sect, GetRank());
    }

    SetMoneyAmount(GetMoneyAmount() + _item_cost);

    CUICellItem* _itm = NULL;
    CUIDragDropListEx* list_from = sell_itm->m_cell_item->OwnerList();
    if (list_from)
        _itm = list_from->RemoveItem(sell_itm->m_cell_item, false);
    else
        _itm = sell_itm->m_cell_item;

    SBuyItemInfo* iinfo = FindItem(_itm); // just detached
    itm_res = iinfo;

    u32 cnt_in_shop = GetItemCount(sell_itm->m_name_sect, SBuyItemInfo::e_shop);

    iinfo->SetState(SBuyItemInfo::e_sold);

    _itm->SetIsHelper(false);

    if (cnt_in_shop != 0)
    {
        if (do_destroy)
            DestroyItem(iinfo);
    }
    else
    { // return to shop

        if (m_store_hierarchy->CurrentLevel().HasItem(iinfo->m_name_sect))
        {
            CUIDragDropListEx* _new_owner = m_list[e_shop];
            _new_owner->SetItem(iinfo->m_cell_item);
            int accel_idx = m_store_hierarchy->CurrentLevel().GetItemIdx(iinfo->m_name_sect);
            VERIFY(accel_idx != -1);
            iinfo->m_cell_item->SetAccelerator((accel_idx > 10) ? 0 : SDL_SCANCODE_1 + accel_idx);
            iinfo->m_cell_item->SetCustomDraw(new CUICellItemTradeMenuDraw(this, iinfo));
        }
    }
    if (_item_cost != 0)
    {
        int item_cost = _item_cost;
        SetMoneyChangeString(item_cost);
    }

    UpdateCorrespondingItemsForList(list_from);
    return true;
}

bool CUIMpTradeWnd::BuyItemAction(SBuyItemInfo* itm)
{
    CUIDragDropListEx* _list = NULL;
    u8 list_idx = m_item_mngr->GetItemSlotIdx(itm->m_name_sect);
    VERIFY(list_idx < e_total_lists && list_idx != e_shop);
    if (list_idx == e_pistol || list_idx == e_rifle || list_idx == e_outfit)
    {
        _list = m_list[list_idx];
        if (_list->ItemsCount())
        {
            VERIFY(_list->ItemsCount() == 1);
            CUICellItem* ci = _list->GetItemIdx(0);

            if (ci->EqualTo(itm->m_cell_item))
            {
                return false;
            }

            SBuyItemInfo* to_sell = FindItem(ci);
            SBuyItemInfo::EItmState _stored_state = to_sell->GetState();

            SBuyItemInfo* tmp_iinfo = NULL;
            TryToSellItem(to_sell, false, tmp_iinfo);

            bool b_res = TryToBuyItem(itm, bf_normal, NULL);

            if (!b_res)
            {
                to_sell->SetState(SBuyItemInfo::e_undefined); // hack
                bool b_res2 = TryToBuyItem(to_sell, bf_check_money | bf_ignore_team | bf_own_itm, NULL);
                R_ASSERT(b_res2);
                to_sell->SetState(SBuyItemInfo::e_undefined); // hack
                to_sell->SetState(_stored_state); // hack
            }
            else if (to_sell->GetState() == SBuyItemInfo::e_sold ||
                to_sell->GetState() == SBuyItemInfo::e_undefined) // maybe in shop now
                DestroyItem(to_sell);

            return b_res;
        }
    }

    return TryToBuyItem(itm, bf_normal, NULL);
}

bool CUIMpTradeWnd::TryToBuyItem(SBuyItemInfo* buy_itm, u32 buy_flags, SBuyItemInfo* itm_parent)
{
    SBuyItemInfo* iinfo = buy_itm;
    const shared_str& buy_item_name = iinfo->m_name_sect;

    const bool is_helper = buy_itm->m_cell_item->IsHelper();
    bool b_can_buy = is_helper || CheckBuyPossibility(buy_item_name, buy_flags, false);
    if (!b_can_buy)
        return false;

    if (0 == (bf_ignore_team & buy_flags))
    {
        if (GameID() == eGameIDCaptureTheArtefact)
        {
            game_cl_CaptureTheArtefact* cta_game = smart_cast<game_cl_CaptureTheArtefact*>(&Game());
            if (cta_game && !cta_game->LocalPlayerCanBuyItem(buy_item_name))
                return false;
        }
        else
        {
            game_cl_Deathmatch* dm_game = smart_cast<game_cl_Deathmatch*>(&Game());
            if (dm_game && !dm_game->LocalPlayerCanBuyItem(buy_item_name))
                return false;
        }
    }

    u32 _item_cost = m_item_mngr->GetItemCost(buy_item_name, GetRank());
    if (is_helper)
    {
        _item_cost = 0;
    }

    if ((buy_flags & bf_check_money))
    {
        SetMoneyAmount(GetMoneyAmount() - _item_cost);
    }

    if (buy_flags & bf_own_itm)
    {
        iinfo->SetState(SBuyItemInfo::e_own);
    }
    else
        iinfo->SetState(SBuyItemInfo::e_bought);

    CUICellItem* cell_itm = NULL;
    bool b_alone = true;
    if (iinfo->m_cell_item->OwnerList()) // just from shop
    {
        cell_itm = iinfo->m_cell_item->OwnerList()->RemoveItem(iinfo->m_cell_item, false);
        b_alone = false;
    }
    else // new created
    {
        cell_itm = iinfo->m_cell_item;
        b_alone = true;
    }

    R_ASSERT(cell_itm->OwnerList() == NULL);

    cell_itm->SetTextureColor(m_item_color_normal);
    bool b_addon = TryToAttachItemAsAddon(iinfo, itm_parent);
    if (!b_addon)
    {
        CUIDragDropListEx* _new_owner = NULL;
        _new_owner = GetMatchedListForItem(buy_item_name);

        R_ASSERT2(!_new_owner->IsOwner(cell_itm), buy_item_name.c_str());

        _new_owner->SetItem(cell_itm);
        cell_itm->SetCustomDraw(NULL);
        cell_itm->SetAccelerator(0);

        UpdateCorrespondingItemsForList(_new_owner);
    }
    else
    {
        DestroyItem(iinfo);
    }

    RenewShopItem(buy_item_name, true);

    if ((buy_flags & bf_normal) && _item_cost != 0)
    {
        int cost = -(int)_item_cost;
        SetMoneyChangeString(cost);
    }
    return true;
}
#include "string_table.h"
bool CUIMpTradeWnd::CheckBuyPossibility(const shared_str& sect_name, u32 buy_flags, bool b_silent)
{
    string256 info_buffer;
    bool b_can_buy = true;

    u32 _item_cost = m_item_mngr->GetItemCost(sect_name, GetRank());

    if ((buy_flags & bf_check_money))
    {
        if (GetMoneyAmount() < _item_cost)
        {
            if (!b_silent)
                xr_sprintf(info_buffer, "%s. %s. %s[%d] %s[%d]",
                    CStringTable().translate("ui_inv_cant_buy_item").c_str(),
                    CStringTable().translate("ui_inv_not_enought_money").c_str(),
                    CStringTable().translate("ui_inv_has").c_str(), GetMoneyAmount(),
                    CStringTable().translate("ui_inv_need").c_str(), _item_cost);
            b_can_buy = false;
        };
    }

    if (b_can_buy && (buy_flags & bf_check_rank_restr) && !g_mp_restrictions.IsAvailable(sect_name))
    {
        if (!b_silent)
            xr_sprintf(info_buffer, "%s. %s. %s[%s] %s[%s] ", CStringTable().translate("ui_inv_cant_buy_item").c_str(),
                CStringTable().translate("ui_inv_rank_restr").c_str(), CStringTable().translate("ui_inv_has").c_str(),
                g_mp_restrictions.GetRankName(GetRank()).c_str(), CStringTable().translate("ui_inv_need").c_str(),
                g_mp_restrictions.GetRankName(get_rank(sect_name)).c_str());
        b_can_buy = false;
    }

    if (b_can_buy && (buy_flags & bf_check_count_restr))
    {
        const shared_str& group = g_mp_restrictions.GetItemGroup(sect_name);
        u32 cnt_restr = g_mp_restrictions.GetGroupCount(group);

        u32 cnt_have = GetGroupCount(group, SBuyItemInfo::e_bought);
        cnt_have += GetGroupCount(group, SBuyItemInfo::e_own);

        if (cnt_have >= cnt_restr)
        {
            if (!b_silent)
                xr_sprintf(info_buffer, "%s. %s. %s [%d]", CStringTable().translate("ui_inv_cant_buy_item").c_str(),
                    CStringTable().translate("ui_inv_count_restr").c_str(),
                    CStringTable().translate("ui_inv_you_already_have").c_str(), cnt_have);
            b_can_buy = false;
        }
    }

    if (!b_can_buy && !b_silent)
    {
        SetInfoString(info_buffer);
    };

    return b_can_buy;
}

void CUIMpTradeWnd::RenewShopItem(const shared_str& sect_name, bool b_just_bought)
{
    if (m_store_hierarchy->CurrentLevel().HasItem(sect_name))
    {
        CUIDragDropListEx* pList = m_list[e_shop];
        SBuyItemInfo* pitem = CreateItem(sect_name, SBuyItemInfo::e_shop, true);

        CUIDragDropListEx* old_parent = pitem->m_cell_item->OwnerList();
        R_ASSERT(old_parent == NULL || old_parent == pList);

        if (pitem->m_cell_item->OwnerList() != pList)
        {
            int accel_idx = m_store_hierarchy->CurrentLevel().GetItemIdx(sect_name);
            pitem->m_cell_item->SetAccelerator((accel_idx > 9) ? 0 : SDL_SCANCODE_1 + accel_idx);

            pitem->m_cell_item->SetCustomDraw(new CUICellItemTradeMenuDraw(this, pitem));
            pList->SetItem(pitem->m_cell_item);
        }
    }
}

void CUIMpTradeWnd::ItemToBelt(const shared_str& sectionName)
{
    R_ASSERT2(m_item_mngr->GetItemIdx(sectionName) != u32(-1), sectionName.c_str());

    CUIDragDropListEx* pList = GetMatchedListForItem(sectionName);

    SBuyItemInfo* pitem = CreateItem(sectionName, SBuyItemInfo::e_own, false);
    pList->SetItem(pitem->m_cell_item);
}

void CUIMpTradeWnd::ItemToRuck(const shared_str& sectionName, u8 addons)
{
    R_ASSERT2(m_item_mngr->GetItemIdx(sectionName) != u32(-1), sectionName.c_str());

    CUIDragDropListEx* pList = GetMatchedListForItem(sectionName);

    SBuyItemInfo* pitem = CreateItem(sectionName, SBuyItemInfo::e_own, false);
    SetItemAddonsState_ext(pitem, addons);
    pList->SetItem(pitem->m_cell_item);
}

void CUIMpTradeWnd::ItemToSlot(const shared_str& sectionName, u8 addons)
{
    R_ASSERT2(m_item_mngr->GetItemIdx(sectionName) != u32(-1), sectionName.c_str());

    CUIDragDropListEx* pList = GetMatchedListForItem(sectionName);

    SBuyItemInfo* pitem = CreateItem(sectionName, SBuyItemInfo::e_own, false);
    SetItemAddonsState_ext(pitem, addons);
    pList->SetItem(pitem->m_cell_item);

    UpdateCorrespondingItemsForList(pList);
}
