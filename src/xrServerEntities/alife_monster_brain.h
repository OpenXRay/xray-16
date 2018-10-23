////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_brain.h
//	Created 	: 06.10.2005
//  Modified 	: 22.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster brain class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/game_graph_space.h"
#include "xrServer_Space.h"
#include "alife_space.h"

class CSE_ALifeMonsterAbstract;
class CALifeMonsterMovementManager;
class CSE_ALifeSmartZone;
class NET_Packet;

class CALifeMonsterBrain
{
public:
    typedef CSE_ALifeMonsterAbstract object_type;
    typedef CALifeMonsterMovementManager movement_manager_type;

private:
    object_type* m_object;
    movement_manager_type* m_movement_manager;
    bool m_can_choose_alife_tasks;

public:
    CSE_ALifeSmartZone* m_smart_terrain;
    ALife::_TIME_ID m_last_search_time;
    ALife::_TIME_ID m_time_interval;

    // sad, but true
public:
    void select_task(const bool forced = false);

private:
    void process_task();
    void default_behaviour();
    IC bool can_choose_alife_tasks() const;

public:
    CALifeMonsterBrain(object_type* object);
    virtual ~CALifeMonsterBrain();

public:
    void on_state_write(NET_Packet& packet);
    void on_state_read(NET_Packet& packet);
    void on_register();
    void on_unregister();
    void on_location_change();
    void on_switch_online();
    void on_switch_offline();

public:
    void update(const bool forced = false);
    void update_script() { this->update(true); }
    bool perform_attack();
    ALife::EMeetActionType action_type(
        CSE_ALifeSchedulable* tpALifeSchedulable, const int& iGroupIndex, const bool& bMutualDetection);

public:
    IC object_type& object() const;
    IC movement_manager_type& movement() const;
    CSE_ALifeSmartZone& smart_terrain();
    IC void can_choose_alife_tasks(bool value);
};

#include "alife_monster_brain_inline.h"
