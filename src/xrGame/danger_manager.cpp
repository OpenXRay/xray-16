////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_manager.cpp
//	Created 	: 11.02.2005
//  Modified 	: 11.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger manager
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "danger_manager.h"
#include "custommonster.h"
#include "memory_space.h"
#include "profiler.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "actor.h"
#include "object_broker.h"

struct CDangerPredicate {
	const CObject	*m_object;

	IC			CDangerPredicate	(const CObject *object)
	{
		m_object			= object;
	}

	IC	bool	operator()			(const CDangerObject &object) const
	{
		return				(m_object == object.object());
	}
};

struct CFindPredicate {
	const CDangerObject		*m_object;

	IC			CFindPredicate		(const CDangerObject &object)
	{
		m_object			= &object;
	}

	IC	bool	operator()			(const CDangerObject &object) const
	{
		return				(*m_object == object);
	}
};

struct CRemoveByTimePredicate {
	u32						m_time_line;
	CDangerManager			*m_manager;

	IC			CRemoveByTimePredicate	(u32 time_line, CDangerManager *manager)
	{
		m_time_line			= time_line;
		VERIFY				(manager);
		m_manager			= manager;
	}

	IC	bool	operator()				(const CDangerObject &object) const
	{
		if (object.time() < m_time_line) {
			if (object.object() && (object.type() == CDangerObject::eDangerTypeFreshEntityCorpse))
				m_manager->ignore	(object.object());

			return			(true);
		}

		if (!object.object())
			return			(false);

		if (!m_manager->useful(object)) {
			if ((object.type() == CDangerObject::eDangerTypeFreshEntityCorpse))
				m_manager->ignore	(object.object());

			return			(true);
		}

		return				(false);
	}
};

CDangerManager::~CDangerManager		()
{
}

void CDangerManager::Load			(LPCSTR section)
{
}

void CDangerManager::reinit			()
{
	m_objects.clear			();
	m_ignored.clear			();
	m_time_line				= 0;
	m_selected				= 0;
}

void CDangerManager::reload			(LPCSTR section)
{
}

void CDangerManager::update			()
{
	START_PROFILE("Memory Manager/dangers::update")
	m_objects.erase			(
		std::remove_if(
			m_objects.begin(),
			m_objects.end(),
			CRemoveByTimePredicate(
				time_line(),
				this
			)
		),
		m_objects.end()
	);

	float					result = flt_max;
	m_selected				= 0;
	OBJECTS::const_iterator	I = m_objects.begin();
	OBJECTS::const_iterator	E = m_objects.end();
	for ( ; I != E; ++I) {
//		Msg					("%6d : Danger : [%d][%d]",(*I).time(),(*I).type(),(*I).perceive_type());
		float				value = do_evaluate(*I);
		if (result > value) {
			result			= value;
			m_selected		= &*I;
		}
	}

	STOP_PROFILE
}

void CDangerManager::remove_links	(const CObject *object)
{
	if (m_selected && (m_selected->object() == object))
		m_selected			= 0;

	m_objects.erase			(
		std::remove_if(
			m_objects.begin(),
			m_objects.end(),
			CDangerPredicate(object)
		),
		m_objects.end()
	);

	{
		OBJECTS::iterator	I = m_objects.begin();
		OBJECTS::iterator	E = m_objects.end();
		for ( ; I != E; ++I) {
			if (!(*I).dependent_object())
				continue;

			if ((*I).dependent_object() != object)
				continue;

			(*I).clear_dependent_object();
		}
	}

	IGNORED::iterator		I = std::find(m_ignored.begin(),m_ignored.end(),object->ID());
	if (I != m_ignored.end())
		m_ignored.erase		(I);
}

bool CDangerManager::useful			(const CDangerObject &object) const
{
	if (object.object() && !object.dependent_object()) {
		IGNORED::const_iterator	I = std::find(m_ignored.begin(),m_ignored.end(),object.object()->ID());
		if (I != m_ignored.end())
			return				(false);
	}

	if (object.time() >= time_line())
		return				(true);

	return					(false);
}

bool CDangerManager::is_useful		(const CDangerObject &object) const
{
	return					(m_object->useful(this,object));
}

float CDangerManager::evaluate		(const CDangerObject &object) const
{
	return					(m_object->evaluate(this,object));
}

float CDangerManager::do_evaluate	(const CDangerObject &object) const
{
	float					result = 0.f;
	switch (object.type()) {
		case CDangerObject::eDangerTypeBulletRicochet : { // I perceived bullet(knife) ricochet
			result			+= 3000.f;
			break;
		}
		case CDangerObject::eDangerTypeAttackSound : { // someone is shooting
			result			+= 2500.f;
			break;
		}
		case CDangerObject::eDangerTypeEntityAttacked : { // someone is hit
			result			+= 2000.f;
			break;
		}
		case CDangerObject::eDangerTypeEntityDeath : { // someone becomes dead
			result			+= 3000.f;
			break;
		}
		case CDangerObject::eDangerTypeFreshEntityCorpse : { // I see a corpse
			result			+= 2250.f;
			break;
		}
		case CDangerObject::eDangerTypeAttacked : { // someone is attacked
			result			+= 2000.f;
			break;
		}
		case CDangerObject::eDangerTypeGrenade : { // grenade to explode nearby
			result			+= 1000.f;
			break;
		}
		case CDangerObject::eDangerTypeEnemySound : { // grenade to explode nearby
			result			+= 1000.f;
			break;
		}
		default : NODEFAULT;
	}

	result					*= 10.f;
	result					+= float(Device.dwTimeGlobal - object.time());

	return					(result);
}

void CDangerManager::add			(const CVisibleObject &object)
{
	if (!object.m_enabled)
		return;

	const CEntityAlive		*obj = smart_cast<const CEntityAlive*>(object.m_object);
	if (obj && !obj->g_Alive() && (obj->killer_id() != ALife::_OBJECT_ID(-1))) {
		add					(CDangerObject(obj,obj->Position(),object.m_level_time,CDangerObject::eDangerTypeFreshEntityCorpse,CDangerObject::eDangerPerceiveTypeVisual));
		return;
	}
}

void CDangerManager::add			(const CSoundObject &object)
{
	if (!object.m_enabled)
		return;
	
	const CEntityAlive		*obj = smart_cast<const CEntityAlive*>(object.m_object);
	
	if ((object.m_sound_type & SOUND_TYPE_BULLET_HIT) == SOUND_TYPE_BULLET_HIT) {
		add					(CDangerObject(obj,object.m_object_params.m_position,object.m_level_time,CDangerObject::eDangerTypeBulletRicochet,CDangerObject::eDangerPerceiveTypeSound));
		return;
	}

	if ((object.m_sound_type & SOUND_TYPE_WEAPON_SHOOTING) == SOUND_TYPE_WEAPON_SHOOTING) {
		add					(CDangerObject(obj,object.m_object_params.m_position,object.m_level_time,CDangerObject::eDangerTypeAttackSound,CDangerObject::eDangerPerceiveTypeSound));
		return;
	}

	if ((object.m_sound_type & SOUND_TYPE_INJURING) == SOUND_TYPE_INJURING) {
		bool				do_add = true;
		if (object.m_object) {
			const CActor	*actor = smart_cast<const CActor*>(object.m_object);
			if (actor && !m_object->is_relation_enemy(actor))
				do_add		= false;
		}
		if (do_add)
			add				(CDangerObject(obj,object.m_object_params.m_position,object.m_level_time,CDangerObject::eDangerTypeEntityAttacked,CDangerObject::eDangerPerceiveTypeSound));
		return;
	}

	if ((object.m_sound_type & SOUND_TYPE_DYING) == SOUND_TYPE_DYING) {
		add					(CDangerObject(obj,object.m_object_params.m_position,object.m_level_time,CDangerObject::eDangerTypeEntityDeath,CDangerObject::eDangerPerceiveTypeSound));
		return;
	}

	if (obj && m_object->is_relation_enemy(obj)) {
		add					(CDangerObject(obj,object.m_object_params.m_position,object.m_level_time,CDangerObject::eDangerTypeEnemySound,CDangerObject::eDangerPerceiveTypeSound));
		return;
	}
}

void CDangerManager::add			(const CHitObject &object)
{
	if (!object.m_enabled)
		return;

	if (fis_zero(object.m_amount))
		return;

	if (object.m_object->ID() == m_object->ID())
		return;

	const CEntityAlive		*obj = smart_cast<const CEntityAlive*>(object.m_object);
	add						(CDangerObject(obj,obj->Position(),object.m_level_time,CDangerObject::eDangerTypeAttacked,CDangerObject::eDangerPerceiveTypeHit));
}

void CDangerManager::add			(const CDangerObject &object)
{
	if (m_object->memory().enemy().selected() && object.object())// && !object.object()->g_Alive())
		ignore				(object.object());

	if (!is_useful(object))
		return;

	OBJECTS::iterator		I = std::find_if(m_objects.begin(),m_objects.end(),CFindPredicate(object));
	if (I != m_objects.end()) {
		*I					= object;
		return;
	}

	m_objects.push_back		(object);
}

void CDangerManager::ignore			(const CGameObject *object)
{
	VERIFY					(object);
	IGNORED::const_iterator	I = std::find(m_ignored.begin(),m_ignored.end(),object->ID());
	if (I != m_ignored.end())
		return;

	m_ignored.push_back		(object->ID());
}

void CDangerManager::save			(NET_Packet &packet) const
{
	save_data				(m_ignored,packet);
}

void CDangerManager::load			(IReader &packet)
{
	load_data				(m_ignored,packet);
}
