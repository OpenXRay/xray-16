#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterSmartTerrainTaskGraphWalkAbstract CStateMonsterSmartTerrainTaskGraphWalk<_Object>

TEMPLATE_SPECIALIZATION
void CStateMonsterSmartTerrainTaskGraphWalkAbstract::initialize()
{
	inherited::initialize		();

	CSE_ALifeMonsterAbstract	*monster = smart_cast<CSE_ALifeMonsterAbstract*>(ai().alife().objects().object(object->ID()));
	VERIFY						(monster);
	VERIFY						(monster->m_smart_terrain_id != 0xffff);

	// get task
	m_task						= monster->brain().smart_terrain().task(monster);
	VERIFY						(m_task);
}


TEMPLATE_SPECIALIZATION
bool CStateMonsterSmartTerrainTaskGraphWalkAbstract::check_start_conditions()
{
	CSE_ALifeMonsterAbstract		*monster = smart_cast<CSE_ALifeMonsterAbstract*>(ai().alife().objects().object(object->ID()));
	VERIFY							(monster);

	if (monster->m_smart_terrain_id == 0xffff) return false;
	
	m_task							= monster->brain().smart_terrain().task(monster);	
	VERIFY3							(m_task, "Smart terrain selected, but task was not set for monster ", *object->cName());
	if (object->ai_location().game_vertex_id() == m_task->game_vertex_id()) return false;

	return							true;
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterSmartTerrainTaskGraphWalkAbstract::check_completion()
{
	// if we get to the graph point - work complete
	if (object->ai_location().game_vertex_id() == m_task->game_vertex_id()) return true;
	return false;
}


TEMPLATE_SPECIALIZATION
void CStateMonsterSmartTerrainTaskGraphWalkAbstract::execute()
{
	object->set_action					(ACT_WALK_FWD);
	object->set_state_sound				(MonsterSound::eMonsterSoundIdle);

	object->path().detour_graph_points	(m_task->game_vertex_id());
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterSmartTerrainTaskGraphWalkAbstract

