#include "stdafx.h"
#include "psy_aura.h"
#include "BaseMonster/base_monster.h"

CPsyAura::CPsyAura()
{
	m_object					= 0;
	m_radius					= 1.f;
}

CPsyAura::~CPsyAura()
{
}

void CPsyAura::schedule_update()
{
	inherited::schedule_update();
	
	if (is_active()){
		feel_touch_update(m_object->Position(), m_radius);
		process_objects_in_aura();
	}
}
