////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_brain.cpp
//	Created 	: 06.10.2005
//  Modified 	: 06.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human brain class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_human_brain.h"
#include "Common/object_broker.h"
#include "xrServer_Objects_ALife_Monsters.h"

#ifdef XRGAME_EXPORTS
#include "alife_human_object_handler.h"
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

CALifeHumanBrain::CALifeHumanBrain(object_type* object) : inherited(object)
{
    VERIFY(object);
    m_object = object;

#ifdef XRGAME_EXPORTS
    m_object_handler = new CALifeHumanObjectHandler(object);
#endif

    m_dwTotalMoney = 0;
    m_cpEquipmentPreferences.resize(5);
    m_cpMainWeaponPreferences.resize(4);

#ifdef XRGAME_EXPORTS
    m_cpEquipmentPreferences.resize(iFloor(ai().ef_storage().m_pfEquipmentType->ffGetMaxResultValue() + .5f));
    m_cpMainWeaponPreferences.resize(iFloor(ai().ef_storage().m_pfMainWeaponType->ffGetMaxResultValue() + .5f));
    R_ASSERT2((iFloor(ai().ef_storage().m_pfEquipmentType->ffGetMaxResultValue() + .5f) == 5) &&
            (iFloor(ai().ef_storage().m_pfMainWeaponType->ffGetMaxResultValue() + .5f) == 4),
        "Recompile Level Editor and xrAI and rebuild file \"game.spawn\"!");
#endif

    for (int i = 0, n = m_cpEquipmentPreferences.size(); i < n; ++i)
        m_cpEquipmentPreferences[i] = u8(::Random.randI(3));

    for (int i = 0, n = m_cpMainWeaponPreferences.size(); i < n; ++i)
        m_cpMainWeaponPreferences[i] = u8(::Random.randI(3));
}

CALifeHumanBrain::~CALifeHumanBrain()
{
#ifdef XRGAME_EXPORTS
    xr_delete(m_object_handler);
#endif
}

void CALifeHumanBrain::on_state_write(NET_Packet& packet)
{
    if (packet.inistream == nullptr)
    {
        save_data(m_cpEquipmentPreferences, packet);
        save_data(m_cpMainWeaponPreferences, packet);
    }
}

void CALifeHumanBrain::on_state_read(NET_Packet& packet)
{
    if (object().m_wVersion <= 19)
        return;

    if (object().m_wVersion < 110)
    {
        {
            DWORD_VECTOR temp;
            load_data(temp, packet);
        }
        {
            xr_vector<bool> temp;
            load_data(temp, packet);
        }
    }

    if (object().m_wVersion <= 35)
        return;

    if (object().m_wVersion < 110)
    {
        shared_str temp;
        packet.r_stringZ(temp);
    }

    if (object().m_wVersion < 118)
    {
        ALife::OBJECT_VECTOR temp;
        load_data(temp, packet);
    }

    if (packet.inistream == nullptr)
    {
        load_data(m_cpEquipmentPreferences, packet);
        load_data(m_cpMainWeaponPreferences, packet);
    }
}
