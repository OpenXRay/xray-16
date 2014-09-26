#pragma once

#include "../../../../xrphysics/PhysicsShell.h"
//#include "../../../PHInterpolation.h"
//#include "../../../PHElement.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterRestFunAbstract CStateMonsterRestFun<_Object>

#define IMPULSE_TO_CORPSE	15.f
#define MIN_DELAY			100
#define TIME_IN_STATE		8000

TEMPLATE_SPECIALIZATION
CStateMonsterRestFunAbstract::CStateMonsterRestFun(_Object *obj) : inherited(obj)
{
}

TEMPLATE_SPECIALIZATION
void CStateMonsterRestFunAbstract::initialize()
{
	inherited::initialize	();

	time_last_hit			= 0;
}


TEMPLATE_SPECIALIZATION
void CStateMonsterRestFunAbstract::execute()
{
	Fvector point;
	float	dist;
	
	Fvector dir;
	dir.sub			(object->CorpseMan.get_corpse_position(), object->Position());
	dist			= dir.magnitude();
	dir.normalize	();
	point.mad		(object->CorpseMan.get_corpse_position(), dir, 2.0f);

	object->set_action									(ACT_RUN);
	object->path().set_target_point			(point);
	object->path().set_rebuild_time			(100 + u32(50.f * dist));
	object->path().set_use_covers			(false);
	object->path().set_distance_to_end		(0.5f);
	object->anim().accel_activate					(eAT_Calm);
	object->anim().accel_set_braking					(false);
	
	object->set_state_sound								(MonsterSound::eMonsterSoundIdle);
	
	if ((dist < object->db().m_fDistToCorpse + 0.5f) && (time_last_hit + MIN_DELAY < Device.dwTimeGlobal)) {
		CEntityAlive		*corpse = const_cast<CEntityAlive *>		(object->CorpseMan.get_corpse());
		CPhysicsShellHolder	*target = smart_cast<CPhysicsShellHolder *>	(corpse);

		if  (target && target->m_pPhysicsShell) {
			Fvector			dir;
			dir.add			(Fvector().sub(target->Position(), object->Position()), object->Direction());
			
			float			h,p;
			dir.getHP		(h,p);
			dir.setHP		(h, p + 5 * PI / 180);
			dir.normalize	();
			
			// выполнить бросок
			for (u32 i=0; i<target->m_pPhysicsShell->get_ElementsNumber();i++) {
				target->m_pPhysicsShell->get_ElementByStoreOrder((u16)i)->applyImpulse(dir, IMPULSE_TO_CORPSE * target->m_pPhysicsShell->getMass() / target->m_pPhysicsShell->Elements().size());
			}

			time_last_hit	= Device.dwTimeGlobal;
		}
	}
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterRestFunAbstract::check_start_conditions()
{
	return ((object->CorpseMan.get_corpse() != 0) && object->Home->at_home(object->CorpseMan.get_corpse()->Position()));
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterRestFunAbstract::check_completion()
{
	if (!object->CorpseMan.get_corpse()) return true;
	if (time_state_started + TIME_IN_STATE < Device.dwTimeGlobal) return true;
	return false;
}

#undef TIME_IN_STATE
#undef MIN_DELAY
#undef IMPULSE_TO_CORPSE
#undef CStateMonsterRestFunAbstract
#undef TEMPLATE_SPECIALIZATION