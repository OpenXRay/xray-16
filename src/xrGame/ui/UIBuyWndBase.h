#pragma once

#include "UIDialogWnd.h"

class CUIDragDropListEx;

enum ETradePreset
{
    _preset_idx_last = 0,
    _preset_idx_1,
    _preset_idx_2,
    _preset_idx_3,
    _preset_idx_origin,
    _preset_idx_temp,
    _preset_idx_default,
    _preset_count
};

struct _preset_item
{
    shared_str sect_name;
    u32 count;
    u8 addon_state;
    shared_str addon_names[3];
    bool operator==(const shared_str& what) { return (sect_name == what); }
};
using preset_items = xr_vector<_preset_item>;

class XR_NOVTABLE IBuyWnd : public CUIDialogWnd
{
public:
    IBuyWnd(pcstr window_name) : CUIDialogWnd(window_name) {}
    ~IBuyWnd() override = 0;

    virtual void Init(const shared_str& sectionName, const shared_str& sectionPrice) = 0;
    virtual void BindDragDropListEvents(CUIDragDropListEx* lst, bool bDrag) = 0;

    virtual void GetWeaponIndexByName(const shared_str& sectionName, u8& grpNum, u8& idx) = 0;
    virtual u32 GetMoneyAmount() const = 0;
    virtual void IgnoreMoney(bool ignore) = 0;
    virtual void SectionToSlot(const u8 grpNum, u8 uIndexInSlot, bool bRealRepresentationSet) = 0;
    virtual void SetMoneyAmount(u32 money) = 0;
    virtual bool CheckBuyAvailabilityInSlots() = 0;
    virtual void AddonToSlot(int add_on, int slot, bool bRealRepresentationSet) = 0;
    virtual const shared_str& GetWeaponNameByIndex(u8 grpNum, u8 idx) = 0;
    virtual void IgnoreMoneyAndRank(bool ignore) = 0;
    virtual bool CanBuyAllItems() = 0;
    virtual void ResetItems() = 0;
    virtual void SetRank(u32 rank) = 0;
    virtual u32 GetRank() = 0;
    virtual void ItemToBelt(const shared_str& sectionName) = 0;
    virtual void ItemToRuck(const shared_str& sectionName, u8 addons) = 0;
    virtual void ItemToSlot(const shared_str& sectionName, u8 addons) = 0;
    virtual void SetupPlayerItemsBegin() = 0;
    virtual void SetupPlayerItemsEnd() = 0;
    virtual void SetupDefaultItemsBegin() = 0;
    virtual void SetupDefaultItemsEnd() = 0;
    virtual const preset_items& GetPreset(ETradePreset idx) = 0;
    virtual u32 GetPresetCost(ETradePreset idx) = 0;
    virtual void ClearPreset(ETradePreset idx) = 0;
    virtual void TryUsePreset(ETradePreset idx) = 0;
    virtual bool IsIgnoreMoneyAndRank() = 0;

    pcstr GetDebugType() override { return "IBuyWnd"; }
};

inline IBuyWnd::~IBuyWnd() = default;
