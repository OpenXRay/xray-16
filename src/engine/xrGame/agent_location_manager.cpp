////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_location_manager.cpp
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Agent location manager
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "agent_location_manager.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "agent_enemy_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "cover_point.h"

const float MIN_SUITABLE_ENEMY_DISTANCE = 3.f;//10.f;

struct CRemoveOldDangerCover {
	typedef CAgentMemberManager::MEMBER_STORAGE MEMBER_STORAGE;

	CAgentMemberManager		*m_members;

	IC			CRemoveOldDangerCover	(CAgentMemberManager *members)
	{
		VERIFY				(members);
		m_members			= members;
	}

	IC	bool	operator()				(const CAgentLocationManager::CDangerLocationPtr &location) const
	{
		if (!location->useful()) {
			MEMBER_STORAGE::iterator		I = m_members->members().begin();
			MEMBER_STORAGE::iterator		E = m_members->members().end();
			for ( ; I != E; ++I) {
				if (!location->mask().test(m_members->mask(&(*I)->object())))
					continue;

				(*I)->object().on_danger_location_remove	(*location);
			}
		}

		return			(!location->useful());
	}
};

struct CDangerLocationPredicate {
	Fvector				m_position;

	IC			CDangerLocationPredicate	(const Fvector &position)
	{
		m_position		= position;
	}

	IC	bool	operator()	(const CAgentLocationManager::CDangerLocationPtr &location) const
	{
		return			(*location == m_position);
	}
};

IC	CAgentLocationManager::CDangerLocationPtr CAgentLocationManager::location	(const Fvector &position)
{
	LOCATIONS::iterator	I = std::find_if(m_danger_locations.begin(),m_danger_locations.end(),CDangerLocationPredicate(position));
	if (I != m_danger_locations.end())
		return			(*I);
	return				(0);
}

bool CAgentLocationManager::suitable	(CAI_Stalker *object, const CCoverPoint *location, bool use_enemy_info) const
{
	CAgentMemberManager::const_iterator	I = this->object().member().members().begin();
	CAgentMemberManager::const_iterator	E = this->object().member().members().end();
	for ( ; I != E; ++I) {
		if ((*I)->object().ID() == object->ID())
			continue;

		if (!(*I)->cover()) {
			if (this->object().member().registered_in_combat(&(*I)->object()))
				continue;

			if ((*I)->object().Position().distance_to_sqr(location->position()) <= _sqr(5.f))
				return					(false);

			continue;
		}

		// check if member cover is too close
		if ((*I)->cover()->m_position.distance_to_sqr(location->position()) <= _sqr(5.f))
			// so member cover is too close
//			if ((*I)->object().Position().distance_to_sqr(location->position()) <= object->Position().distance_to_sqr(location->position()))
			// check if member to its cover is more close than we to our cover
			if ((*I)->object().Position().distance_to_sqr((*I)->cover()->m_position) <= object->Position().distance_to_sqr(location->position()) + 2.f)
				return				(false);
	}

	if (use_enemy_info) {
		CAgentEnemyManager::ENEMIES::const_iterator	I = this->object().enemy().enemies().begin();
		CAgentEnemyManager::ENEMIES::const_iterator	E = this->object().enemy().enemies().end();
		for ( ; I != E; ++I)
			if ((*I).m_enemy_position.distance_to_sqr(location->position()) < _sqr(MIN_SUITABLE_ENEMY_DISTANCE))
				return				(false);
	}

	return							(true);
}

void CAgentLocationManager::make_suitable	(CAI_Stalker *object, const CCoverPoint *location) const
{
	this->object().member().member(object).cover(location);

	if (!location)
		return;

	CAgentMemberManager::const_iterator	I = this->object().member().members().begin();
	CAgentMemberManager::const_iterator	E = this->object().member().members().end();
	for ( ; I != E; ++I) {
		if ((*I)->object().ID() == object->ID())
			continue;

		if (!(*I)->cover())
			continue;

		// check if member cover is too close
		if ((*I)->cover()->m_position.distance_to_sqr(location->position()) <= _sqr(5.f)) {
//			Msg						("%6d : object [%s] disabled cover for object [%s]",Device.dwFrame,*object->cName(),*(*I)->object().cName());
			(*I)->object().on_cover_blocked	((*I)->cover());
			(*I)->cover						(0);
		}
	}
}

void CAgentLocationManager::add	(CDangerLocationPtr location)
{
	typedef CAgentMemberManager::MEMBER_STORAGE MEMBER_STORAGE;
	MEMBER_STORAGE::iterator		I = object().member().members().begin();
	MEMBER_STORAGE::iterator		E = object().member().members().end();
	for ( ; I != E; ++I) {
		if	(!location->mask().test(object().member().mask(&(*I)->object())))
			continue;

		(*I)->object().on_danger_location_add	(*location);
	}

	CDangerLocationPtr				danger = this->location(location->position());
	if (!danger) {
		m_danger_locations.push_back(location);
		return;
	}

	danger->m_level_time			= location->m_level_time;
	
	if (danger->m_interval < location->m_interval)
		danger->m_interval			= location->m_interval;
	
	if (danger->m_radius < location->m_radius)
		danger->m_radius			= location->m_radius;
}

void CAgentLocationManager::remove_old_danger_covers	()
{
	m_danger_locations.erase	(
		std::remove_if(
			m_danger_locations.begin(),
			m_danger_locations.end(),
			CRemoveOldDangerCover(
				&object().member()
			)
		),
		m_danger_locations.end()
	);
}

float CAgentLocationManager::danger		(const CCoverPoint *cover, CAI_Stalker *member) const
{
	float						result = 1;
	squad_mask_type				mask = object().member().mask(member);
	LOCATIONS::const_iterator	I = m_danger_locations.begin();
	LOCATIONS::const_iterator	E = m_danger_locations.end();
	for ( ; I != E; ++I) {
		if (Device.dwTimeGlobal > (*I)->m_level_time + (*I)->m_interval)
			continue;

		if (!(*I)->mask().test(mask))
			continue;

		float					distance = 1.f + (*I)->position().distance_to(cover->position());
		if (distance > (*I)->m_radius)
			continue;

		result					*= 
			float(Device.dwTimeGlobal - (*I)->m_level_time)/float((*I)->m_interval);
	}

	return						(result);
}

void CAgentLocationManager::update	()
{
	remove_old_danger_covers	();
}

void CAgentLocationManager::remove_links(CObject *object)
{
	m_danger_locations.erase	(
		std::remove_if(	
			m_danger_locations.begin(),
			m_danger_locations.end(),
			CRemoveDangerObject(object)
		),
		m_danger_locations.end()
	);
}
