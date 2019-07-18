#pragma once
#include "inventory_item.h"

class CInventory;
class CInventoryItem;
class CHudItem;
class CInventoryOwner;

class CInventorySlot
{
public:
    CInventorySlot();
    virtual ~CInventorySlot();

    bool CanBeActivated() const;

    PIItem m_pIItem;
    bool m_bPersistent;
    bool m_bAct;
};

class priority_group
{
public:
    priority_group();
    void init_group(shared_str const& game_section, shared_str const& line);
    bool is_item_in_group(shared_str const& section_name) const;

private:
    xr_set<shared_str> m_sections;
}; // class priority_group

typedef xr_vector<CInventorySlot> TISlotArr;

class CInventory
{
public:
    CInventory();
    virtual ~CInventory();

    float TotalWeight() const;
    float CalcTotalWeight();

    void Take(CGameObject* pObj, bool bNotActivate, bool strict_placement);
    // if just_before_destroy is true, then activate will be forced (because deactivate message will not deliver)
    bool DropItem(CGameObject* pObj, bool just_before_destroy, bool dont_create_shell);
    void Clear();

    IC u16 FirstSlot() const { return KNIFE_SLOT; }
    IC u16 LastSlot() const { return m_iLastSlot; } // not "end"
    IC bool SlotIsPersistent(u16 slot_id) const { return m_slots[slot_id].m_bPersistent; }
    bool Slot(u16 slot_id, PIItem pIItem, bool bNotActivate = false, bool strict_placement = false);
    bool Belt(PIItem pIItem, bool strict_placement = false);
    bool Ruck(PIItem pIItem, bool strict_placement = false);

    bool InSlot(const CInventoryItem* pIItem) const;
    bool InBelt(const CInventoryItem* pIItem) const;
    bool InRuck(const CInventoryItem* pIItem) const;

    bool CanPutInSlot(PIItem pIItem, u16 slot_id) const;
    bool CanPutInBelt(PIItem pIItem);
    bool CanPutInRuck(PIItem pIItem) const;

    bool CanTakeItem(CInventoryItem* inventory_item) const;

    void Activate(u16 slot, /*EActivationReason reason=eGeneral, */ bool bForce = false);

    static u32 const qs_priorities_count = 5;
    PIItem GetNextItemInActiveSlot(u8 const priority_value, bool ignore_ammo);
    bool ActivateNextItemInActiveSlot();
    priority_group& GetPriorityGroup(u8 const priority_value, u16 slot);
    void InitPriorityGroupsForQSwitch();

    PIItem ActiveItem() const { return (m_iActiveSlot == NO_ACTIVE_SLOT) ? NULL : ItemFromSlot(m_iActiveSlot); }
    PIItem ItemFromSlot(u16 slot) const;

    bool Action(u16 cmd, u32 flags);
    void ActiveWeapon(u16 slot);
    void Update();
    // Ищет на поясе аналогичный IItem
    PIItem Same(const PIItem pIItem, bool bSearchRuck) const;
    // Ищет на поясе IItem для указанного слота
    PIItem SameSlot(const u16 slot, PIItem pIItem, bool bSearchRuck) const;
    // Ищет на поясе или в рюкзаке IItem с указанным именем (cName())
    PIItem Get(LPCSTR name, bool bSearchRuck) const;
    // Ищет на поясе или в рюкзаке IItem с указанным именем (id)
    PIItem Get(const u16 id, bool bSearchRuck) const;
    // Ищет на поясе или в рюкзаке IItem с указанным CLS_ID
    PIItem Get(CLASS_ID cls_id, bool bSearchRuck) const;
    PIItem GetAny(LPCSTR name) const; // search both (ruck and belt)
    PIItem item(CLASS_ID cls_id) const;

    // get all the items with the same section name
    virtual u32 dwfGetSameItemCount(LPCSTR caSection, bool SearchAll = false);
    virtual u32 dwfGetGrenadeCount(LPCSTR caSection, bool SearchAll);
    // get all the items with the same object id
    virtual bool bfCheckForObject(ALife::_OBJECT_ID tObjectID);
    PIItem get_object_by_id(ALife::_OBJECT_ID tObjectID);

    u32 dwfGetObjectCount();
    PIItem tpfGetObjectByIndex(int iIndex);
    PIItem GetItemFromInventory(LPCSTR caItemName);

    bool Eat(PIItem pIItem);
    bool ClientEat(PIItem pIItem);

    IC u16 GetActiveSlot() const { return m_iActiveSlot; }
    void SetPrevActiveSlot(u16 ActiveSlot) { m_iPrevActiveSlot = ActiveSlot; }
    u16 GetPrevActiveSlot() const { return m_iPrevActiveSlot; }
    IC u16 GetNextActiveSlot() const { return m_iNextActiveSlot; }
    void SetActiveSlot(u16 ActiveSlot) { m_iActiveSlot = m_iNextActiveSlot = ActiveSlot; }
    bool IsSlotsUseful() const { return m_bSlotsUseful; }
    void SetSlotsUseful(bool slots_useful) { m_bSlotsUseful = slots_useful; }
    bool IsBeltUseful() const { return m_bBeltUseful; }
    void SetBeltUseful(bool belt_useful) { m_bBeltUseful = belt_useful; }
    void SetSlotsBlocked(u16 mask, bool bBlock);

    void BlockSlot(u16 slot_id);
    void UnblockSlot(u16 slot_id);
    bool IsSlotBlocked(PIItem const iitem) const;

    TIItemContainer m_all;
    TIItemContainer m_ruck, m_belt;
    TIItemContainer m_activ_last_items;

protected:
    TISlotArr m_slots;

public:
    //возвращает все кроме PDA в слоте и болта
    void AddAvailableItems(TIItemContainer& items_container, bool for_trade) const;

    float GetMaxWeight() const { return m_fMaxWeight; }
    void SetMaxWeight(float weight) { m_fMaxWeight = weight; }
    u32 BeltWidth() const;

    inline CInventoryOwner* GetOwner() const { return m_pOwner; }
    friend class CInventoryOwner;

    u32 ModifyFrame() const { return m_dwModifyFrame; }
    void InvalidateState() noexcept;
    void Items_SetCurrentEntityHud(bool current_entity);
    bool isBeautifulForActiveSlot(CInventoryItem* pIItem);

protected:
    void UpdateDropTasks();
    void UpdateDropItem(PIItem pIItem);

    // Активный слот и слот который станет активным после смены
    //значения совпадают в обычном состоянии (нет смены слотов)
    u16 m_iActiveSlot;
    u16 m_iNextActiveSlot;
    u16 m_iPrevActiveSlot;
    u16 m_iLastSlot;

    CInventoryOwner* m_pOwner;

    //флаг, показывающий наличие пояса в инвенторе
    bool m_bBeltUseful;
    //флаг, допускающий использование слотов
    bool m_bSlotsUseful;

    // максимальный вес инвентаря
    float m_fMaxWeight;
    // текущий вес в инвентаре
    float m_fTotalWeight;

    //кадр на котором произошло последнее изменение в инвенторе
    u32 m_dwModifyFrame;

    bool m_drop_last_frame;

    void SendActionEvent(u16 cmd, u32 flags);

private:
    priority_group* m_slot2_priorities[qs_priorities_count];
    priority_group* m_slot3_priorities[qs_priorities_count];

    priority_group m_groups[qs_priorities_count];
    priority_group m_null_priority;
    typedef xr_set<PIItem> except_next_items_t;
    except_next_items_t m_next_items_exceptions;
    u32 m_next_item_iteration_time;

    u8 m_blocked_slots[LAST_SLOT + 1];
    bool IsSlotBlocked(u16 slot_id) const;
    void TryActivatePrevSlot();
    void TryDeactivateActiveSlot();
};
