////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_online_offline_group_brain.cpp
//	Created 	: 25.10.2005
//  Modified 	: 25.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife Online Offline Group brain class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_online_offline_group_brain.h"
#include "Common/object_broker.h"
#include "xrServer_Objects_ALife_Monsters.h"

#ifdef XRGAME_EXPORTS
#include "alife_monster_movement_manager.h"
#include "alife_monster_detail_path_manager.h"
#include "alife_monster_patrol_path_manager.h"
#include "ai_space.h"
#include "ef_storage.h"
#include "ef_primary.h"
#include "alife_simulator.h"
#include "alife_graph_registry.h"
#include "movement_manager_space.h"
#include "alife_smart_terrain_registry.h"
#include "alife_object_registry.h"
#include "alife_time_manager.h"
#include "date_time.h"
#ifdef DEBUG
#include "Level.h"
#include "map_location.h"
#include "map_manager.h"
#endif
#endif

#define MAX_ITEM_FOOD_COUNT 3
#define MAX_ITEM_MEDIKIT_COUNT 3
#define MAX_AMMO_ATTACH_COUNT 1

CALifeOnlineOfflineGroupBrain::CALifeOnlineOfflineGroupBrain(object_type* object)
{
    VERIFY(object);
    m_object = object;

#ifdef XRGAME_EXPORTS
    m_movement_manager = new CALifeMonsterMovementManager(object);
#endif
}

CALifeOnlineOfflineGroupBrain::~CALifeOnlineOfflineGroupBrain()
{
#ifdef XRGAME_EXPORTS
    xr_delete(m_movement_manager);
#endif
}

void CALifeOnlineOfflineGroupBrain::on_state_write(NET_Packet& packet) {}
void CALifeOnlineOfflineGroupBrain::on_state_read(NET_Packet& packet) {}
#ifdef XRGAME_EXPORTS

void CALifeOnlineOfflineGroupBrain::on_register() {}
void CALifeOnlineOfflineGroupBrain::on_unregister() {}
void CALifeOnlineOfflineGroupBrain::on_location_change() {}
void CALifeOnlineOfflineGroupBrain::update()
{
    CALifeSmartTerrainTask* const task = object().get_current_task();
    THROW2(task, "CALifeOnlineOfflineGroupBrain returned nil task, while npc is registered in it");
    movement().path_type(MovementManager::ePathTypeGamePath);
    movement().detail().target(*task);
    movement().update();
}

void CALifeOnlineOfflineGroupBrain::on_switch_online() { movement().on_switch_online(); }
void CALifeOnlineOfflineGroupBrain::on_switch_offline() { movement().on_switch_offline(); }
#endif // XRGAME_EXPORTS
