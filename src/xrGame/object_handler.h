////////////////////////////////////////////////////////////////////////////
//	Module 		: object_handler.h
//	Created 	: 11.03.2004
//  Modified 	: 11.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Object handler
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "InventoryOwner.h"
#include "xrAICore/Navigation/graph_engine_space.h"

namespace MonsterSpace
{
enum EObjectAction : u32;
}

class CAI_Stalker;
class CWeapon;
class CMissile;
class CFoodItem;
class CObjectHandlerPlanner;

class CObjectHandler : public CInventoryOwner
{
protected:
    typedef CInventoryOwner inherited;
    typedef GraphEngineSpace::_solver_value_type _value_type;
    typedef GraphEngineSpace::_solver_condition_type _condition_type;

private:
    int m_r_hand;
    int m_l_finger1;
    int m_r_finger2;

private:
    mutable int m_strap_bone0;
    mutable int m_strap_bone1;
    mutable ALife::_OBJECT_ID m_strap_object_id;

protected:
    bool m_hammer_is_clutched;
    bool m_infinite_ammo;
    CObjectHandlerPlanner* m_planner;
    mutable bool m_inventory_actual;

public:
    bool m_clutched_hammer_enabled;

private:
    void actualize_strap_mode(CWeapon* weapon) const;

protected:
    IC void switch_torch(CInventoryItem* inventory_item, bool value);

public:
    CObjectHandler();
    virtual ~CObjectHandler();
    virtual void Load(LPCSTR section);
    virtual void reinit(CAI_Stalker* object);
    virtual void reload(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void update();
    virtual void OnItemTake(CInventoryItem* inventory_item);
    virtual void OnItemDrop(CInventoryItem* inventory_item, bool just_before_destroy);
    virtual void attach(CInventoryItem* inventory_item);
    virtual void detach(CInventoryItem* inventory_item);
    CInventoryItem* best_weapon() const;
    void set_goal(MonsterSpace::EObjectAction object_action, CGameObject* game_object = 0, u32 min_queue_size = -1,
        u32 max_queue_size = -1, u32 min_queue_interval = 300, u32 max_queue_interval = 300);
    void set_goal(MonsterSpace::EObjectAction object_action, CInventoryItem* inventory_item, u32 min_queue_size = -1,
        u32 max_queue_size = -1, u32 min_queue_interval = 300, u32 max_queue_interval = 300);
    bool goal_reached();
    IC bool hammer_is_clutched() const;
    IC bool const& infinite_ammo() const;
    IC CObjectHandlerPlanner& planner() const;
    void weapon_bones(int& b0, int& b1, int& b2) const;
    bool weapon_strapped() const;
    bool weapon_strapped(CWeapon* weapon) const;
    bool weapon_unstrapped() const;
    bool weapon_unstrapped(CWeapon* weapon) const;
    bool is_weapon_going_to_be_strapped(CGameObject const* object) const;

private:
    void set_inertia(const CWeapon& weapon, const u32& action_id, const u32& aim_time) const;

public:
    virtual bool can_use_dynamic_lights();
    void aim_time(const CWeapon& weapon, const u32& aim_time) const;
    u32 aim_time(const CWeapon& weapon) const;
};

#include "object_handler_inline.h"
