#pragma once

#include "../anti_aim_ability.h"

template <class Object>
CStateBurerAntiAim<Object>::CStateBurerAntiAim (Object *obj) : inherited(obj)
{
	m_allow_anti_aim					=	false;
}

template <class Object>
void   CStateBurerAntiAim<Object>::initialize()
{
	inherited::initialize					();
	m_allow_anti_aim					=	true;
	object->control().activate				(ControlCom::eAntiAim);
	m_allow_anti_aim					=	false;

	VERIFY									( object->get_anti_aim()->is_active() );
}

template <class Object>
void   CStateBurerAntiAim<Object>::execute()
{
	object->face_enemy						();
	object->set_action						(ACT_STAND_IDLE);
}

template <class Object>
void   CStateBurerAntiAim<Object>::finalize()
{
	inherited::finalize();
}

template <class Object>
void   CStateBurerAntiAim<Object>::critical_finalize()
{
	inherited::critical_finalize			();
}

template <class Object>
bool   CStateBurerAntiAim<Object>::check_start_conditions()
{
	return									object->get_anti_aim()->check_start_condition();
}

template <class Object>
bool   CStateBurerAntiAim<Object>::check_completion()
{
	if ( !object->get_anti_aim()->is_active() )
	{
		return								true;
	}

	return									false;
}
