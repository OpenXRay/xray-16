////////////////////////////////////////////////////////////////////////////
//	Module 		: memory_manager.cpp
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Memory manager
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "sound_memory_manager.h"
#include "hit_memory_manager.h"
#include "enemy_manager.h"
#include "item_manager.h"
#include "danger_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/stalker/ai_stalker_impl.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "memory_space_impl.h"
#include "ai_object_location.h"
#include "level_graph.h"
#include "profiler.h"
#include "agent_enemy_manager.h"
#include "script_game_object.h"

CMemoryManager::CMemoryManager		(CEntityAlive *entity_alive, CSound_UserDataVisitor *visitor)
{
	VERIFY				(entity_alive);
	m_object			= smart_cast<CCustomMonster*>(entity_alive);
	m_stalker			= smart_cast<CAI_Stalker*>(m_object);

	if (m_stalker)
		m_visual		= xr_new<CVisualMemoryManager>(m_stalker);
	else
		m_visual		= xr_new<CVisualMemoryManager>(m_object);

	m_sound				= xr_new<CSoundMemoryManager>	(m_object, m_stalker, visitor);
	m_hit				= xr_new<CHitMemoryManager>		(m_object, m_stalker);
	m_enemy				= xr_new<CEnemyManager>			(m_object);
	m_item				= xr_new<CItemManager>			(m_object);
	m_danger			= xr_new<CDangerManager>		(m_object);
}

CMemoryManager::~CMemoryManager		()
{
	xr_delete			(m_visual);
	xr_delete			(m_sound);
	xr_delete			(m_hit);
	xr_delete			(m_enemy);
	xr_delete			(m_item);
	xr_delete			(m_danger);
}

void CMemoryManager::Load			(LPCSTR section)
{
	sound().Load		(section);
	hit().Load			(section);
	enemy().Load		(section);
	item().Load			(section);
	danger().Load		(section);
}

void CMemoryManager::reinit			()
{
	visual().reinit		();
	sound().reinit		();
	hit().reinit		();
	enemy().reinit		();
	item().reinit		();
	danger().reinit		();
}

void CMemoryManager::reload			(LPCSTR section)
{
	visual().reload		(section);
	sound().reload		(section);
	hit().reload		(section);
	enemy().reload		(section);
	item().reload		(section);
	danger().reload		(section);
}

#ifdef _DEBUG
extern bool g_enemy_manager_second_update;
#endif // _DEBUG

void CMemoryManager::update_enemies	(const bool &registered_in_combat)
{
#ifdef _DEBUG
	g_enemy_manager_second_update	= false;
#endif // _DEBUG
	enemy().update		();

	if	(
			m_stalker && 
			(
				!enemy().selected() || 
				(
					smart_cast<const CAI_Stalker*>(enemy().selected()) && 
					smart_cast<const CAI_Stalker*>(enemy().selected())->wounded()
				)
			) &&
			registered_in_combat
		)
	{
		m_stalker->agent_manager().enemy().distribute_enemies	();

		if (visual().enabled())
			update		(visual().objects(),true);

		update			(sound().objects(),true);
		update			(hit().objects(),true);

#ifdef _DEBUG
		g_enemy_manager_second_update	= true;
#endif // _DEBUG
		enemy().update	();
	}
}

void CMemoryManager::update			(float time_delta)
{
	START_PROFILE("Memory Manager")

	visual().update		(time_delta);
	sound().update		();
	hit().update		();
	
	bool				registered_in_combat = false;
	if (m_stalker)
		registered_in_combat	= m_stalker->agent_manager().member().registered_in_combat(m_stalker);

	// update enemies and items
	enemy().reset		();
	item().reset		();

	if (visual().enabled())
		update			(visual().objects(),true);

	update				(sound().objects(),registered_in_combat ? true : false);
	update				(hit().objects(),registered_in_combat ? true : false);
	
	update_enemies		(registered_in_combat);
	item().update		();
	danger().update		();
	
	STOP_PROFILE
}

void CMemoryManager::enable			(const CObject *object, bool enable)
{
	visual().enable		(object,enable);
	sound().enable		(object,enable);
	hit().enable		(object,enable);
}

template <typename T>
void CMemoryManager::update			(const xr_vector<T> &objects, bool add_enemies)
{
	squad_mask_type					mask = m_stalker ? m_stalker->agent_manager().member().mask(m_stalker) : 0;
	xr_vector<T>::const_iterator	I = objects.begin();
	xr_vector<T>::const_iterator	E = objects.end();
	for ( ; I != E; ++I) {
		if (!(*I).m_enabled)
			continue;
		
		if (m_stalker && !(*I).m_squad_mask.test(mask))
			continue;

		danger().add				(*I);
		
		if (add_enemies) {
			const CEntityAlive		*entity_alive = smart_cast<const CEntityAlive*>((*I).m_object);
			if (entity_alive && enemy().add(entity_alive))
				continue;
		}

		const CAI_Stalker			*stalker = smart_cast<const CAI_Stalker*>((*I).m_object);
		if (m_stalker && stalker)
			continue;

		if ((*I).m_object)
			item().add				((*I).m_object);
	}
}

CMemoryInfo CMemoryManager::memory(const CObject *object) const
{
	CMemoryInfo						result;
	if (!this->object().g_Alive())
		return						(result);

	u32								level_time = 0;
	const CGameObject				*game_object = smart_cast<const CGameObject*>(object);
	VERIFY							(game_object);
	squad_mask_type					mask = m_stalker ? m_stalker->agent_manager().member().mask(m_stalker) : squad_mask_type(-1);

	{
		xr_vector<CVisibleObject>::const_iterator	I = std::find(visual().objects().begin(),visual().objects().end(),object_id(object));
		if (visual().objects().end() != I) {
			(CMemoryObject<CGameObject>&)result	= (CMemoryObject<CGameObject>&)(*I);
			result.visible						((*I).visible(mask));
			result.m_visual_info				= true;
			level_time							= (*I).m_level_time;
			VERIFY								(result.m_object);
		}
	}

	{
		xr_vector<CSoundObject>::const_iterator	I = std::find(sound().objects().begin(),sound().objects().end(),object_id(object));
		if ((sound().objects().end() != I) && (level_time < (*I).m_level_time)) {
			(CMemoryObject<CGameObject>&)result = (CMemoryObject<CGameObject>&)(*I);
			result.m_sound_info						= true;
			level_time								= (*I).m_level_time;
			VERIFY									(result.m_object);
		}
	}
	
	{
		xr_vector<CHitObject>::const_iterator	I = std::find(hit().objects().begin(),hit().objects().end(),object_id(object));
		if ((hit().objects().end() != I) && (level_time < (*I).m_level_time)) {
			(CMemoryObject<CGameObject>&)result = (CMemoryObject<CGameObject>&)(*I);
			result.m_object							= game_object;
			result.m_hit_info						= true;
			VERIFY									(result.m_object);
		}
	}

	return		(result);
}

u32 CMemoryManager::memory_time(const CObject *object) const
{
	u32					result = 0;
	if (!this->object().g_Alive())
		return			(0);

	const CGameObject	*game_object = smart_cast<const CGameObject*>(object);
	VERIFY				(game_object);

	{
		xr_vector<CVisibleObject>::const_iterator	I = std::find(visual().objects().begin(),visual().objects().end(),object_id(object));
		if (visual().objects().end() != I)
			result		= (*I).m_level_time;
	}

	{
		xr_vector<CSoundObject>::const_iterator	I = std::find(sound().objects().begin(),sound().objects().end(),object_id(object));
		if ((sound().objects().end() != I) && (result < (*I).m_level_time))
			result		= (*I).m_level_time;
	}
	
	{
		xr_vector<CHitObject>::const_iterator	I = std::find(hit().objects().begin(),hit().objects().end(),object_id(object));
		if ((hit().objects().end() != I) && (result < (*I).m_level_time))
			result		= (*I).m_level_time;
	}

	return				(result);
}

Fvector CMemoryManager::memory_position	(const CObject *object) const
{
	u32					time = 0;
	Fvector				result = Fvector().set(0.f,0.f,0.f);
	if (!this->object().g_Alive())
		return			(result);

	const CGameObject	*game_object = smart_cast<const CGameObject*>(object);
	VERIFY				(game_object);

	{
		xr_vector<CVisibleObject>::const_iterator	I = std::find(visual().objects().begin(),visual().objects().end(),object_id(object));
		if (visual().objects().end() != I) {
			time		= (*I).m_level_time;
			result		= (*I).m_object_params.m_position;
		}
	}

	{
		xr_vector<CSoundObject>::const_iterator	I = std::find(sound().objects().begin(),sound().objects().end(),object_id(object));
		if ((sound().objects().end() != I) && (time < (*I).m_level_time)) {
			time		= (*I).m_level_time;
			result		= (*I).m_object_params.m_position;
		}
	}
	
	{
		xr_vector<CHitObject>::const_iterator	I = std::find(hit().objects().begin(),hit().objects().end(),object_id(object));
		if ((hit().objects().end() != I) && (time < (*I).m_level_time)) {
			time		= (*I).m_level_time;
			result		= (*I).m_object_params.m_position;
		}
	}

	return				(result);
}

void CMemoryManager::remove_links	(CObject *object)
{
	if (m_object->g_Alive()) {
		visual().remove_links	(object);
		sound().remove_links	(object);
		hit().remove_links		(object);
	}

	danger().remove_links		(object);
	enemy().remove_links		(object);
	item().remove_links			(object);
}

void CMemoryManager::on_restrictions_change	()
{
	if (!m_object->g_Alive())
		return;

//	danger().on_restrictions_change	();
//	enemy().on_restrictions_change	();
	item().on_restrictions_change	();
}

void CMemoryManager::make_object_visible_somewhen	(const CEntityAlive *enemy)
{
	squad_mask_type				mask = stalker().agent_manager().member().mask(&stalker());
	MemorySpace::CVisibleObject	*obj = visual().visible_object(enemy);
//	if (obj) {
//		Msg						("------------------------------------------------------");
//		Msg						("[%6d] make_object_visible_somewhen [%s] = %x",Device.dwTimeGlobal,*enemy->cName(),obj->m_squad_mask.get());
//	}
//	LogStackTrace				("-------------make_object_visible_somewhen-------------");
	bool						prev = obj ? obj->visible(mask) : false;
	visual().add_visible_object	(enemy,.001f,true);
	MemorySpace::CVisibleObject	*obj1 = object().memory().visual().visible_object(enemy);
	VERIFY						(obj1);
//	if (obj1)
//		Msg						("[%6d] make_object_visible_somewhen [%s] = %x",Device.dwTimeGlobal,*enemy->cName(),obj1->m_squad_mask.get());
	obj1->visible				(mask,prev);
}

void CMemoryManager::save							(NET_Packet &packet) const
{
	visual().save				(packet);
	sound().save				(packet);
	hit().save					(packet);
	danger().save				(packet);
}

void CMemoryManager::load							(IReader &packet)
{
	visual().load				(packet);
	sound().load				(packet);
	hit().load					(packet);
	danger().load				(packet);
}

// we do this due to the limitation of client spawn manager
// should be revisited from the acrhitectural point of view
void CMemoryManager::on_requested_spawn				(CObject *object)
{
	visual().on_requested_spawn	(object);
	sound().on_requested_spawn	(object);
	hit().on_requested_spawn	(object);
}
