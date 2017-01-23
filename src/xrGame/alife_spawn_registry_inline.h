////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_spawn_registry_inline.h
//	Created 	: 15.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife spawn registry inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	const CALifeSpawnHeader &CALifeSpawnRegistry::header	() const
{
	return							(m_header);
}

IC	void CALifeSpawnRegistry::assign_artefact_position		(CSE_ALifeAnomalousZone	*anomaly, CSE_ALifeDynamicObject *object) const
{
	object->m_tGraphID				= anomaly->m_tGraphID;
	VERIFY3							(anomaly->m_artefact_spawn_count,"Anomaly is outside of the AI-map but is used for artefact generation : ",anomaly->name_replace());
	u32								index = anomaly->m_artefact_position_offset + anomaly->randI(anomaly->m_artefact_spawn_count);
	object->o_Position				= m_artefact_spawn_positions[index].level_point();
	object->m_tNodeID				= m_artefact_spawn_positions[index].level_vertex_id();
	object->m_fDistance				= m_artefact_spawn_positions[index].distance();
#ifdef DEBUG
	if (psAI_Flags.test(aiALife)) {
		Msg							("[LSS] Zone %s[%f][%f][%f] %d: generated artefact position %s[%f][%f][%f]",anomaly->name_replace(),VPUSH(anomaly->o_Position),anomaly->m_artefact_position_offset,object->name_replace(),VPUSH(object->o_Position));
	}
#endif
}

IC	const CALifeSpawnRegistry::SPAWN_GRAPH &CALifeSpawnRegistry::spawns	() const
{
	return							(m_spawns);
}

IC	void CALifeSpawnRegistry::process_spawns		(SPAWN_IDS &spawns)
{
	std::sort						(spawns.begin(),spawns.end());
	spawns.erase					(
		std::unique(
			spawns.begin(),
			spawns.end()
		),
		spawns.end()
	);
}

IC	const ALife::_SPAWN_ID &CALifeSpawnRegistry::spawn_id	(const ALife::_SPAWN_STORY_ID &spawn_story_id) const
{
	SPAWN_STORY_IDS::const_iterator	I = m_spawn_story_ids.find(spawn_story_id);
	VERIFY2							(I != m_spawn_story_ids.end(),"Spawn story id cannot be found");
	return							((*I).second);
}
