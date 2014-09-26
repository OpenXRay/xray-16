////////////////////////////////////////////////////////////////////////////
//	Module 		: game_spawn_constructor.h
//	Created 	: 16.10.2004
//  Modified 	: 16.10.2004
//	Author		: Dmitriy Iassenev
//	Description : Game spawn constructor
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"
#include "xr_graph_merge.h"
#include "xrthread.h"
#include "graph_abstract.h"
#include "xrServer_Object_Base.h"
#include "spawn_constructor_space.h"
#include "server_entity_wrapper.h"
#include "guid_generator.h"

class CSE_Abstract;
class CLevelSpawnConstructor;
class CSE_ALifeCreatureAbstract;
class CPatrolPathStorage;

class CGameSpawnConstructor {
	friend class CSpawnMerger;
public:
	typedef SpawnConstructorSpace::LEVEL_POINT_STORAGE								LEVEL_POINT_STORAGE;
	typedef SpawnConstructorSpace::LEVEL_CHANGER_STORAGE							LEVEL_CHANGER_STORAGE;
	typedef CGraphAbstractSerialize<CServerEntityWrapper*,float,ALife::_SPAWN_ID>	SPAWN_GRAPH;
	typedef xr_vector<CLevelSpawnConstructor*>										LEVEL_SPAWN_STORAGE;
	typedef xr_set<CLevelInfo>														LEVEL_INFO_STORAGE;

public:
	struct CSpawnHeader {
		u32							m_version;
		xrGUID						m_guid;
		xrGUID						m_graph_guid;
		u32							m_spawn_count;
		u32							m_level_count;
	};

private:
	xrCriticalSection				m_critical_section;
	ALife::_SPAWN_ID				m_spawn_id;
	CThreadManager					m_thread_manager;
	CSpawnHeader					m_spawn_header;
	ALife::STORY_P_MAP				m_story_objects;
	LEVEL_INFO_STORAGE				m_levels;
	LEVEL_SPAWN_STORAGE				m_level_spawns;
	LEVEL_CHANGER_STORAGE			m_level_changers;
	LEVEL_POINT_STORAGE				m_level_points;
	bool							m_no_separator_check;

private:
	xr_vector<ALife::_SPAWN_ID>		m_spawn_roots;
	xr_vector<ALife::_SPAWN_ID>		m_temp0;
	xr_vector<ALife::_SPAWN_ID>		m_temp1;

private:
	CGameGraph						*m_game_graph;
	SPAWN_GRAPH						*m_spawn_graph;
	CPatrolPathStorage				*m_patrol_path_storage;
	CInifile						*m_game_info;
	CSE_ALifeCreatureAbstract		*m_actor;

private:
	string_path						m_game_graph_id;

private:
	IC		shared_str				actor_level_name		();
	IC		shared_str				spawn_name				(LPCSTR output);
			void					save_spawn				(LPCSTR name, LPCSTR output);
			void					verify_level_changers	();
			void					verify_spawns			(ALife::_SPAWN_ID spawn_id);
			void					verify_spawns			();
			void					process_spawns			();
			void					load_spawns				(LPCSTR name, bool no_separator_check);
	IC		SPAWN_GRAPH				&spawn_graph			();
	IC		ALife::_SPAWN_ID		spawn_id				();
	IC		void					process_spawns			(xr_vector<ALife::_SPAWN_ID> &spawns);
			void					process_actor			(LPCSTR start_level_name);

public:
									CGameSpawnConstructor	(LPCSTR name, LPCSTR output, LPCSTR start, bool no_separator_check);
	virtual							~CGameSpawnConstructor	();
			void					add_story_object		(ALife::_STORY_ID id,CSE_ALifeDynamicObject *object, LPCSTR level_name);
			void					add_object				(CSE_Abstract *object);
			void					remove_object			(CSE_Abstract *object);
	IC		void					add_level_changer		(CSE_ALifeLevelChanger *level_changer);
	IC		void					add_level_points		(const LEVEL_POINT_STORAGE &level_points);
	IC		u32						level_id				(LPCSTR level_name);
	IC		CGameGraph				&game_graph				() const;
	IC		CInifile				&game_info				();
	IC		void					add_edge				(ALife::_SPAWN_ID id0, ALife::_SPAWN_ID id1, float weight);
	IC		u32						level_point_count		() const;
	IC		LEVEL_CHANGER_STORAGE	&level_changers			();
	IC		CPatrolPathStorage		&patrol_path_storage	() const;
};

#include "game_spawn_constructor_inline.h"