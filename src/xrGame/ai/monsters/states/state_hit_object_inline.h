#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateMonsterHitObjectAbstract CStateMonsterHitObject<_Object>

#define TEST_ANGLE			PI_DIV_6
#define TIME_OUT_STATE		1000
#define TIME_POINTBREAK		500
#define IMPULSE				20

TEMPLATE_SPECIALIZATION
void CStateMonsterHitObjectAbstract::initialize()
{
	inherited::initialize	();
	
	m_hitted				= false;		
}


TEMPLATE_SPECIALIZATION
void CStateMonsterHitObjectAbstract::execute()
{
	object->set_action				(ACT_STAND_IDLE);
	object->anim().SetSpecParams	(ASP_CHECK_CORPSE);
			
	if (!m_hitted && (time_state_started + TIME_POINTBREAK < Device.dwTimeGlobal)) {
		m_hitted		= true;
		
		Fvector			dir;
		dir.add			(Fvector().sub(target->Position(), object->Position()), object->Direction());
		dir.normalize	();
		target->m_pPhysicsShell->applyImpulse(dir,IMPULSE * target->m_pPhysicsShell->getMass());
	}
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterHitObjectAbstract::check_start_conditions()
{
	target									= 0;
	
	// получить физ. объекты в радиусе
	m_nearest_objects.clear_not_free		();
	Level().ObjectSpace.GetNearest			(m_nearest_objects,object->Position(), object->Radius() - 0.5f, object()); 
	
	xr_vector<CObject*>::iterator B = m_nearest_objects.begin();
	xr_vector<CObject*>::iterator E = m_nearest_objects.end();

	for (xr_vector<CObject*>::iterator I = B; I != E; I++)	 {
		CPhysicsShellHolder  *obj = smart_cast<CPhysicsShellHolder *>(*I);
		if (!obj || !obj->m_pPhysicsShell) continue;

		// определить дистанцию до врага
		Fvector d;
		d.sub(obj->Position(),object->Position());

		// проверка на  Field-Of-Hit
		float my_h,my_p;
		float h,p;

		object->Direction().getHP(my_h,my_p);
		d.getHP(h,p);

		float from	= angle_normalize(my_h - TEST_ANGLE);
		float to	= angle_normalize(my_h + TEST_ANGLE);

		if (!is_angle_between(h, from, to)) continue;

		from	= angle_normalize(my_p - TEST_ANGLE);
		to		= angle_normalize(my_p + TEST_ANGLE);

		if (!is_angle_between(p, from, to)) continue;

		target	= obj;
		return true;
	}
	
	return false;
}

TEMPLATE_SPECIALIZATION
bool CStateMonsterHitObjectAbstract::check_completion()
{
	if (time_state_started + TIME_OUT_STATE < Device.dwTimeGlobal) return true;
	return false;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateMonsterHitObjectAbstract
