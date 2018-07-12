#include "stdafx.h"
#include "UIMpTradeWnd.h"
#include "inventory_item.h"
#include "PhysicsShellHolder.h"
#include "Common/object_broker.h"
#include "UICellItem.h"
#include "UIDragDropListEx.h"
#include "string_table.h"
#include "UIMpItemsStoreWnd.h"
#include "Weapon.h"
#include "WeaponMagazinedWGrenade.h"
#include "UICellCustomItems.h"

extern "C" IFactoryObject* __cdecl xrFactory_Create(CLASS_ID clsid);

extern "C" void __cdecl xrFactory_Destroy(IFactoryObject* O);

CUICellItem* create_cell_item(CInventoryItem* itm);

SBuyItemInfo::SBuyItemInfo() { m_item_state = e_undefined; }
SBuyItemInfo::~SBuyItemInfo()
{
    CInventoryItem* iitem = (CInventoryItem*)m_cell_item->m_pData;
    xrFactory_Destroy(&iitem->object());
    delete_data(m_cell_item);
}

void SBuyItemInfo::SetState(const EItmState& s)
{
    if (s == e_undefined)
    {
        m_item_state = s;
        return;
    }

    switch (m_item_state)
    {
    case e_undefined:
    {
        m_item_state = s;
        break;
    };
    case e_bought:
    {
        VERIFY2(s == e_shop || s == e_sold, "incorrect SBuyItemInfo::SetState sequence");

        m_item_state = e_shop;
        break;
    };
    case e_sold:
    {
        VERIFY2(s == e_own || s == e_bought, "incorrect SBuyItemInfo::SetState sequence");
        m_item_state = e_own;
        break;
    };
    case e_own:
    {
        VERIFY2(s == e_sold, "incorrect SBuyItemInfo::SetState sequence");
        m_item_state = s;
        break;
    };
    case e_shop:
    {
        VERIFY2(s == e_bought, "incorrect SBuyItemInfo::SetState sequence");
        m_item_state = s;
        break;
    };
    };
}

LPCSTR _state_names[] = {"e_undefined", "e_bought", "e_sold", "e_own", "e_shop"};

LPCSTR SBuyItemInfo::GetStateAsText() const
{
    EItmState st = GetState();
    return _state_names[st];
}

CInventoryItem* CUIMpTradeWnd::CreateItem_internal(const shared_str& name_sect)
{
    CLASS_ID class_id = pSettings->r_clsid(name_sect, "class");

    IFactoryObject* dll_pure = xrFactory_Create(class_id);
    VERIFY(dll_pure);
    CInventoryItem* pIItem = smart_cast<CInventoryItem*>(dll_pure);
    pIItem->object().Load(name_sect.c_str());
    VERIFY(pIItem);
    return (pIItem);
}

SBuyItemInfo* CUIMpTradeWnd::CreateItem(const shared_str& name_sect, SBuyItemInfo::EItmState type, bool find_if_exist)
{
    SBuyItemInfo* iinfo = (find_if_exist) ? FindItem(name_sect, type) : nullptr;
    if (iinfo)
        return iinfo;

    iinfo = new SBuyItemInfo();
    m_all_items.push_back(iinfo);
    iinfo->m_name_sect = name_sect;
    iinfo->SetState(type);
    iinfo->m_cell_item = create_cell_item(CreateItem_internal(name_sect));
    iinfo->m_cell_item->m_b_destroy_childs = false;
    return iinfo;
}

struct state_eq
{
    SBuyItemInfo::EItmState m_state;

    state_eq(SBuyItemInfo::EItmState state) : m_state(state) {}
    bool operator()(SBuyItemInfo* itm) { return (m_state == itm->GetState()); }
};

SBuyItemInfo* CUIMpTradeWnd::FindItem(SBuyItemInfo::EItmState state)
{
    state_eq eq(state);

    auto it = std::find_if(m_all_items.begin(), m_all_items.end(), eq);
    return (it == m_all_items.end()) ? nullptr : (*it);
}

SBuyItemInfo* CUIMpTradeWnd::FindItem(const shared_str& name_sect, SBuyItemInfo::EItmState type)
{
    auto it = m_all_items.begin();
    auto it_e = m_all_items.end();
    for (; it != it_e; ++it)
    {
        SBuyItemInfo* pitm = *it;
        if (pitm->m_name_sect == name_sect && pitm->GetState() == type)
            return pitm;
    }
    return nullptr;
}

SBuyItemInfo* CUIMpTradeWnd::FindItem(CUICellItem* item)
{
    auto it = m_all_items.begin();
    auto it_e = m_all_items.end();

    for (; it != it_e; ++it)
    {
        SBuyItemInfo* pitm = *it;
        if (pitm->m_cell_item == item)
            return pitm;
    }
    R_ASSERT2(0, "buy menu data corruption. cant find corresponding SBuyItemInfo* for CellItem");
    return nullptr;
}

bool CUIMpTradeWnd::HasItemInGroup(shared_str const& section_name)
{
    return m_store_hierarchy->FindItem(section_name) ? true : false;
}

void CUIMpTradeWnd::DeleteHelperItems()
{
    int lists[] = {e_medkit, e_granade, e_rifle_ammo, e_pistol_ammo};

    for (int i = 0; i < sizeof(lists) / sizeof(lists[0]); ++i)
    {
        DeleteHelperItems(m_list[lists[i]]);
    }
}

void CUIMpTradeWnd::DeleteHelperItems(CUIDragDropListEx* list)
{
    ITEMS_vec to_sell;

    for (ITEMS_vec::iterator it = m_all_items.begin(); it != m_all_items.end(); ++it)
    {
        SBuyItemInfo* item = *it;

        if (item->m_cell_item->OwnerList() != list)
        {
            continue;
        }

        if (item->GetState() != SBuyItemInfo::e_bought && item->GetState() != SBuyItemInfo::e_own)
        {
            continue;
        }

        if (item->m_cell_item->IsHelper())
        {
            if (std::find(to_sell.begin(), to_sell.end(), item) == to_sell.end())
            {
                to_sell.push_back(item);
            }
        }
    }

    for (ITEMS_vec::iterator it = to_sell.begin(); it != to_sell.end(); ++it)
    {
        SBuyItemInfo* tempo = nullptr;
        TryToSellItem(*it, true, tempo);
    }
}

void CUIMpTradeWnd::UpdateHelperItems()
{
    DeleteHelperItems();

    int lists[] = {e_medkit, e_granade, e_rifle_ammo, e_pistol_ammo};

    for (int i = 0; i < sizeof(lists) / sizeof(lists[0]); ++i)
    {
        CreateHelperItems(m_list[lists[i]]);
    }
}

void CUIMpTradeWnd::CreateHelperItems(xr_vector<shared_str>& ammo_types)
{
    for (xr_vector<shared_str>::iterator it = ammo_types.begin(); it != ammo_types.end(); ++it)
    {
        const shared_str& ammo_name = *it;
        if (!m_store_hierarchy->FindItem(ammo_name))
        {
            continue;
        }

        SBuyItemInfo* ammo_item = CreateItem(ammo_name, SBuyItemInfo::e_undefined, false);
        ammo_item->m_cell_item->SetIsHelper(true);
        TryToBuyItem(ammo_item, bf_normal, nullptr);
    }
}

void CUIMpTradeWnd::CreateHelperItems(CUIDragDropListEx* list, const CStoreHierarchy::item* shop_level)
{
    for (xr_vector<shared_str>::const_iterator it = shop_level->m_items_in_group.begin();
         it != shop_level->m_items_in_group.end(); ++it)
    {
        shared_str item_name = *it;
        CUIDragDropListEx* match_list = GetMatchedListForItem(item_name);

        if (match_list == list)
        {
            SBuyItemInfo* new_item = CreateItem(item_name, SBuyItemInfo::e_undefined, false);

            CUIInventoryCellItem* inventory_cell_item;
            if ((inventory_cell_item = dynamic_cast<CUIInventoryCellItem*>(new_item->m_cell_item)) != nullptr)
            {
                inventory_cell_item->SetIsHelper(true);
                inventory_cell_item->UpdateItemText();

                TryToBuyItem(new_item, bf_normal, nullptr);
            }
        }
    }

    for (u32 i = 0; i < shop_level->ChildCount(); ++i)
    {
        const CStoreHierarchy::item* child = &shop_level->ChildAtIdx(i);
        CreateHelperItems(list, child);
    }
}

void CUIMpTradeWnd::CreateHelperItems(CUIDragDropListEx* list)
{
    CUIDragDropListEx* parent_list = nullptr;

    if (list == m_list[e_pistol_ammo])
    {
        parent_list = m_list[e_pistol];
    }
    else if (list == m_list[e_rifle_ammo])
    {
        parent_list = m_list[e_rifle];
    }

    if (list == m_list[e_medkit] || list == m_list[e_granade])
    {
        CreateHelperItems(list, &m_store_hierarchy->GetRoot());
        return;
    }

    VERIFY(parent_list);
    if (!parent_list->ItemsCount())
    {
        return;
    }

    CInventoryItem* parent_item = (CInventoryItem*)parent_list->GetItemIdx(0)->m_pData;
    CWeapon* wpn = smart_cast<CWeapon*>(parent_item);
    R_ASSERT(wpn);

    CreateHelperItems(wpn->m_ammoTypes);

    if (CWeaponMagazinedWGrenade* wpn2 = smart_cast<CWeaponMagazinedWGrenade*>(parent_item))
    {
        if (wpn2->IsGrenadeLauncherAttached())
        {
            CreateHelperItems(wpn2->m_ammoTypes2);
        }
    }
}

void CUIMpTradeWnd::UpdateCorrespondingItemsForList(CUIDragDropListEx* _list)
{
    CUIDragDropListEx* dependent_list = nullptr;
    CUIDragDropListEx* bag_list = m_list[e_player_bag];

    if (_list == m_list[e_pistol])
        dependent_list = m_list[e_pistol_ammo];
    else if (_list == m_list[e_rifle])
        dependent_list = m_list[e_rifle_ammo];

    if (!dependent_list)
        return;

    DeleteHelperItems(dependent_list);

    xr_list<SBuyItemInfo*> _tmp_list;
    while (dependent_list->ItemsCount() != 0)
    {
        CUICellItem* ci = dependent_list->GetItemIdx(0);
        CUICellItem* ci2 = dependent_list->RemoveItem(ci, false);
        SBuyItemInfo* bi = FindItem(ci2);
        _tmp_list.push_back(bi);
        bag_list->SetItem(ci2);
    };

    CreateHelperItems(dependent_list);

    if (_list->ItemsCount() != 0)
    {
        R_ASSERT(_list->ItemsCount() == 1);

        CInventoryItem* main_item = (CInventoryItem*)_list->GetItemIdx(0)->m_pData;

        while (bag_list->ItemsCount())
        {
            u32 cnt = bag_list->ItemsCount();
            for (u32 idx = 0; idx < cnt; ++idx)
            {
                CUICellItem* ci = bag_list->GetItemIdx(idx);
                SBuyItemInfo* iinfo = FindItem(ci);

                if (main_item->IsNecessaryItem(iinfo->m_name_sect))
                {
                    CUICellItem* ci2 = bag_list->RemoveItem(ci, false);
                    dependent_list->SetItem(ci2);
                    goto _l1;
                }
            }
            break;
        _l1:;
        }
    }

    while (!_tmp_list.empty())
    {
        xr_list<SBuyItemInfo*>::iterator _curr = _tmp_list.begin();
        SBuyItemInfo* bi = *(_curr);

        CUIDragDropListEx* _owner_list = bi->m_cell_item->OwnerList();
        if (_owner_list != bag_list)
        {
            _tmp_list.erase(_curr);
            continue;
        }

        u32 _bag_cnt = bag_list->ItemsCount();

        bool bNecessary = false;

        for (u32 _i = 0; _i < _bag_cnt; ++_i)
        {
            CUICellItem* _ci = bag_list->GetItemIdx(_i);
            CInventoryItem* _ii = (CInventoryItem*)_ci->m_pData;

            bNecessary = _ii->IsNecessaryItem(bi->m_name_sect);

            if (bNecessary)
            {
                _tmp_list.erase(_curr);
                break;
            }
        }
        if (!bNecessary)
        {
            // sell
            SBuyItemInfo* res_info = nullptr;
            TryToSellItem(bi, true, res_info);
            xr_list<SBuyItemInfo*>::iterator tmp_it = find(_tmp_list.begin(), _tmp_list.end(), res_info);
            VERIFY(tmp_it != _tmp_list.end());
            _tmp_list.erase(tmp_it);
        }
    }
}

void CUIMpTradeWnd::DestroyItem(SBuyItemInfo* item)
{
    auto it = std::find(m_all_items.begin(), m_all_items.end(), item);
    R_ASSERT(it != m_all_items.end());

    R_ASSERT(!IsAddonAttached(item, at_scope));
    R_ASSERT(!IsAddonAttached(item, at_glauncher));
    R_ASSERT(!IsAddonAttached(item, at_silencer));

    m_all_items.erase(it);
    delete_data(item);
}

struct eq_name_state_comparer
{
    shared_str m_name;
    SBuyItemInfo::EItmState m_state;
    eq_name_state_comparer(const shared_str& _name, SBuyItemInfo::EItmState _state) : m_name(_name), m_state(_state) {}
    bool operator()(SBuyItemInfo* info) { return (info->m_name_sect == m_name) && (info->GetState() == m_state); }
};

struct eq_name_state_addon_comparer
{
    shared_str m_name;
    SBuyItemInfo::EItmState m_state;
    u8 m_addons;

    eq_name_state_addon_comparer(const shared_str& _name, SBuyItemInfo::EItmState _state, u8 ad)
        : m_name(_name), m_state(_state), m_addons(ad)
    {
    }
    bool operator()(SBuyItemInfo* info)
    {
        if ((info->m_name_sect == m_name) && (info->GetState() == m_state))
        {
            return GetItemAddonsState_ext(info) == m_addons;
        }
        else
            return false;
    }
};

struct eq_group_state_comparer
{
    shared_str m_group;
    SBuyItemInfo::EItmState m_state;

    eq_group_state_comparer(const shared_str& _group, SBuyItemInfo::EItmState _state) : m_group(_group), m_state(_state)
    {
    }
    bool operator()(SBuyItemInfo* info)
    {
        if (!info->m_cell_item->IsHelper() && (info->GetState() == m_state))
        {
            const shared_str& _grp = g_mp_restrictions.GetItemGroup(info->m_name_sect);
            return (_grp == m_group);
        }
        else
            return false;
    }
};

struct eq_state_comparer
{
    SBuyItemInfo::EItmState m_state;
    eq_state_comparer(SBuyItemInfo::EItmState _state) : m_state(_state) {}
    bool operator()(SBuyItemInfo* info) { return (info->GetState() == m_state); }
};

u32 CUIMpTradeWnd::GetItemCount(SBuyItemInfo::EItmState state) const
{
    return (u32)std::count_if(m_all_items.begin(), m_all_items.end(), eq_state_comparer(state));
}

u32 CUIMpTradeWnd::GetItemCount(const shared_str& name_sect, SBuyItemInfo::EItmState state) const
{
    return (u32)std::count_if(m_all_items.begin(), m_all_items.end(), eq_name_state_comparer(name_sect, state));
}
u32 CUIMpTradeWnd::GetItemCount(const shared_str& name_sect, SBuyItemInfo::EItmState state, u8 addon) const
{
    return (u32)std::count_if(
        m_all_items.begin(), m_all_items.end(), eq_name_state_addon_comparer(name_sect, state, addon));
}

u32 CUIMpTradeWnd::GetGroupCount(const shared_str& name_group, SBuyItemInfo::EItmState state) const
{
    return (u32)std::count_if(m_all_items.begin(), m_all_items.end(), eq_group_state_comparer(name_group, state));
}

void CUIMpTradeWnd::ClearPreset(ETradePreset idx)
{
    VERIFY(idx < _preset_count);
    m_preset_storage[idx].clear();
}

const preset_items& CUIMpTradeWnd::GetPreset(ETradePreset idx)
{
    VERIFY(idx < _preset_count);
    return m_preset_storage[idx];
};

u32 _list_prio[] = {
    6, //	e_pistol
    4, //	e_pistol_ammo
    7, //	e_rifle
    5, //	e_rifle_ammo
    10, //	e_outfit
    9, //	e_medkit
    8, //	e_granade
    3, //	e_others
    2, //	e_player_bag
    0, //	e_shop
    0, 0, 0, 0,
};

struct preset_sorter
{
    CItemMgr* m_mgr;
    preset_sorter(CItemMgr* mgr) : m_mgr(mgr){};
    bool operator()(const _preset_item& i1, const _preset_item& i2)
    {
        u8 list_idx1 = m_mgr->GetItemSlotIdx(i1.sect_name);
        u8 list_idx2 = m_mgr->GetItemSlotIdx(i2.sect_name);

        return (_list_prio[list_idx1] > _list_prio[list_idx2]);
    };
};
struct preset_eq
{
    shared_str m_name;
    u8 m_addon;
    preset_eq(const shared_str& _name, u8 ad) : m_name(_name), m_addon(ad){};
    bool operator()(const _preset_item& pitem)
    {
        return (pitem.sect_name == m_name) && (pitem.addon_state == m_addon);
    };
};

void CUIMpTradeWnd::StorePreset(ETradePreset idx, bool bSilent, bool check_allowed_items, bool flush_helpers)
{
    if (flush_helpers)
    {
        DeleteHelperItems();
    }

    if (!bSilent)
    {
        string512 buff;
        xr_sprintf(buff, "%s [%d]", CStringTable().translate("ui_st_preset_stored_to").c_str(), idx);
        SetInfoString(buff);
    }
    auto it = m_all_items.begin();
    auto it_e = m_all_items.end();

    preset_items& v = m_preset_storage[idx];
    v.clear();
    for (; it != it_e; ++it)
    {
        SBuyItemInfo* iinfo = *it;
        if (!(iinfo->GetState() == SBuyItemInfo::e_bought || iinfo->GetState() == SBuyItemInfo::e_own))
            continue;

        if (iinfo->m_cell_item->IsHelper())
        {
            continue;
        }

        u8 addon_state = GetItemAddonsState_ext(iinfo);

        preset_items::iterator fit = std::find_if(v.begin(), v.end(), preset_eq(iinfo->m_name_sect, addon_state));
        if (fit != v.end())
            continue;

        u32 cnt = GetItemCount(iinfo->m_name_sect, SBuyItemInfo::e_bought, addon_state);
        cnt += GetItemCount(iinfo->m_name_sect, SBuyItemInfo::e_own, addon_state);
        if (0 == cnt)
            continue;

        if (check_allowed_items)
        {
            if (nullptr == m_store_hierarchy->FindItem(iinfo->m_name_sect, 0))
                continue;
        }
        v.resize(v.size() + 1);
        _preset_item& _one = v.back();
        _one.sect_name = iinfo->m_name_sect;
        _one.count = cnt;
        _one.addon_state = addon_state;

        if (addon_state & at_scope)
            _one.addon_names[0] = GetAddonNameSect(iinfo, at_scope);

        if (addon_state & at_glauncher)
            _one.addon_names[1] = GetAddonNameSect(iinfo, at_glauncher);

        if (addon_state & at_silencer)
            _one.addon_names[2] = GetAddonNameSect(iinfo, at_silencer);
    }

    std::sort(v.begin(), v.end(), preset_sorter(m_item_mngr));

    if (flush_helpers)
    {
        UpdateHelperItems();
    }
}

void CUIMpTradeWnd::ApplyPreset(ETradePreset idx)
{
    SellAll();
    UpdateHelperItems();

    const preset_items& v = GetPreset(idx);
    preset_items::const_iterator it = v.begin();
    preset_items::const_iterator it_e = v.end();

    for (; it != it_e; ++it)
    {
        const _preset_item& _one = *it;
        u32 _cnt = GetItemCount(_one.sect_name, SBuyItemInfo::e_own);
        for (u32 i = _cnt; i < _one.count; ++i)
        {
            SBuyItemInfo* pitem = CreateItem(_one.sect_name, SBuyItemInfo::e_undefined, false);

            bool b_res = TryToBuyItem(pitem, bf_normal, nullptr);
            if (!b_res)
            {
                DestroyItem(pitem);
            }
            else
            {
                if (_one.addon_state)
                {
                    for (u32 i = 0; i < 3; ++i)
                    {
                        item_addon_type at = (i == 0) ? at_scope : ((i == 1) ? at_glauncher : at_silencer);

                        if (!(_one.addon_state & at))
                            continue;

                        shared_str addon_name = GetAddonNameSect(pitem, at);

                        SBuyItemInfo* addon_item = CreateItem(addon_name, SBuyItemInfo::e_undefined, false);
                        bool b_res_addon = TryToBuyItem(addon_item, bf_normal, pitem);
                        if (!b_res_addon)
                            DestroyItem(addon_item);
                    }
                }
            }
        }
    }
}

void CUIMpTradeWnd::CleanUserItems()
{
    SBuyItemInfo* iinfo = nullptr;
    SBuyItemInfo::EItmState _state = SBuyItemInfo::e_bought;

    for (int i = 0; i < 3; ++i)
    {
        if (i == 0)
            _state = SBuyItemInfo::e_bought;
        else if (i == 1)
            _state = SBuyItemInfo::e_sold;
        else if (i == 2)
            _state = SBuyItemInfo::e_own;

        do
        {
            iinfo = FindItem(_state);
            if (iinfo)
            {
                while (iinfo->m_cell_item->ChildsCount())
                {
                    CUICellItem* iii = iinfo->m_cell_item->PopChild(nullptr);
                    SBuyItemInfo* iinfo_sub = FindItem(iii);
                    R_ASSERT2(iinfo_sub->GetState() == _state || iinfo_sub->GetState() == SBuyItemInfo::e_shop ||
                            iinfo_sub->GetState() == SBuyItemInfo::e_own,
                        _state_names[_state]);
                }

                if (IsAddonAttached(iinfo, at_scope))
                {
                    SBuyItemInfo* detached_addon = DetachAddon(iinfo, at_scope);
                    detached_addon->SetState(SBuyItemInfo::e_undefined);
                    detached_addon->SetState(SBuyItemInfo::e_shop);
                    detached_addon->m_cell_item->SetOwnerList(nullptr);
                }
                if (IsAddonAttached(iinfo, at_silencer))
                {
                    SBuyItemInfo* detached_addon = DetachAddon(iinfo, at_silencer);
                    detached_addon->SetState(SBuyItemInfo::e_undefined);
                    detached_addon->SetState(SBuyItemInfo::e_shop);
                    detached_addon->m_cell_item->SetOwnerList(nullptr);
                }
                if (IsAddonAttached(iinfo, at_glauncher))
                {
                    SBuyItemInfo* detached_addon = DetachAddon(iinfo, at_glauncher);
                    detached_addon->SetState(SBuyItemInfo::e_undefined);
                    detached_addon->SetState(SBuyItemInfo::e_shop);
                    detached_addon->m_cell_item->SetOwnerList(nullptr);
                }

                iinfo->SetState(SBuyItemInfo::e_undefined);
                iinfo->SetState(SBuyItemInfo::e_shop);
                iinfo->m_cell_item->SetOwnerList(nullptr);
            }
        } while (iinfo);
    }
    for (u32 i = e_first; i < e_player_total; ++i)
        m_list[i]->ClearAll(false);
}

void CUIMpTradeWnd::SellAll()
{
    SBuyItemInfo* iinfo = nullptr;
    SBuyItemInfo* tmp_iinfo = nullptr;
    bool b_ok = true;

    do
    {
        iinfo = FindItem(SBuyItemInfo::e_bought);
        if (iinfo)
            b_ok = TryToSellItem(iinfo, true, tmp_iinfo);

        R_ASSERT(b_ok);
    } while (iinfo);

    do
    {
        iinfo = FindItem(SBuyItemInfo::e_own);
        if (iinfo)
        {
            VERIFY(iinfo->m_cell_item->OwnerList());
            CUICellItem* citem = iinfo->m_cell_item->OwnerList()->RemoveItem(iinfo->m_cell_item, false);
            SBuyItemInfo* iinfo_int = FindItem(citem);
            R_ASSERT(TryToSellItem(iinfo_int, true, tmp_iinfo));
        }
    } while (iinfo);
}

void CUIMpTradeWnd::ResetToOrigin()
{
    // 1-sell all bought items
    // 2-buy all sold items

    SBuyItemInfo* iinfo = nullptr;
    SBuyItemInfo* tmp_iinfo = nullptr;
    bool b_ok = true;

    DeleteHelperItems();

    do
    {
        iinfo = FindItem(SBuyItemInfo::e_bought);
        if (iinfo)
            b_ok = TryToSellItem(iinfo, true, tmp_iinfo);

        R_ASSERT(b_ok);
    } while (iinfo);

    do
    {
        iinfo = FindItem(SBuyItemInfo::e_sold);
        if (iinfo)
            b_ok = TryToBuyItem(iinfo, bf_normal, nullptr);

        R_ASSERT(b_ok);
    } while (iinfo);
}

void CUIMpTradeWnd::OnBtnSellClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    CUIDragDropListEx* pList = m_list[e_player_bag];

    SBuyItemInfo* iinfo = nullptr;
    while (pList->ItemsCount())
    {
        CUICellItem* ci = pList->GetItemIdx(0);
        iinfo = FindItem(ci);
        bool b_ok = true;
        SBuyItemInfo* tmp_iinfo = nullptr;
        b_ok = TryToSellItem(iinfo, true, tmp_iinfo);
        R_ASSERT(b_ok);
    }
}

void CUIMpTradeWnd::SetupPlayerItemsBegin()
{
    for (int idx = e_first; idx < e_player_total; ++idx)
    {
        CUIDragDropListEx* lst = m_list[idx];
        R_ASSERT(0 == lst->ItemsCount());
    }

    UpdateHelperItems();
}

void CUIMpTradeWnd::SetupPlayerItemsEnd() { StorePreset(_preset_idx_origin, true, false, true); }
void CUIMpTradeWnd::SetupDefaultItemsBegin() { SetupPlayerItemsBegin(); }
void CUIMpTradeWnd::SetupDefaultItemsEnd() { StorePreset(_preset_idx_default, true, false, true); }
void CUIMpTradeWnd::CheckDragItemToDestroy()
{
    if (CUIDragDropListEx::m_drag_item && CUIDragDropListEx::m_drag_item->ParentItem())
    {
        CUIDragDropListEx* _drag_owner = CUIDragDropListEx::m_drag_item->ParentItem()->OwnerList();
        _drag_owner->DestroyDragItem();
    }
}

struct items_sorter
{
    items_sorter(){};
    bool operator()(SBuyItemInfo* i1, SBuyItemInfo* i2)
    {
        if (i1->m_name_sect == i2->m_name_sect)
            return i1->GetState() < i2->GetState();

        return i1->m_name_sect < i2->m_name_sect;
    };
};

void CUIMpTradeWnd::DumpAllItems(LPCSTR s)
{
    std::sort(m_all_items.begin(), m_all_items.end(), items_sorter());

#ifndef MASTER_GOLD
    Msg("CUIMpTradeWnd::DumpAllItems.total[%d] reason [%s]", m_all_items.size(), s);
    auto it = m_all_items.begin();
    auto it_e = m_all_items.end();
    for (; it != it_e; ++it)
    {
        SBuyItemInfo* iinfo = *it;
        Msg("[%s] state[%s]", iinfo->m_name_sect.c_str(), iinfo->GetStateAsText());
    }
    Msg("------");
#endif // #ifndef MASTER_GOLD
}

void CUIMpTradeWnd::DumpPreset(ETradePreset idx)
{
#ifndef MASTER_GOLD
    const preset_items& v = GetPreset(idx);
    preset_items::const_iterator it = v.begin();
    preset_items::const_iterator it_e = v.end();

    Msg("dump preset [%d]", idx);
    for (; it != it_e; ++it)
    {
        const _preset_item& _one = *it;

        Msg("[%s]-[%d]", _one.sect_name.c_str(), _one.count);

        if (_one.addon_names[0].c_str())
            Msg("	[%s]", _one.addon_names[0].c_str());
        if (_one.addon_names[1].c_str())
            Msg("	[%s]", _one.addon_names[1].c_str());
        if (_one.addon_names[2].c_str())
            Msg("	[%s]", _one.addon_names[2].c_str());
    }
#endif // #ifndef MASTER_GOLD
}

void CUICellItemTradeMenuDraw::OnDraw(CUICellItem* cell)
{
    Fvector2 pos;
    cell->GetAbsolutePos(pos);
    UI().ClientToScreenScaled(pos, pos.x, pos.y);

    int acc = cell->GetAccelerator();
    if (acc != 0)
    {
        if (acc == 11)
            acc = 1;
        string64 buff;

        xr_sprintf(buff, " %d", acc - SDL_SCANCODE_ESCAPE);
        CGameFont* pFont = UI().Font().pFontLetterica16Russian;
        pFont->SetAligment(CGameFont::alCenter);
        pFont->SetColor(color_rgba(135, 123, 116, 255));
        pFont->Out(pos.x, pos.y, buff);
        pFont->OnRender();
    }

    bool b_can_buy_rank =
        m_trade_wnd->CheckBuyPossibility(m_info_item->m_name_sect, CUIMpTradeWnd::bf_check_rank_restr, true);

    if (!b_can_buy_rank)
    {
        cell->SetTextureColor(m_trade_wnd->m_item_color_restr_rank);
        return;
    }
    bool b_can_buy_money =
        m_trade_wnd->CheckBuyPossibility(m_info_item->m_name_sect, CUIMpTradeWnd::bf_check_money, true);
    if (!b_can_buy_money)
    {
        cell->SetTextureColor(m_trade_wnd->m_item_color_restr_money);
        return;
    }
    cell->SetTextureColor(m_trade_wnd->m_item_color_normal);
}
