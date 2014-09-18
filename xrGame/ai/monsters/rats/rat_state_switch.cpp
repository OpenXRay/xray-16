#include "pch_script.h"
#include "ai_rat.h"
#include "../../ai_monsters_misc.h"
#include "../../../game_level_cross_table.h"
#include "../../../game_graph.h"
#include "ai_rat_space.h"
#include "../../../../Include/xrRender/KinematicsAnimated.h"
#include "../../../detail_path_manager.h"
#include "../../../memory_manager.h"
#include "../../../enemy_manager.h"
#include "../../../item_manager.h"
#include "../../../memory_space.h"
#include "../../../ai_object_location.h"
#include "../../../movement_manager.h"
#include "../../../sound_player.h"
#include "ai_rat_impl.h"
#include "../../../ai_space.h"
#include "ai/monsters/ai_monster_squad_manager.h"
#include "ai/monsters/ai_monster_squad.h"

bool CAI_Rat::switch_to_attack_melee()
{
	if (!switch_if_porsuit() ||	!switch_if_home())
	{
		return true;
	}
	return false;
}

const CEntityAlive *CAI_Rat::get_enemy()
{
	return memory().enemy().selected();
}

bool CAI_Rat::switch_if_home()
{
	if (Position().distance_to(m_home_position) < m_fMaxHomeRadius)
	{
		return true;
	}
	return false;
}

bool CAI_Rat::switch_if_position()
{
	if (memory().enemy().selected()->Position().distance_to(Position()) > m_fAttackDistance)
	{
		return true;
	}
	return false;
}

bool CAI_Rat::switch_if_enemy()
{
	if (memory().enemy().selected())
	{
		return true;
	}
	return false;
}

bool CAI_Rat::get_morale()
{
	if (m_fMorale >= m_fMoraleNormalValue - EPS_L)
	{
		return true;
	}
	return false;
}

bool CAI_Rat::switch_to_eat()
{
	if (memory().item().selected())
	{
		return true;
	}
	return false;
}

bool CAI_Rat::get_if_dw_time()
{
	if (m_tLastSound.dwTime >= m_dwLastUpdateTime)
	{
		return true;
	}
	return false;
}

bool CAI_Rat::get_if_tp_entity()
{
	if ( m_tLastSound.tpEntity && (m_tLastSound.tpEntity->g_Team() != g_Team()) && (!bfCheckIfSoundFrightful()))
	{
		return true;
	}
	return false;
}

void CAI_Rat::set_previous_query_time()
{
	m_previous_query_time = Device.dwTimeGlobal;
}

SRotation CAI_Rat::sub_rotation()
{
	Fvector tTemp;
	tTemp.sub(memory().enemy().selected()->Position(),Position());
	vfNormalizeSafe(tTemp);
	SRotation sTemp;
	mk_rotation(tTemp,sTemp);
	return sTemp;
}

CAI_Rat::ERatStates CAI_Rat::get_state()
{
	return ERatStates(dwfChooseAction(m_dwActionRefreshRate,m_fAttackSuccessProbability,m_fAttackSuccessProbability,m_fAttackSuccessProbability,m_fAttackSuccessProbability,g_Team(),g_Squad(),g_Group(),aiRatAttackMelee,aiRatAttackMelee,aiRatAttackMelee,aiRatRetreat,aiRatRetreat,this,30.f));
}

bool CAI_Rat::switch_if_porsuit()
{
	if (m_home_position.distance_to(memory().enemy().selected()->Position()) > m_fMaxPursuitRadius)
	{
		return true;
	}
	return false;
}

void CAI_Rat::set_dir()
{
	if ((Device.dwTimeGlobal - m_previous_query_time > TIME_TO_GO) || !m_previous_query_time) {
		CMonsterSquad *squad	= monster_squad().get_squad(this);
		Fvector m_enemy_position = memory().enemy().selected()->Position();
		if (squad && squad->SquadActive())
		{
			float m_delta_Angle = angle_normalize((PI * 2) / squad->squad_alife_count());
			float m_heading, m_pitch;
			Fvector m_temp, m_dest_direction;
			m_temp = squad->GetLeader()->Position();
			m_dest_direction.x = (m_temp.x - m_enemy_position.x) / m_temp.distance_to(m_enemy_position);
			m_dest_direction.y = (m_temp.y - m_enemy_position.y) / m_temp.distance_to(m_enemy_position);
			m_dest_direction.z = (m_temp.z - m_enemy_position.z) / m_temp.distance_to(m_enemy_position);
			m_dest_direction.getHP(m_heading, m_pitch);
			m_heading = angle_normalize(m_heading + m_delta_Angle * squad->get_index(this));
			m_dest_direction.setHP(m_heading, m_pitch);
			m_dest_direction.mul(0.5f);
			m_enemy_position.add(m_enemy_position,m_dest_direction);
		}

		m_tGoalDir.set(m_enemy_position);
	}
}

void CAI_Rat::set_dir_m()
{

	if ((Device.dwTimeGlobal - m_previous_query_time > TIME_TO_GO) || !m_previous_query_time)
		m_tGoalDir.set(memory().memory(memory().enemy().selected()).m_object_params.m_position);
}

void CAI_Rat::set_sp_dir()
{
	if ((Device.dwTimeGlobal - m_previous_query_time > TIME_TO_GO) || !m_previous_query_time)
		m_tGoalDir = m_tSpawnPosition;
}

void CAI_Rat::set_way_point()
{
	m_tGoalDir = get_next_target_point();
}

bool CAI_Rat::switch_if_no_enemy()
{
	if	(!switch_if_enemy() ||
		(switch_if_enemy() && 
		(
		!switch_if_alife()
		|| 
		(
		(Device.dwTimeGlobal - memory().memory(memory().enemy().selected()).m_level_time > m_dwRetreatTime) && 
		(
		(m_tLastSound.dwTime < m_dwLastUpdateTime) || 
		!m_tLastSound.tpEntity || 
		(m_tLastSound.tpEntity->g_Team() == g_Team()) || 
		!bfCheckIfSoundFrightful()
		)
		)
		)
		)
		)
	{
		memory().enable	(memory().enemy().selected(),false);
		return true;
	}
	return false;
}

bool CAI_Rat::switch_to_free_recoil()
{
	if	(
		(m_tLastSound.dwTime >= m_dwLastUpdateTime) && 
		(
		!m_tLastSound.tpEntity || 
		(
		(!switch_to_eat() || (memory().item().selected()->ID() != m_tLastSound.tpEntity->ID())) && 
		(m_tLastSound.tpEntity->g_Team() != g_Team())
		)
		) && 
		!switch_if_enemy()
		)
	{
		return true;
	}
	return false;
}

bool CAI_Rat::switch_if_lost_time()
{
	if (switch_if_enemy() && (Device.dwTimeGlobal - memory().memory(memory().enemy().selected()).m_level_time >= m_dwLostMemoryTime)) 
	{
		memory().enable(memory().enemy().selected(),false);
		return true;
	}
	return false;
}

bool CAI_Rat::switch_if_lost_rtime()
{
	if (Device.dwTimeGlobal - memory().memory(memory().enemy().selected()).m_level_time >= m_dwLostRecoilTime)
	{
		return true;
	}
	return false;
}

bool CAI_Rat::switch_if_alife()
{
	if (memory().enemy().selected()->g_Alive())
	{
		return true;
	}
	return false;
}

bool CAI_Rat::switch_if_diff()
{
	SRotation sTemp = sub_rotation();
	if (angle_difference(movement().m_body.current.yaw,sTemp.yaw) > m_fAttackAngle)
	{
		return true;
	}
	return false;
}

bool CAI_Rat::switch_if_dist_no_angle()
{
	if (!switch_if_position() && switch_if_diff()) {
		m_fSpeed = 0.f;
		return true;
	}
	return false;
}


bool CAI_Rat::switch_if_dist_angle()
{
	if(!switch_if_position() && !switch_if_diff())
	{
		return true;
	}
	return false;
}

void CAI_Rat::set_rew_position()
{
	Fvector tTemp;
	tTemp.sub(memory().enemy().selected()->Position(),Position());
	vfNormalizeSafe(tTemp);
	tTemp.sub(Position(),memory().enemy().selected()->Position());
	tTemp.normalize_safe();
	tTemp.mul(m_fRetreatDistance);
	m_tSpawnPosition.add(Position(),tTemp);
}

bool CAI_Rat::switch_if_time()
{
	if (m_dwLastUpdateTime > m_dwLostRecoilTime + 2000)
	{
		return true;
	}
	return false;
}

void CAI_Rat::set_rew_cur_position()
{
	Fvector tTemp;
	tTemp.setHP(-movement().m_body.current.yaw,-movement().m_body.current.pitch);
	tTemp.normalize_safe();
	tTemp.mul(m_fUnderFireDistance);
	m_tSpawnPosition.add(Position(),tTemp);
}

void CAI_Rat::set_home_pos()
{
	if ((Device.dwTimeGlobal - m_previous_query_time > TIME_TO_GO) || !m_previous_query_time)
		m_tGoalDir.set			(m_home_position);
}

void CAI_Rat::set_goal_time(float f_val)
{
	m_fGoalChangeTime = 0;
}

bool CAI_Rat::get_alife()
{
	if (!g_Alive()) {
		m_fSpeed = m_fSafeSpeed = 0;
		return false;
	}
	return true;
}

void CAI_Rat::set_movement_type(bool bCanAdjustSpeed, bool bStraightForward)
{
	m_bCanAdjustSpeed	= bCanAdjustSpeed;
	m_bStraightForward	= bStraightForward;
}

bool CAI_Rat::check_completion_no_way()
{
	if(time_to_next_attack + time_old_attack < Device.dwTimeGlobal ) return true;
	return false;
}