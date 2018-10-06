#pragma once

#include "UIBuyWndShared.h"
#include "UIBuyWndBase.h"
#include "xrUICore/Callbacks/UIWndCallback.h"
#include "Restrictions.h"
#include "UIMpItemsStoreWnd.h"

class CUIDragDropListEx;
class CUI3tButton;
class CUIStatic;
class CUITextWnd;
class CUIMpItemsStoreWnd;
class CUITabControl;
class CUICellItem;
class CInventoryItem;
class CItemMgr;
class CUIItemInfo;

struct SBuyItemInfo
{
    enum EItmState
    {
        e_undefined,
        e_bought,
        e_sold,
        e_own,
        e_shop
    };

    ~SBuyItemInfo();
    SBuyItemInfo();
    shared_str m_name_sect;
    CUICellItem* m_cell_item;

    const EItmState& GetState() const { return m_item_state; }
    void SetState(const EItmState& s);
    LPCSTR GetStateAsText() const;

private:
    EItmState m_item_state;
};

using ITEMS_vec = xr_vector<SBuyItemInfo*>;

class CUIMpTradeWnd : public IBuyWnd, public CUIWndCallback
{
    typedef CUIDialogWnd inherited;
    friend class CUICellItemTradeMenuDraw;

public:
    enum
    {
        e_first = 0,
        e_pistol = e_first,
        e_pistol_ammo,
        e_rifle,
        e_rifle_ammo,
        e_outfit,
        e_medkit,
        e_granade,
        e_others,
        e_player_bag,
        e_player_total,
        e_shop = e_player_total,
        e_total_lists,
    };
    enum dd_list_type
    {
        dd_shop = 0,
        dd_own_bag = 1,
        dd_own_slot = 2,
    };
    enum item_addon_type
    {
        at_not_addon = 0,
        at_scope = 0x1,
        at_glauncher = 0x2,
        at_silencer = 0x4,
    };
    enum _buy_flags
    {
        bf_check_money = (1 << 0),
        bf_check_rank_restr = (1 << 1),
        bf_check_count_restr = (1 << 2),
        bf_own_itm = (1 << 3),
        bf_ignore_team = (1 << 4),

        bf_normal = bf_check_money | bf_check_rank_restr | bf_check_count_restr,
    };

    CUIMpTradeWnd();
    virtual ~CUIMpTradeWnd();
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);

    //
    virtual void Update();
    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);

    virtual void Init(const shared_str& sectionName, const shared_str& sectionPrice);
    virtual void BindDragDropListEvents(CUIDragDropListEx* lst, bool bDrag);

    virtual void GetWeaponIndexByName(const shared_str& sectionName, u8& grpNum, u8& idx);
    virtual u32 GetMoneyAmount() const;
    virtual void SetMoneyAmount(u32 money);
    virtual void IgnoreMoney(bool ignore);
    virtual void SectionToSlot(const u8 grpNum, u8 uIndexInSlot, bool bRealRepresentationSet);
    virtual bool CheckBuyAvailabilityInSlots();
    virtual void AddonToSlot(int add_on, int slot, bool bRealRepresentationSet);
    virtual const shared_str& GetWeaponNameByIndex(u8 grpNum, u8 idx);
    virtual void IgnoreMoneyAndRank(bool ignore);
    virtual bool CanBuyAllItems();
    virtual void ResetItems();
    virtual void SetRank(u32 rank);
    virtual u32 GetRank();

    virtual void ItemToBelt(const shared_str& sectionName);
    virtual void ItemToRuck(const shared_str& sectionName, u8 addons);
    virtual void ItemToSlot(const shared_str& sectionName, u8 addons);
    virtual void SetupPlayerItemsBegin();
    virtual void SetupPlayerItemsEnd();
    virtual void SetupDefaultItemsBegin();
    virtual void SetupDefaultItemsEnd();

    virtual const preset_items& GetPreset(ETradePreset idx);
    virtual u32 GetPresetCost(ETradePreset idx);
    virtual void ClearPreset(ETradePreset idx);
    virtual void TryUsePreset(ETradePreset idx);
    virtual void Show(bool status);
    virtual bool IsIgnoreMoneyAndRank();

    bool HasItemInGroup(shared_str const& section_name);
    CItemMgr const* GetItemMngr() const { return m_item_mngr; };
private:
    // data
    shared_str m_sectionName;
    shared_str m_sectionPrice;
    u32 m_money;
    CStoreHierarchy* m_store_hierarchy;
    CUICellItem* m_pCurrentCellItem;
    ITEMS_vec m_all_items;
    CItemMgr* m_item_mngr;
    preset_items m_preset_storage[_preset_count];
    bool m_bIgnoreMoneyAndRank;
    // controls
    CUIWindow* m_shop_wnd;
    CUITextWnd* m_static_curr_items_money;
    CUITextWnd* m_static_player_money;
    CUITextWnd* m_static_preset_money[5];
    CUIStatic* m_static_player_rank;
    CUITextWnd* m_static_information;
    CUITextWnd* m_static_money_change;
    CUI3tButton* m_btn_shop_back;
    CUI3tButton* m_btn_ok;
    CUI3tButton* m_btn_cancel;

    CUI3tButton* m_btns_preset[5];
    CUI3tButton* m_btns_save_preset[3];
    CUI3tButton* m_btn_reset;
    CUI3tButton* m_btn_sell;

    CUI3tButton* m_btn_pistol_ammo;
    CUI3tButton* m_btn_pistol_silencer;
    CUI3tButton* m_btn_rifle_ammo;
    CUI3tButton* m_btn_rifle_silencer;
    CUI3tButton* m_btn_rifle_scope;
    CUI3tButton* m_btn_rifle_glauncher;
    CUI3tButton* m_btn_rifle_ammo2;
    CUIItemInfo* m_item_info;
    CUIStatic* m_static_item_rank;
    u32 m_item_color_restr_rank;
    u32 m_item_color_restr_money;
    u32 m_item_color_normal;

    u32 m_text_color_money_positive;
    u32 m_text_color_money_negative;

    CUITabControl* m_root_tab_control;
    CUIDragDropListEx* m_list[e_total_lists];

    void UpdateHelperItems();
    void CreateHelperItems(CUIDragDropListEx* list);
    void CreateHelperItems(xr_vector<shared_str>& ammo_types);
    void CreateHelperItems(CUIDragDropListEx* list, const CStoreHierarchy::item* shop_level);
    void DeleteHelperItems(CUIDragDropListEx* list);
    void DeleteHelperItems();
    bool CanBuyOrSellInList(CUIDragDropListEx* list);

    void UpdateMoneyIndicator();
    void UpdateShop();

    // handlers
    void xr_stdcall OnBtnOkClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnCancelClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnShopBackClicked(CUIWindow* w, void* d);
    void xr_stdcall OnRootTabChanged(CUIWindow* w, void* d);
    void xr_stdcall OnSubLevelBtnClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnPreset1Clicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnPreset2Clicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnPreset3Clicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnPresetDefaultClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnLastSetClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnSave1PresetClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnSave2PresetClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnSave3PresetClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnResetClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnSellClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnPistolAmmoClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnPistolSilencerClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnRifleAmmoClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnRifleSilencerClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnRifleScopeClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnRifleGLClicked(CUIWindow* w, void* d);
    void xr_stdcall OnBtnRifleAmmo2Clicked(CUIWindow* w, void* d);

    void CheckDragItemToDestroy();

    // drag drop handlers
    bool xr_stdcall OnItemDrop(CUICellItem* itm);
    bool xr_stdcall OnItemStartDrag(CUICellItem* itm);
    bool xr_stdcall OnItemDbClick(CUICellItem* itm);
    bool xr_stdcall OnItemSelected(CUICellItem* itm);
    bool xr_stdcall OnItemRButtonClick(CUICellItem* itm);
    bool xr_stdcall OnItemLButtonClick(CUICellItem* itm);

    void FillUpSubLevelButtons();
    void FillUpSubLevelItems();

    bool TryToBuyItem(SBuyItemInfo* itm, u32 buy_flags, SBuyItemInfo* itm_parent);
    bool TryToSellItem(SBuyItemInfo* itm, bool do_destroy, SBuyItemInfo*& itm_res);
    bool BuyItemAction(SBuyItemInfo* itm);
    bool CheckBuyPossibility(const shared_str& sect_name, u32 buy_flags, bool b_silent);

    //---item addons---
    bool TryToAttachItemAsAddon(SBuyItemInfo* buy_itm, SBuyItemInfo* itm_parent);
    void SellItemAddons(SBuyItemInfo* sell_itm, item_addon_type at);

    bool IsAddonAttached(SBuyItemInfo* sell_itm, item_addon_type at);
    bool CanAttachAddon(SBuyItemInfo* sell_itm, item_addon_type at);
    SBuyItemInfo* DetachAddon(SBuyItemInfo* sell_itm, item_addon_type at);
    bool AttachAddon(SBuyItemInfo* sell_itm, item_addon_type at);
    item_addon_type GetItemType(const shared_str& name_sect);
    shared_str GetAddonNameSect(SBuyItemInfo* itm, item_addon_type at);
    //-----

    void SetCurrentItem(CUICellItem* itm);
    CInventoryItem* CurrentIItem();
    CUICellItem* CurrentItem();
    int GetItemPrice(CInventoryItem* itm);

    CInventoryItem* CreateItem_internal(const shared_str& name_sect);
    SBuyItemInfo* CreateItem(const shared_str& name_sect, SBuyItemInfo::EItmState state, bool find_if_exist);
    SBuyItemInfo* FindItem(CUICellItem* item);
    SBuyItemInfo* FindItem(const shared_str& name_sect, SBuyItemInfo::EItmState state);
    SBuyItemInfo* FindItem(SBuyItemInfo::EItmState state);
    void DestroyItem(SBuyItemInfo* item);

    void RenewShopItem(const shared_str& sect_name, bool b_just_bought);
    u32 GetItemCount(SBuyItemInfo::EItmState state) const;
    u32 GetItemCount(const shared_str& name_sect, SBuyItemInfo::EItmState state) const;
    u32 GetItemCount(const shared_str& name_sect, SBuyItemInfo::EItmState state, u8 addon) const;
    u32 GetGroupCount(const shared_str& name_group, SBuyItemInfo::EItmState state) const;

    void SellAll();
    void ResetToOrigin();
    void DumpPreset(ETradePreset idx);
    void DumpAllItems(LPCSTR reason);
    dd_list_type GetListType(CUIDragDropListEx* l);
    CUIDragDropListEx* GetMatchedListForItem(const shared_str& sect_name);
    void UpdateCorrespondingItemsForList(CUIDragDropListEx* _list);
    const u32 GetRank() const;
    void SetInfoString(LPCSTR str);
    void SetMoneyChangeString(int diff);
    void CleanUserItems();

    void ApplyPreset(ETradePreset idx);
    void StorePreset(ETradePreset idx, bool bSilent, bool check_allowed_items, bool flush_helpers);
};

u8 GetItemAddonsState_ext(SBuyItemInfo* item);
void SetItemAddonsState_ext(SBuyItemInfo* item, u8 addons);

#include "UICellItem.h"
class CUICellItemTradeMenuDraw : public ICustomDrawCellItem
{
    CUIMpTradeWnd* m_trade_wnd;
    SBuyItemInfo* m_info_item;

public:
    CUICellItemTradeMenuDraw(CUIMpTradeWnd* w, SBuyItemInfo* info) : m_trade_wnd(w), m_info_item(info) {}
    virtual void OnDraw(CUICellItem* cell);
};
