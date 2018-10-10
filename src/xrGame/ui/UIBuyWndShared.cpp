#include "StdAfx.h"
#include "UIBuyWndShared.h"
#include "UIMpTradeWnd.h"

extern LPCSTR _list_names[];

void CItemMgr::Load(const shared_str& sect_cost)
{
    CInifile::Sect& sect = pSettings->r_section(sect_cost);

    u32 idx = 0;
    for (auto it = sect.Data.cbegin(); it != sect.Data.cend(); ++it, ++idx)
    {
        _i& val = m_items[it->first];
        val.slot_idx = 0xff;
        int c = sscanf(
            it->second.c_str(), "%d,%d,%d,%d,%d", &val.cost[0], &val.cost[1], &val.cost[2], &val.cost[3], &val.cost[4]);
        VERIFY(c > 0);

        while (c < _RANK_COUNT)
        {
            val.cost[c] = val.cost[c - 1];
            ++c;
        }
    }

    for (u8 i = CUIMpTradeWnd::e_first; i < CUIMpTradeWnd::e_total_lists; ++i)
    {
        const shared_str& buff = pSettings->r_string("buy_menu_items_place", _list_names[i]);

        u32 cnt = _GetItemCount(buff.c_str());
        string1024 _one;

        for (u32 c = 0; c < cnt; ++c)
        {
            _GetItem(buff.c_str(), c, _one);
            shared_str _one_str = _one;
            COST_MAP_IT it = m_items.find(_one_str);
            R_ASSERT(it != m_items.end());
            R_ASSERT3(
                it->second.slot_idx == 0xff, "item has duplicate record in [buy_menu_items_place] section ", _one);
            it->second.slot_idx = i;
        }
    }
}

const u32 CItemMgr::GetItemCost(const shared_str& sect_name, u32 rank) const
{
    COST_MAP_CIT it = m_items.find(sect_name);
    VERIFY(it != m_items.end());
    return it->second.cost[rank];
}

const u8 CItemMgr::GetItemSlotIdx(const shared_str& sect_name) const
{
    COST_MAP_CIT it = m_items.find(sect_name);
    VERIFY(it != m_items.end());
    return it->second.slot_idx;
}

const u32 CItemMgr::GetItemIdx(const shared_str& sect_name) const
{
    COST_MAP_CIT it = m_items.find(sect_name);

    if (it == m_items.end())
    {
#ifdef DEBUG
        Msg("item not found in registry [%s]", sect_name.c_str());
#endif // DEBUG
        return u32(-1);
    }

    return u32(std::distance(m_items.begin(), it));
}

void CItemMgr::Dump() const
{
#ifndef MASTER_GOLD
    COST_MAP_CIT it = m_items.begin();
    COST_MAP_CIT it_e = m_items.end();

    Msg("--CItemMgr::Dump");
    for (; it != it_e; ++it)
    {
        const _i& val = it->second;
        R_ASSERT3(
            it->second.slot_idx != 0xff, "item has no record in [buy_menu_items_place] section ", it->first.c_str());
        Msg("[%s] slot=[%d] cost= %d,%d,%d,%d,%d", it->first.c_str(), val.slot_idx, val.cost[0], val.cost[1],
            val.cost[2], val.cost[3], val.cost[4]);
    }
#endif // #ifndef MASTER_GOLD
}

const u32 CItemMgr::GetItemsCount() const { return m_items.size(); }
const shared_str& CItemMgr::GetItemName(u32 Idx) const
{
    R_ASSERT(Idx < m_items.size());
    return (m_items.begin() + Idx)->first;
}
