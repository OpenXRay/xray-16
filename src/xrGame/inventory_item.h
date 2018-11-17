////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item.h
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Victor Reutsky, Yuri Dobronravin
//	Description : Inventory item
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "inventory_space.h"
#include "hit_immunity.h"
#include "attachable_item.h"
#include "xrServer_Objects_ALife.h"
#include "xrServer_Objects_ALife_Items.h"
#include "xrCommon/xr_deque.h"
#ifdef DEBUG
#include "xrEngine/pure.h"
#endif

enum EHandDependence
{
    hdNone = 0,
    hd1Hand = 1,
    hd2Hand = 2
};

class CSE_Abstract;
class CGameObject;
class CFoodItem;
class CMissile;
class CHudItem;
class CWeaponAmmo;
class CWeapon;
class CPhysicsShellHolder;
class NET_Packet;
class CEatableItem;
struct SPHNetState;
struct net_update_IItem;

class CInventoryOwner;

struct SHit;

class CSE_ALifeInventoryItem;
typedef typename CSE_ALifeInventoryItem::mask_num_items mask_inv_num_items;

struct net_update_IItem
{
    u32 dwTimeStamp;
    SPHNetState State;
};

struct net_updateInvData
{
    xr_deque<net_update_IItem> NET_IItem;
    u32 m_dwIStartTime;
    u32 m_dwIEndTime;
};

class CInventoryItem : public CAttachableItem,
                       public CHitImmunity
#ifdef DEBUG
                       ,
                       public pureRender
#endif
{
private:
    typedef CAttachableItem inherited;

protected:
    enum EIIFlags
    {
        FdropManual = (1 << 0),
        FCanTake = (1 << 1),
        FCanTrade = (1 << 2),
        Fbelt = (1 << 3),
        Fruck = (1 << 4),
        FRuckDefault = (1 << 5),
        FUsingCondition = (1 << 6),
        FAllowSprint = (1 << 7),
        Fuseful_for_NPC = (1 << 8),
        FInInterpolation = (1 << 9),
        FInInterpolate = (1 << 10),
        FIsQuestItem = (1 << 11),
        FIsHelperItem = (1 << 12),
    };

    Flags16 m_flags;
    BOOL m_can_trade;

public:
    CInventoryItem();
    virtual ~CInventoryItem();

public:
    virtual void Load(LPCSTR section);
    void ReloadNames();

    LPCSTR NameItem(); // remove <virtual> by sea
    LPCSTR NameShort();
    shared_str ItemDescription() { return m_Description; }
    virtual bool GetBriefInfo(II_BriefInfo& info)
    {
        info.clear();
        return false;
    }

    virtual void OnEvent(NET_Packet& P, u16 type);

    virtual bool Useful() const; // !!! Переопределить. (см. в Inventory.cpp)
    virtual bool IsUsingCondition() const { return m_flags.test(FUsingCondition); }
    virtual bool Attach(PIItem pIItem, bool b_send_event) { return false; }
    virtual bool Detach(PIItem pIItem) { return false; }
    //при детаче спаунится новая вещь при заданно названии секции
    virtual bool Detach(const char* item_section_name, bool b_spawn_item);
    virtual bool CanAttach(PIItem pIItem) { return false; }
    virtual bool CanDetach(LPCSTR item_section_name) { return false; }
    virtual EHandDependence HandDependence() const { return hd1Hand; };
    virtual bool IsSingleHanded() const { return true; };
    virtual bool ActivateItem(); // !!! Переопределить. (см. в Inventory.cpp)
    virtual void DeactivateItem(); // !!! Переопределить. (см. в Inventory.cpp)
    virtual bool Action(u16 cmd, u32 flags) { return false; } // true если известная команда, иначе false
    virtual void DiscardState(){};

    virtual void OnH_B_Chield();
    virtual void OnH_A_Chield();
    virtual void OnH_B_Independent(bool just_before_destroy);
    virtual void OnH_A_Independent();

    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);
    virtual BOOL net_SaveRelevant() { return TRUE; }
    virtual void render_item_ui(){}; // when in slot & query return TRUE
    virtual bool render_item_ui_query() { return false; }; // when in slot
    virtual void UpdateCL();

    virtual void Hit(SHit* pHDS);

    BOOL GetDropManual() const { return m_flags.test(FdropManual); }
    void SetDropManual(BOOL val);

    BOOL IsInvalid() const;

    BOOL IsQuestItem() const { return m_flags.test(FIsQuestItem); }
    virtual u32 Cost() const { return m_cost; }
    //			u32					Cost				()	const	{ return m_cost; }
    virtual float Weight() const { return m_weight; }
    void SetWeight(float w) { m_weight = w; }

public:
    CInventory* m_pInventory;
    shared_str m_section_id;
    shared_str m_name;
    shared_str m_nameShort;
    shared_str m_nameComplex;

    SInvItemPlace m_ItemCurrPlace;

    virtual void OnMoveToSlot(const SInvItemPlace& prev){};
    virtual void OnMoveToBelt(const SInvItemPlace& prev){};
    virtual void OnMoveToRuck(const SInvItemPlace& prev){};

    Irect GetInvGridRect() const;
    Irect GetUpgrIconRect() const;
    const shared_str& GetIconName() const { return m_icon_name; };
    Frect GetKillMsgRect() const;
    //---------------------------------------------------------------------
    IC float GetCondition() const { return m_fCondition; }
    virtual float GetConditionToShow() const { return GetCondition(); }
    IC void SetCondition(float val) { m_fCondition = val; }
    void ChangeCondition(float fDeltaCondition);

    u16 BaseSlot() const { return m_ItemCurrPlace.base_slot_id; }
    u16 CurrSlot() const { return m_ItemCurrPlace.slot_id; }
    u16 CurrPlace() const { return m_ItemCurrPlace.type; }
    bool Belt() { return !!m_flags.test(Fbelt); }
    void Belt(bool on_belt) { m_flags.set(Fbelt, on_belt); }
    bool Ruck() { return !!m_flags.test(Fruck); }
    void Ruck(bool on_ruck) { m_flags.set(Fruck, on_ruck); }
    bool RuckDefault() { return !!m_flags.test(FRuckDefault); }
    virtual bool CanTake() const { return !!m_flags.test(FCanTake); }
    bool CanTrade() const;
    void AllowTrade() { m_flags.set(FCanTrade, m_can_trade); };
    void DenyTrade() { m_flags.set(FCanTrade, FALSE); };
    virtual bool IsNecessaryItem(CInventoryItem* item);
    virtual bool IsNecessaryItem(const shared_str& item_sect) { return false; };
protected:
    u32 m_cost;
    float m_weight;
    float m_fCondition;
    shared_str m_Description;

protected:
    ALife::_TIME_ID m_dwItemIndependencyTime;

    float m_fControlInertionFactor;
    shared_str m_icon_name;

public:
    virtual void make_Interpolation(){};
    virtual void PH_B_CrPr(); // actions & operations before physic correction-prediction steps
    virtual void PH_I_CrPr(); // actions & operations after correction before prediction steps
#ifdef DEBUG
    virtual void PH_Ch_CrPr(); //
#endif
    virtual void PH_A_CrPr(); // actions & operations after phisic correction-prediction steps

    virtual void net_Import(NET_Packet& P); // import from server
    virtual void net_Export(NET_Packet& P); // export to server

public:
    virtual void activate_physic_shell();
    virtual bool has_network_synchronization() const;

    virtual bool NeedToDestroyObject() const;
    virtual ALife::_TIME_ID TimePassedAfterIndependant() const;

    virtual bool IsSprintAllowed() const { return !!m_flags.test(FAllowSprint); };
    virtual float GetControlInertionFactor() const { return m_fControlInertionFactor; };
    virtual void UpdateXForm();

protected:
    net_updateInvData* m_net_updateData;
    net_updateInvData* NetSync();
    void CalculateInterpolationParams();

public:
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void reload(LPCSTR section);
    virtual void reinit();
    virtual bool can_kill() const;
    virtual CInventoryItem* can_kill(CInventory* inventory) const;
    virtual const CInventoryItem* can_kill(const xr_vector<const CGameObject*>& items) const;
    virtual CInventoryItem* can_make_killing(const CInventory* inventory) const;
    virtual bool ready_to_kill() const;
    IC bool useful_for_NPC() const;
#ifdef DEBUG
    virtual void OnRender();
#endif

public:
    virtual IFactoryObject* _construct();
    IC CPhysicsShellHolder& object() const
    {
        VERIFY(m_object);
        return (*m_object);
    }
    u16 object_id() const;
    u16 parent_id() const;
    virtual void on_activate_physic_shell() { R_ASSERT2(0, "failed call of virtual function!"); }
protected:
    float m_holder_range_modifier;
    float m_holder_fov_modifier;

public:
    virtual void modify_holder_params(float& range, float& fov) const;

protected:
    IC CInventoryOwner& inventory_owner() const;

private:
    CPhysicsShellHolder* m_object;

public:
    virtual CInventoryItem* cast_inventory_item() { return this; }
    virtual CAttachableItem* cast_attachable_item() { return this; }
    virtual CPhysicsShellHolder* cast_physics_shell_holder() { return 0; }
    virtual CEatableItem* cast_eatable_item() { return 0; }
    virtual CWeapon* cast_weapon() { return 0; }
    virtual CFoodItem* cast_food_item() { return 0; }
    virtual CMissile* cast_missile() { return 0; }
    virtual CHudItem* cast_hud_item() { return 0; }
    virtual CWeaponAmmo* cast_weapon_ammo() { return 0; }
    virtual CGameObject* cast_game_object() { return 0; }
    ////////// upgrades //////////////////////////////////////////////////
public:
    typedef xr_vector<shared_str> Upgrades_type;

protected:
    Upgrades_type m_upgrades;

public:
    IC bool has_any_upgrades() { return (m_upgrades.size() != 0); }
    bool has_upgrade(const shared_str& upgrade_id);
    bool has_upgrade_group(const shared_str& upgrade_group_id);
    void add_upgrade(const shared_str& upgrade_id, bool loading);
    bool get_upgrades_str(string2048& res) const;
#ifdef GAME_OBJECT_EXTENDED_EXPORTS
    Upgrades_type get_upgrades() { return m_upgrades; } //Alundaio
#endif

    bool equal_upgrades(Upgrades_type const& other_upgrades) const;

    bool verify_upgrade(LPCSTR section);
    bool install_upgrade(LPCSTR section);
    void pre_install_upgrade();

#ifdef DEBUG
    void log_upgrades();
#endif // DEBUG

    IC Upgrades_type const& upgardes() const;
    virtual void Interpolate();
    float interpolate_states(net_update_IItem const& first, net_update_IItem const& last, SPHNetState& current);

protected:
    virtual void net_Spawn_install_upgrades(Upgrades_type saved_upgrades);
    virtual bool install_upgrade_impl(LPCSTR section, bool test);

    template <typename T>
    IC static bool process_if_exists(
        pcstr section, pcstr name, T (CInifile::*method)(pcstr, pcstr) const, T& value, bool test);

    template <typename T>
    IC static bool process_if_exists_set(
        pcstr section, pcstr name, T (CInifile::*method)(pcstr, pcstr) const, T& value, bool test);

    void net_Export_PH_Params(NET_Packet& P, SPHNetState& State, mask_inv_num_items& num_items);
    void net_Import_PH_Params(NET_Packet& P, net_update_IItem& N, mask_inv_num_items& num_items);

    bool m_just_after_spawn;
    bool m_activated;

public:
    IC bool is_helper_item() { return !!m_flags.test(FIsHelperItem); }
    IC void set_is_helper(bool is_helper) { m_flags.set(FIsHelperItem, is_helper); }
}; // class CInventoryItem

#include "inventory_item_inline.h"
