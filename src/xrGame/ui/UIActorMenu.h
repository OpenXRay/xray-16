#pragma once

#include "UIDialogWnd.h"
#include "xrUICore/Callbacks/UIWndCallback.h"
#include "xrServerEntities/inventory_space.h"
#include "xrUICore/Hint/UIHint.h"

class CUICharacterInfo;
class CUIDragDropListEx;
class CUIDragDropReferenceList;
class CUICellItem;
class CUIDragItem;
class ui_actor_state_wnd;
class CUIItemInfo;
class CUIFrameLineWnd;
class CUIStatic;
class CUITextWnd;
class CUI3tButton;
class CInventoryOwner;
class CInventoryBox;
class CUIInventoryUpgradeWnd;
class UIInvUpgradeInfo;
class CUIMessageBoxEx;
class StaticDrawableWrapper;
class CUIPropertiesBox;
class CTrade;
class CUIProgressBar;
class CUITradeBar;
class CUIWeightBar;

namespace inventory
{
namespace upgrade
{
class Upgrade;
}
} // namespace upgrade, inventory

enum EDDListType
{
    iInvalid,
    iActorSlot,
    iActorBag,
    iActorBelt,

    iActorTrade,
    iPartnerTradeBag,
    iPartnerTrade,
    iDeadBodyBag,
    iQuickSlot,
    iTrashSlot,
    iListTypeMax
};

enum EMenuMode
{
    mmUndefined,
    mmInventory,
    mmTrade,
    mmUpgrade,
    mmDeadBodySearch,
};

class CUIActorMenu : public CUIDialogWnd, public CUIWndCallback
{
    typedef CUIDialogWnd inherited;
    typedef inventory::upgrade::Upgrade Upgrade_type;

protected:
    enum eActorMenuSndAction
    {
        eSndOpen = 0,
        eSndClose,
        eItemToSlot,
        eItemToBelt,
        eItemToRuck,
        eProperties,
        eDropItem,
        eAttachAddon,
        eDetachAddon,
        eItemUse,
        eSndMax
    };

    enum eActorMenuListType
    {
        eInventoryPistolList,
        eInventoryAutomaticList,

        eInventoryOutfitList,
        eInventoryHelmetList,

        eInventoryBeltList,
        eInventoryDetectorList,

        eInventoryBagList,

        eTradeActorList,
        eTradeActorBagList,

        eTradePartnerList,
        eTradePartnerBagList,

        eSearchLootBagList,
        eSearchLootActorBagList,

        eTrashList,

        eListCount
    };

    EMenuMode m_currMenuMode;
    ref_sound sounds[eSndMax];
    void PlaySnd(eActorMenuSndAction a);

    CInventoryOwner* m_pActorInvOwner{};
    CInventoryOwner* m_pPartnerInvOwner{};
    CInventoryBox* m_pInvBox{};

    CTrade* m_actor_trade{};
    CTrade* m_partner_trade{};

    CUICellItem* m_pCurrentCellItem{};

    // Messages
    CUIPropertiesBox* m_UIPropertiesBox{};
    CUIMessageBoxEx* m_message_box_yes_no{};
    CUIMessageBoxEx* m_message_box_ok{};
    StaticDrawableWrapper* m_message_static{};

    // Windows
    CUIWindow* m_pInventoryWnd{};
    CUIWindow* m_pTradeWnd{};
    CUIWindow* m_pSearchLootWnd{};
    CUIInventoryUpgradeWnd* m_pUpgradeWnd{};
    UIInvUpgradeInfo* m_upgrade_info{};
    CUICellItem* m_upgrade_selected{};

    // Left side
    CUIStatic* m_LeftBackground{};

    // Character infos
    CUICharacterInfo* m_ActorCharacterInfo{};
    CUICharacterInfo* m_PartnerCharacterInfo{};
    CUICharacterInfo* m_TradeActorCharacterInfo{};
    CUICharacterInfo* m_TradePartnerCharacterInfo{};
    CUICharacterInfo* m_SearchLootActorCharacterInfo{};
    CUICharacterInfo* m_SearchLootPartnerCharacterInfo{};

    // Item infos
    CUIItemInfo* m_ItemInfo{};
    CUIItemInfo* m_ItemInfoInventoryMode{};
    CUIItemInfo* m_ItemInfoTradeMode{};
    CUIItemInfo* m_ItemInfoSearchLootMode{};

    // Money
    CUIStatic* m_ActorMoney{};
    CUIStatic* m_TradeActorMoney{};
    CUIStatic* m_PartnerMoney{};

    // Trade bar
    CUITradeBar* m_ActorTradeBar;
    CUITradeBar* m_PartnerTradeBar;

    // Weight bar
    CUIWeightBar* m_ActorWeightBar;
    CUIWeightBar* m_PartnerWeightBar;

    // Drag&Drop lists
    CUIDragDropListEx* m_pLists[eListCount]{};

public:
    CUIDragDropReferenceList* m_pQuickSlot{};

protected:
    // Condition bars
    ui_actor_state_wnd* m_ActorStateInfo{};

    // Buttons
    CUI3tButton* m_trade_button{};
    CUI3tButton* m_trade_buy_button{};
    CUI3tButton* m_trade_sell_button{};
    CUI3tButton* m_takeall_button{};
    CUI3tButton* m_exit_button{};

    // Helpers
    CUIStatic* m_clock_value{};
    UIHint* m_hint_wnd{};

protected:
    u32 m_last_time;
    bool m_repair_mode;
    bool m_item_info_view;
    bool m_highlight_clear;
    u32 m_trade_partner_inventory_state;

public:
    void SetMenuMode(EMenuMode mode);
    EMenuMode GetMenuMode() { return m_currMenuMode; };
    void SetActor(CInventoryOwner* io);
    void SetPartner(CInventoryOwner* io);
    CInventoryOwner* GetPartner() { return m_pPartnerInvOwner; };
    void SetInvBox(CInventoryBox* box);
    CInventoryBox* GetInvBox() { return m_pInvBox; };
private:
    void PropertiesBoxForSlots(PIItem item, bool& b_show);
    void PropertiesBoxForWeapon(CUICellItem* cell_item, PIItem item, bool& b_show);
    void PropertiesBoxForAddon(PIItem item, bool& b_show);
    void PropertiesBoxForUsing(PIItem item, bool& b_show);
    void PropertiesBoxForPlaying(PIItem item, bool& b_show);
    void PropertiesBoxForDrop(CUICellItem* cell_item, PIItem item, bool& b_show);
    void PropertiesBoxForRepair(PIItem item, bool& b_show);
    void PropertiesBoxForDonate(PIItem item, bool& b_show); //Alundaio

private:
    void clear_highlight_lists();
    void set_highlight_item(CUICellItem* cell_item);
    void highlight_item_slot(CUICellItem* cell_item);
    void highlight_armament(PIItem item, CUIDragDropListEx* ddlist);
    void highlight_ammo_for_weapon(PIItem weapon_item, CUIDragDropListEx* ddlist);
    void highlight_weapons_for_ammo(PIItem ammo_item, CUIDragDropListEx* ddlist);
    bool highlight_addons_for_weapon(PIItem weapon_item, CUICellItem* ci);
    void highlight_weapons_for_addon(PIItem addon_item, CUIDragDropListEx* ddlist);

protected:
    void Construct();
    void InitializeUniversal(CUIXml& uiXml);
    void InitializeUpgradeMode(CUIXml& uiXml);

    // Old style UI
    void InitializeInventoryMode(CUIXml& uiXml);
    void InitializeTradeMode(CUIXml& uiXml);
    void InitializeSearchLootMode(CUIXml& uiXml);
    // Old style UI

    void InitSounds(CUIXml& uiXml);
    void InitAllowedDrops();
    void InitCallbacks();

    void InitCellForSlot(u16 slot_idx);
    void InitInventoryContents(CUIDragDropListEx* pBagList, bool onlyBagList = false);
    void ClearAllLists();

    void BindDragDropListEvents(CUIDragDropListEx* lst);
    void RegisterCallback(CUIWindow* window, s16 event, const CUIWndCallback::void_function& callback);

    static bool ShowIfExist(CUIWindow* window, bool status); // sorry

    EDDListType GetListType(CUIDragDropListEx* l);

public:
    CUIDragDropListEx* GetListByType(EDDListType t); //Alundaio: Made public

protected:
    CUIDragDropListEx* GetSlotList(u16 slot_idx);
    bool CanSetItemToList(PIItem item, CUIDragDropListEx* l, u16& ret_slot);

    xr_vector<EDDListType> m_allowed_drops[iListTypeMax];
    bool AllowItemDrops(EDDListType from, EDDListType to);

    bool xr_stdcall OnItemDrop(CUICellItem* itm);
    bool xr_stdcall OnItemStartDrag(CUICellItem* itm);
    bool xr_stdcall OnItemDbClick(CUICellItem* itm);
    bool xr_stdcall OnItemSelected(CUICellItem* itm);
    bool xr_stdcall OnItemRButtonClick(CUICellItem* itm);
    bool xr_stdcall OnItemFocusReceive(CUICellItem* itm);
    bool xr_stdcall OnItemFocusLost(CUICellItem* itm);
    bool xr_stdcall OnItemFocusedUpdate(CUICellItem* itm);
    void xr_stdcall OnDragItemOnTrash(CUIDragItem* item, bool b_receive);
    bool OnItemDropped(PIItem itm, CUIDragDropListEx* new_owner, CUIDragDropListEx* old_owner);

    void ResetMode();
    void InitInventoryMode();
    void DeInitInventoryMode();
    void InitTradeMode();
    void DeInitTradeMode();
    void InitUpgradeMode();
    void DeInitUpgradeMode();
    void InitDeadBodySearchMode();
    void DeInitDeadBodySearchMode() const;

    void InitActorInfo();
    void InitPartnerInfo();

    void CurModeToScript();
    void RepairEffect_CurItem();

public:
    //Alundaio: Made public
    void SetCurrentItem(CUICellItem* itm);
    CUICellItem* CurrentItem();

protected:
    PIItem CurrentIItem();

    void InfoCurItem(CUICellItem* cell_item); // on update item
    CUIItemInfo* GetModeSpecificItemInfo();
    CUICharacterInfo* GetModeSpecificActorInfo() const;
    CUICharacterInfo* GetModeSpecificPartnerInfo(EMenuMode fallback) const;

    void ActivatePropertiesBox();
    void TryHidePropertiesBox();
    void xr_stdcall ProcessPropertiesBoxClicked(CUIWindow* w, void* d);

    void CheckDistance();
    void UpdateItemsPlace();

    void SetupUpgradeItem();
    void UpdateUpgradeItem();
    void TrySetCurUpgrade();
    void UpdateButtonsLayout();

    // inventory
    bool ToSlot(CUICellItem* itm, bool force_place, u16 slot_id);
    bool ToBag(CUICellItem* itm, bool b_use_cursor_pos);
    bool ToBelt(CUICellItem* itm, bool b_use_cursor_pos);
    bool TryUseItem(CUICellItem* cell_itm);
    bool ToQuickSlot(CUICellItem* itm);

    void UpdateActorMP();
    void UpdateOutfit();
    void MoveArtefactsToBag();
    bool TryActiveSlot(CUICellItem* itm);
    void xr_stdcall TryRepairItem(CUIWindow* w, void* d);
    bool CanUpgradeItem(PIItem item);

    bool ToActorTrade(CUICellItem* itm, bool b_use_cursor_pos);
    bool ToPartnerTrade(CUICellItem* itm, bool b_use_cursor_pos);
    bool ToPartnerTradeBag(CUICellItem* itm, bool b_use_cursor_pos);
    bool ToDeadBodyBag(CUICellItem* itm, bool b_use_cursor_pos);

    void AttachAddon(PIItem item_to_upgrade);
    void DetachAddon(LPCSTR addon_name, PIItem itm = NULL);

    void SendEvent_Item2Slot(PIItem pItem, u16 parent, u16 slot_id);
    void SendEvent_Item2Belt(PIItem pItem, u16 parent);
    void SendEvent_Item2Ruck(PIItem pItem, u16 parent);
    void SendEvent_Item_Drop(PIItem pItem, u16 parent);
    void SendEvent_Item_Eat(PIItem pItem, u16 parent);
    void SendEvent_ActivateSlot(u16 slot, u16 recipient);
    void DropAllCurrentItem();
    void OnPressUserKey();

    // trade
    void InitPartnerInventoryContents();
    void ColorizeItem(CUICellItem* itm, bool colorize);
    float CalcItemsWeight(CUIDragDropListEx* pList);
    u32 CalcItemsPrice(CUIDragDropListEx* pList, CTrade* pTrade, bool bBuying);
    void UpdatePrices();
    bool CanMoveToPartner(PIItem pItem);
    void TransferItems(CUIDragDropListEx* pSellList, CUIDragDropListEx* pBuyList, CTrade* pTrade, bool bBuying);

public:
    CUIActorMenu();
    virtual ~CUIActorMenu();

    virtual bool StopAnyMove();
    virtual void SendMessage(CUIWindow* pWnd, s16 msg, void* pData = NULL);
    virtual void Draw();
    virtual void Update();
    virtual void Show(bool status);

    virtual bool OnKeyboardAction(int dik, EUIMessages keyboard_action);
    virtual bool OnMouseAction(float x, float y, EUIMessages mouse_action);

    void ShowMessage(pcstr message, pcstr staticMessage = nullptr, float staticMsgTtl = -1.0f);

    void CallMessageBoxYesNo(LPCSTR text);
    void CallMessageBoxOK(LPCSTR text);
    void xr_stdcall OnMesBoxYes(CUIWindow*, void*);
    void xr_stdcall OnMesBoxNo(CUIWindow*, void*);

    void OnInventoryAction(PIItem pItem, u16 action_type);
    void ShowRepairButton(bool status);
    bool SetInfoCurUpgrade(Upgrade_type* upgrade_type, CInventoryItem* inv_item);
    void SeparateUpgradeItem();
    PIItem get_upgrade_item();
    bool DropAllItemsFromRuck(bool quest_force = false); // debug func

    void UpdateActor();
    void UpdatePartnerBag();
    void UpdateDeadBodyBag();

    void xr_stdcall OnBtnPerformTrade(CUIWindow* w, void* d);
    void xr_stdcall OnBtnPerformTradeBuy(CUIWindow* w, void* d);
    void xr_stdcall OnBtnPerformTradeSell(CUIWindow* w, void* d);
    void xr_stdcall OnBtnExitClicked(CUIWindow* w, void* d);
    void xr_stdcall TakeAllFromPartner(CUIWindow* w, void* d);
    void TakeAllFromInventoryBox();

    IC UIHint* get_hint_wnd() { return m_hint_wnd; }

    void RefreshCurrentItemCell();
    void DonateCurrentItem(CUICellItem* cell_item); //Alundaio: Donate item via context menu while in trade menu
}; // class CUIActorMenu
