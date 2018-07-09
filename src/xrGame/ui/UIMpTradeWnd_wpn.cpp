#include "stdafx.h"
#include "UIMpTradeWnd.h"
#include "UIDragDropListEx.h"
#include "UICellItem.h"
#include "WeaponMagazinedWGrenade.h"
#include "xrEngine/xr_input.h"
#include "UIMpItemsStoreWnd.h"

void CUIMpTradeWnd::OnBtnPistolAmmoClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    CUIDragDropListEx* res = m_list[e_pistol];
    CUICellItem* ci = (res->ItemsCount()) ? res->GetItemIdx(0) : NULL;
    if (!ci)
        return;

    CInventoryItem* ii = (CInventoryItem*)ci->m_pData;
    CWeapon* wpn = smart_cast<CWeapon*>(ii);
    R_ASSERT(wpn);

    u32 ammo_idx = (pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT)) ? 1 : 0;

    if (wpn->m_ammoTypes.size() < ammo_idx + 1)
        return;
    const shared_str& ammo_name = wpn->m_ammoTypes[ammo_idx];

    if (NULL == m_store_hierarchy->FindItem(ammo_name))
        return;

    SBuyItemInfo* pitem = CreateItem(ammo_name, SBuyItemInfo::e_undefined, false);
    bool b_res = TryToBuyItem(pitem, bf_normal, NULL);
    if (!b_res)
        DestroyItem(pitem);
}

void CUIMpTradeWnd::OnBtnPistolSilencerClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    CUIDragDropListEx* res = m_list[e_pistol];
    CUICellItem* ci = (res->ItemsCount()) ? res->GetItemIdx(0) : NULL;
    if (!ci)
        return;

    SBuyItemInfo* pitem = FindItem(ci);
    if (IsAddonAttached(pitem, at_silencer))
    { // detach
        SellItemAddons(pitem, at_silencer);
    }
    else if (CanAttachAddon(pitem, at_silencer))
    { // attach
        shared_str addon_name = GetAddonNameSect(pitem, at_silencer);

        if (NULL == m_store_hierarchy->FindItem(addon_name))
            return;

        SBuyItemInfo* addon_item = CreateItem(addon_name, SBuyItemInfo::e_undefined, false);
        bool b_res_addon = TryToBuyItem(addon_item, bf_normal, pitem);
        if (!b_res_addon)
            DestroyItem(addon_item);
    }
}

void CUIMpTradeWnd::OnBtnRifleAmmoClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    CUIDragDropListEx* res = m_list[e_rifle];
    CUICellItem* ci = (res->ItemsCount()) ? res->GetItemIdx(0) : NULL;
    if (!ci)
        return;

    CInventoryItem* ii = (CInventoryItem*)ci->m_pData;
    CWeapon* wpn = smart_cast<CWeapon*>(ii);
    R_ASSERT(wpn);

    u32 ammo_idx = (pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT)) ? 1 : 0;

    if (wpn->m_ammoTypes.size() < ammo_idx + 1)
        return;

    const shared_str& ammo_name = wpn->m_ammoTypes[ammo_idx];

    if (NULL == m_store_hierarchy->FindItem(ammo_name))
        return;

    SBuyItemInfo* pitem = CreateItem(ammo_name, SBuyItemInfo::e_undefined, false);
    bool b_res = TryToBuyItem(pitem, bf_normal, NULL);
    if (!b_res)
        DestroyItem(pitem);
}

void CUIMpTradeWnd::OnBtnRifleSilencerClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    CUIDragDropListEx* res = m_list[e_rifle];
    CUICellItem* ci = (res->ItemsCount()) ? res->GetItemIdx(0) : NULL;
    if (!ci)
        return;

    SBuyItemInfo* pitem = FindItem(ci);
    if (IsAddonAttached(pitem, at_silencer))
    { // detach
        SellItemAddons(pitem, at_silencer);
    }
    else if (CanAttachAddon(pitem, at_silencer))
    { // attach
        shared_str addon_name = GetAddonNameSect(pitem, at_silencer);

        if (NULL == m_store_hierarchy->FindItem(addon_name))
            return;

        SBuyItemInfo* addon_item = CreateItem(addon_name, SBuyItemInfo::e_undefined, false);
        bool b_res_addon = TryToBuyItem(addon_item, bf_normal, pitem);
        if (!b_res_addon)
            DestroyItem(addon_item);
    }
}

void CUIMpTradeWnd::OnBtnRifleScopeClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    CUIDragDropListEx* res = m_list[e_rifle];
    CUICellItem* ci = (res->ItemsCount()) ? res->GetItemIdx(0) : NULL;
    if (!ci)
        return;

    SBuyItemInfo* pitem = FindItem(ci);
    if (IsAddonAttached(pitem, at_scope))
    { // detach
        SellItemAddons(pitem, at_scope);
    }
    else if (CanAttachAddon(pitem, at_scope))
    { // attach
        shared_str addon_name = GetAddonNameSect(pitem, at_scope);

        if (NULL == m_store_hierarchy->FindItem(addon_name))
            return;

        SBuyItemInfo* addon_item = CreateItem(addon_name, SBuyItemInfo::e_undefined, false);
        bool b_res_addon = TryToBuyItem(addon_item, bf_normal, pitem);
        if (!b_res_addon)
            DestroyItem(addon_item);
    }
}

void CUIMpTradeWnd::OnBtnRifleGLClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    CUIDragDropListEx* res = m_list[e_rifle];
    CUICellItem* ci = (res->ItemsCount()) ? res->GetItemIdx(0) : NULL;
    if (!ci)
        return;

    SBuyItemInfo* pitem = FindItem(ci);
    if (IsAddonAttached(pitem, at_glauncher))
    { // detach
        SellItemAddons(pitem, at_glauncher);
    }
    else if (CanAttachAddon(pitem, at_glauncher))
    { // attach
        shared_str addon_name = GetAddonNameSect(pitem, at_glauncher);

        if (NULL == m_store_hierarchy->FindItem(addon_name))
            return;

        SBuyItemInfo* addon_item = CreateItem(addon_name, SBuyItemInfo::e_undefined, false);
        bool b_res_addon = TryToBuyItem(addon_item, bf_normal, pitem);
        if (!b_res_addon)
            DestroyItem(addon_item);
    }

    DeleteHelperItems(m_list[e_rifle_ammo]);
    UpdateCorrespondingItemsForList(m_list[e_rifle]);
}

void CUIMpTradeWnd::OnBtnRifleAmmo2Clicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    CUIDragDropListEx* res = m_list[e_rifle];
    CUICellItem* ci = (res->ItemsCount()) ? res->GetItemIdx(0) : NULL;
    if (!ci)
        return;

    CInventoryItem* ii = (CInventoryItem*)ci->m_pData;
    CWeaponMagazinedWGrenade* wpn = smart_cast<CWeaponMagazinedWGrenade*>(ii);
    if (!wpn)
        return;

    u32 ammo_idx = 0;

    const shared_str& ammo_name = wpn->m_ammoTypes2[ammo_idx];

    if (NULL == m_store_hierarchy->FindItem(ammo_name))
        return;

    SBuyItemInfo* pitem = CreateItem(ammo_name, SBuyItemInfo::e_undefined, false);
    bool b_res = TryToBuyItem(pitem, bf_normal, NULL);
    if (!b_res)
        DestroyItem(pitem);
}

bool CUIMpTradeWnd::TryToAttachItemAsAddon(SBuyItemInfo* itm, SBuyItemInfo* itm_parent)
{
    bool b_res = false;

    item_addon_type _addon_type = GetItemType(itm->m_name_sect);
    if (_addon_type == at_not_addon)
        return b_res;

    if (itm_parent)
    {
        if (CanAttachAddon(itm_parent, _addon_type))
        {
            return AttachAddon(itm_parent, _addon_type);
        }
    }
    else // auto-attach
        for (u32 i = 0; i < 2; ++i)
        {
            u32 list_idx = (i == 0) ? e_rifle : e_pistol;
            CUIDragDropListEx* _list = m_list[list_idx];

            VERIFY(_list->ItemsCount() <= 1);

            CUICellItem* ci = (_list->ItemsCount()) ? _list->GetItemIdx(0) : NULL;
            if (!ci)
                return false;

            SBuyItemInfo* attach_to = FindItem(ci);

            if (CanAttachAddon(attach_to, _addon_type))
            {
                AttachAddon(attach_to, _addon_type);
                b_res = true;
                break;
            }
        }

    return b_res;
}

void CUIMpTradeWnd::SellItemAddons(SBuyItemInfo* sell_itm, item_addon_type addon_type)
{
    CInventoryItem* item_ = (CInventoryItem*)sell_itm->m_cell_item->m_pData;
    CWeapon* w = smart_cast<CWeapon*>(item_);
    if (!w)
        return; // ammo,medkit etc.

    if (IsAddonAttached(sell_itm, addon_type))
    {
        SBuyItemInfo* detached_addon = DetachAddon(sell_itm, addon_type);
        u32 _item_cost = m_item_mngr->GetItemCost(detached_addon->m_name_sect, GetRank());
        SetMoneyAmount(GetMoneyAmount() + _item_cost);
        DestroyItem(detached_addon);

        if (addon_type == at_glauncher)
        {
            CWeaponMagazinedWGrenade* wpn2 = smart_cast<CWeaponMagazinedWGrenade*>(item_);
            VERIFY(wpn2);

            for (u32 ammo_idx = 0; ammo_idx < wpn2->m_ammoTypes2.size(); ++ammo_idx)
            {
                const shared_str& ammo_name = wpn2->m_ammoTypes2[ammo_idx];
                SBuyItemInfo* ammo = NULL;

                while ((ammo = FindItem(ammo_name, SBuyItemInfo::e_bought)) != NULL)
                {
                    SBuyItemInfo* tempo = NULL;
                    TryToSellItem(ammo, true, tempo);
                }
            }
        }
    }
}

bool CUIMpTradeWnd::IsAddonAttached(SBuyItemInfo* itm, item_addon_type at)
{
    bool b_res = false;
    CInventoryItem* item_ = (CInventoryItem*)itm->m_cell_item->m_pData;
    CWeapon* w = smart_cast<CWeapon*>(item_);

    if (!w)
        return b_res;
    switch (at)
    {
    case at_scope: { b_res = (w->ScopeAttachable() && w->IsScopeAttached());
    }
    break;

    case at_silencer: { b_res = (w->SilencerAttachable() && w->IsSilencerAttached());
    }
    break;

    case at_glauncher: { b_res = (w->GrenadeLauncherAttachable() && w->IsGrenadeLauncherAttached());
    }
    break;
    };
    return b_res;
}

bool CUIMpTradeWnd::CanAttachAddon(SBuyItemInfo* itm, item_addon_type at)
{
    if (IsAddonAttached(itm, at))
        return false;

    bool b_res = false;
    CInventoryItem* item_ = (CInventoryItem*)itm->m_cell_item->m_pData;
    CWeapon* w = smart_cast<CWeapon*>(item_);

    if (!w)
        return b_res;
    switch (at)
    {
    case at_scope: { b_res = (w->ScopeAttachable() && !w->IsScopeAttached());
    }
    break;

    case at_silencer: { b_res = (w->SilencerAttachable() && !w->IsSilencerAttached());
    }
    break;

    case at_glauncher: { b_res = (w->GrenadeLauncherAttachable() && !w->IsGrenadeLauncherAttached());
    }
    break;
    };
    return b_res;
}

SBuyItemInfo* CUIMpTradeWnd::DetachAddon(SBuyItemInfo* itm, item_addon_type at)
{
    VERIFY(IsAddonAttached(itm, at));

    CInventoryItem* item_ = (CInventoryItem*)itm->m_cell_item->m_pData;
    CWeapon* w = smart_cast<CWeapon*>(item_);
    R_ASSERT(w);

    u8 curr_addon_state = w->GetAddonsState();
    curr_addon_state &= ~at;

    shared_str addon_name_sect = GetAddonNameSect(itm, at);

    w->SetAddonsState(curr_addon_state);
    SBuyItemInfo* detached_addon = CreateItem(addon_name_sect, SBuyItemInfo::e_own, false);
    return detached_addon;
}

shared_str CUIMpTradeWnd::GetAddonNameSect(SBuyItemInfo* itm, item_addon_type at)
{
    CInventoryItem* item_ = (CInventoryItem*)itm->m_cell_item->m_pData;
    CWeapon* w = smart_cast<CWeapon*>(item_);

    switch (at)
    {
    case at_scope: { return w->GetScopeName();
    }
    break;

    case at_silencer: { return w->GetSilencerName();
    }
    break;

    case at_glauncher: { return w->GetGrenadeLauncherName();
    }
    break;
    };
    return NULL;
}

bool CUIMpTradeWnd::AttachAddon(SBuyItemInfo* itm, item_addon_type at)
{
    VERIFY(!IsAddonAttached(itm, at));

    CInventoryItem* item_ = (CInventoryItem*)itm->m_cell_item->m_pData;
    CWeapon* w = smart_cast<CWeapon*>(item_);
    R_ASSERT(w);

    u8 curr_addon_state = w->GetAddonsState();
    curr_addon_state |= at;
    w->SetAddonsState(curr_addon_state);
    return true;
}

CUIMpTradeWnd::item_addon_type CUIMpTradeWnd::GetItemType(const shared_str& name_sect)
{
    const shared_str& group = g_mp_restrictions.GetItemGroup(name_sect);
    if (group == "scp")
        return at_scope;
    else if (group == "sil")
        return at_silencer;
    else if (group == "gl")
        return at_glauncher;
    else
        return at_not_addon;
}

u8 GetItemAddonsState_ext(SBuyItemInfo* item)
{
    CInventoryItem* item_ = (CInventoryItem*)item->m_cell_item->m_pData;
    CWeapon* w = smart_cast<CWeapon*>(item_);
    if (!w)
        return 0;
    return w->GetAddonsState();
}

void SetItemAddonsState_ext(SBuyItemInfo* item, u8 addons)
{
    CInventoryItem* item_ = (CInventoryItem*)item->m_cell_item->m_pData;
    CWeapon* w = smart_cast<CWeapon*>(item_);
    if (!w)
        return;

    w->SetAddonsState(addons);
}
