#pragma once

#define _new_buy_wnd

#ifdef _new_buy_wnd
class CUIMpTradeWnd;
typedef CUIMpTradeWnd BUY_WND_TYPE;
#else
class CUIBuyWnd;
typedef CUIBuyWnd BUY_WND_TYPE;
#endif

#include "xrCore/Containers/AssociativeVector.hpp"
#include "Restrictions.h"

struct string_lex_less
{
    bool operator()(const shared_str& A, const shared_str& B) const { return (xr_strcmp(A, B) < 0); }
};
class CItemMgr
{
    struct _i
    {
        u8 slot_idx;
        u32 cost[_RANK_COUNT];
    };
    typedef AssociativeVector<shared_str, _i, string_lex_less> COST_MAP;
    typedef COST_MAP::iterator COST_MAP_IT;
    typedef COST_MAP::const_iterator COST_MAP_CIT;
    COST_MAP m_items;

public:
    void Load(const shared_str& sect);
    const u32 GetItemCost(const shared_str& sect_name, u32 rank) const;
    const u8 GetItemSlotIdx(const shared_str& sect_name) const;
    const u32 GetItemIdx(const shared_str& sect_name) const;
    void Dump() const;
    const u32 GetItemsCount() const;
    const shared_str& GetItemName(u32 Idx) const;
};
