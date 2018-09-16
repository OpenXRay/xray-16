#include "stdafx.h"
#include "UIMpTradeWnd.h"
#include "UIMpItemsStoreWnd.h"
#include "UITabButtonMP.h"
#include "UITabControl.h"
#include "UIDragDropListEx.h"
#include "UICellItem.h"
#include "UIDialogHolder.h"
#include "game_cl_deathmatch.h"

void CUIMpTradeWnd::OnBtnOkClicked(CUIWindow* w, void* d)
{
    DeleteHelperItems();
    CheckDragItemToDestroy();
    StorePreset(_preset_idx_last, true, false, false);
    HideDialog();
    game_cl_mp* clgame = smart_cast<game_cl_mp*>(&(Game()));
    VERIFY(clgame);
    clgame->OnBuyMenu_Ok();
    //.	StorePreset							(_preset_idx_last, true, true);
}

void CUIMpTradeWnd::OnBtnCancelClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    HideDialog();
    game_cl_mp* clgame = smart_cast<game_cl_mp*>(&(Game()));
    VERIFY(clgame);
    clgame->OnBuyMenu_Cancel();
}

void CUIMpTradeWnd::OnBtnShopBackClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    m_store_hierarchy->MoveUp();
    UpdateShop();
}

void CUIMpTradeWnd::TryUsePreset(ETradePreset idx)
{
    VERIFY(IsShown());
    u32 _cost = GetPresetCost(idx);
    bool b_has_enought_money = _cost <= GetMoneyAmount();
    if (!b_has_enought_money)
        return;

    ApplyPreset(idx);
}

#include "xrEngine/xr_input.h"
void CUIMpTradeWnd::OnBtnPreset1Clicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT))
    {
        DumpPreset(_preset_idx_1);
        return;
    }

    ApplyPreset(_preset_idx_1);
}

void CUIMpTradeWnd::OnBtnPreset2Clicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT))
    {
        DumpPreset(_preset_idx_2);
        return;
    };
    ApplyPreset(_preset_idx_2);
}

void CUIMpTradeWnd::OnBtnPreset3Clicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT))
    {
        DumpPreset(_preset_idx_3);
        return;
    };

    ApplyPreset(_preset_idx_3);
}

void CUIMpTradeWnd::OnBtnLastSetClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT))
    {
        DumpPreset(_preset_idx_last);
        return;
    };
    ApplyPreset(_preset_idx_last);
}

void xr_stdcall CUIMpTradeWnd::OnBtnPresetDefaultClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    {
        if (pInput->iGetAsyncKeyState(SDL_SCANCODE_LSHIFT))
        {
            DumpPreset(_preset_idx_default);
            return;
        };
        ApplyPreset(_preset_idx_default);
    }
}

void CUIMpTradeWnd::OnBtnSave1PresetClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    StorePreset(_preset_idx_1, false, true, true);
}

void CUIMpTradeWnd::OnBtnSave2PresetClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    StorePreset(_preset_idx_2, false, true, true);
}

void CUIMpTradeWnd::OnBtnSave3PresetClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    StorePreset(_preset_idx_3, false, true, true);
}

void CUIMpTradeWnd::OnBtnResetClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    ApplyPreset(_preset_idx_origin); // origin
}

void CUIMpTradeWnd::OnRootTabChanged(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    const shared_str& curr = m_root_tab_control->GetActiveId();
    m_store_hierarchy->Reset();
    m_store_hierarchy->MoveDown(curr);

    UpdateShop();
}

void CUIMpTradeWnd::OnSubLevelBtnClicked(CUIWindow* w, void* d)
{
    CheckDragItemToDestroy();
    CUITabButtonMP* btn = smart_cast<CUITabButtonMP*>(w);

    shared_str curr = btn->m_temp_id;
    m_store_hierarchy->MoveDown(curr);

    UpdateShop();
}

void CUIMpTradeWnd::UpdateShop()
{
    m_shop_wnd->DetachAll();

    bool b_matched_root = m_store_hierarchy->CurrentIsRoot();
    m_btn_shop_back->Enable(!b_matched_root);
    if (b_matched_root)
        m_root_tab_control->ResetTab();

#ifdef DEBUG
    Msg("current level=[%s]", m_store_hierarchy->CurrentLevel().m_name.c_str());
#endif // #ifdef DEBUG
    SetCurrentItem(NULL);
    if (m_store_hierarchy->CurrentIsRoot())
        return;

    if (m_store_hierarchy->CurrentLevel().HasSubLevels())
    { // show sub-levels
        FillUpSubLevelButtons();
    }
    else
    { // show items
        FillUpSubLevelItems();
    }
}

void CUIMpTradeWnd::FillUpSubLevelButtons()
{
    u32 root_cnt = m_store_hierarchy->CurrentLevel().ChildCount();

    //	Fvector2							pos;
    //	pos.set								(40.0f,90.0f);

    for (u32 i = 0; i < root_cnt; ++i)
    {
        const CStoreHierarchy::item& it = m_store_hierarchy->CurrentLevel().ChildAtIdx(i);
        CUITabButtonMP* btn = it.m_button;
        btn->m_temp_id = it.m_name;
        Register(btn);
        //		btn->SetWndPos					(pos);
        //		pos.add							(btn->GetWndSize().y);
        //		pos.y							+= 40.0f;
        //		pos.x							= 40.0f;
        m_shop_wnd->AttachChild(btn);
    }
}

void CUIMpTradeWnd::FillUpSubLevelItems()
{
    CUIDragDropListEx* pList = m_list[e_shop];
    m_shop_wnd->AttachChild(pList);
    pList->ClearAll(false);

    const CStoreHierarchy::item& curr_level = m_store_hierarchy->CurrentLevel();

    for (u32 idx = 0; idx < curr_level.m_items_in_group.size(); ++idx)
    {
        const shared_str& sect = curr_level.m_items_in_group[idx];
        RenewShopItem(sect, false);
    }
}

#include "Actor.h"
void CUIMpTradeWnd::Show(bool status)
{
    inherited::Show(status);

    CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (pActor)
        pActor->SetWeaponHideState(INV_STATE_BUY_MENU, status);

    if (status)
    {
        m_pMouseCapturer = NULL;

        m_static_information->SetText("");
        m_static_money_change->SetText("");
    }
    else
    {
        CheckDragItemToDestroy();
        CleanUserItems();
    }
}

bool CUIMpTradeWnd::IsIgnoreMoneyAndRank() { return m_bIgnoreMoneyAndRank; }
