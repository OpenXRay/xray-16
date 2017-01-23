#pragma once

template <typename Object>
CStateBurerAttackGravi<Object>::CStateBurerAttackGravi(Object *obj) : inherited(obj)
{
	m_next_gravi_allowed_tick		=	0;
	m_anim_end_tick					=	0;
}

template <typename Object>
void CStateBurerAttackGravi<Object>::initialize()
{
	inherited::initialize				();
	m_action						=	ACTION_GRAVI_STARTED;
	m_time_gravi_started			=	0;
	m_anim_end_tick					=	0;
	m_next_gravi_allowed_tick		=	current_time() + object->m_gravi.cooldown;
	object->set_force_gravi_attack		(false);
	object->set_script_capture			(false);
}

template <typename Object>
void CStateBurerAttackGravi<Object>::execute()
{
	switch ( m_action )
	{
		case ACTION_GRAVI_STARTED:
			ExecuteGraviStart			();
			break;

		case ACTION_GRAVI_CONTINUE:
			ExecuteGraviContinue		();
			break;

		case ACTION_GRAVI_FIRE:
			ExecuteGraviFire			();
			m_action				=	 ACTION_WAIT_ANIM_END;
			break;

		case ACTION_WAIT_ANIM_END:
			if ( current_time() > m_anim_end_tick ) 
			{
				m_action			=	ACTION_COMPLETED; 
			}

		case ACTION_COMPLETED:
			break;
	}

	object->face_enemy						();
	
	if ( current_time() < m_anim_end_tick ) 
	{
		object->anim().set_override_animation	(eAnimGraviFire);
	}

	object->set_action						(ACT_STAND_IDLE);
}

template <typename Object>
void CStateBurerAttackGravi<Object>::finalize()
{
	inherited::finalize();
	object->set_script_capture			(true);
}

template <typename Object>
void CStateBurerAttackGravi<Object>::critical_finalize()
{
	inherited::critical_finalize		();
	
	object->StopGraviPrepare			();
	object->set_script_capture			(false);
}

template <typename Object>
bool CStateBurerAttackGravi<Object>::check_start_conditions()
{
	// обработать объекты
	if ( object->get_force_gravi_attack() ) return true;
	float dist = object->Position().distance_to(object->EnemyMan.get_enemy()->Position());
	if ( current_time() < m_next_gravi_allowed_tick ) return false;
	if ( dist < object->m_gravi.min_dist ) return false;
	if ( dist > object->m_gravi.max_dist ) return false;
	if ( !object->EnemyMan.see_enemy_now() ) return false; 
	if ( !object->control().direction().is_face_target(object->EnemyMan.get_enemy(), deg(45)) ) return false;

	return								true;
}

template <typename Object>
bool CStateBurerAttackGravi<Object>::check_completion()
{
	return								m_action == ACTION_COMPLETED;
}

//////////////////////////////////////////////////////////////////////////

template <typename Object>
void CStateBurerAttackGravi<Object>::ExecuteGraviStart()
{
	float const time				=	object->anim().get_animation_length (eAnimGraviFire, 0);
	m_anim_end_tick					=	current_time() + TTime(time*1000);
	m_action						=	ACTION_GRAVI_CONTINUE;
	m_time_gravi_started			=	Device.dwTimeGlobal;
	object->StartGraviPrepare			();
}

template <typename Object>
void CStateBurerAttackGravi<Object>::ExecuteGraviContinue()
{
	// проверить на грави удар
	float dist						=	object->Position().distance_to
										(object->EnemyMan.get_enemy()->Position());

	float time_to_hold				=	(abs(dist - object->m_gravi.min_dist) / object->m_gravi.min_dist);
	clamp								(time_to_hold, 0.f, 1.f);
	time_to_hold					*=	float(object->m_gravi.time_to_hold);

	if ( m_time_gravi_started + u32(time_to_hold) < Device.dwTimeGlobal )
	{
		m_action					=	ACTION_GRAVI_FIRE;
	}
}

template <typename Object>
void CStateBurerAttackGravi<Object>::ExecuteGraviFire()
{
	Fvector from_pos;
	Fvector target_pos;
	from_pos						=	object->Position();	
	from_pos.y						+=	0.5f;
	target_pos						=	object->EnemyMan.get_enemy()->Position();	
	target_pos.y					+=	0.5f;

	object->m_gravi_object.activate		(object->EnemyMan.get_enemy(), from_pos, target_pos);

	object->StopGraviPrepare			();
	object->sound().play				(CBurer::eMonsterSoundGraviAttack);
}
