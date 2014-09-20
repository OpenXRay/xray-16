////////////////////////////////////////////////////////////////////////////
//	Module 		: rat_states.cpp
//	Created 	: 31.08.2007
//  Modified 	: 24.10.2007
//	Author		: Dmitriy Iassenev
//	Developer	: Ivan Andrushchenko
//	Description : rat states classes
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rat_states.h"
#include "ai/monsters/rats/ai_rat.h"
#include "ai/monsters/ai_monster_squad_manager.h"
#include "ai/monsters/ai_monster_squad.h"
#include "entity_alive.h"

////////////////////////////////////////////////////////////////////////////
// rat_state_death
////////////////////////////////////////////////////////////////////////////

void rat_state_death::initialize		()
{
}

void rat_state_death::execute			()
{
	if (object().m_fFood > 0.f)
		return;

	object().setEnabled			(FALSE);

	NET_Packet					packet;
	object().u_EventGen			(packet, GE_DESTROY, object().ID());
	object().u_EventSend		(packet);
}

void rat_state_death::finalize			()
{
}

////////////////////////////////////////////////////////////////////////////
// rat_state_free_active
////////////////////////////////////////////////////////////////////////////

void rat_state_free_active::initialize	()
{
	if (object().m_walk_on_way) return;
	object().init_free_active();
}

void rat_state_free_active::execute		()
{
	if (!object().get_alife()) return;

	if (object().m_walk_on_way)
	{
		object().m_state_manager->push_state	(CAI_Rat::aiRatNoWay);
		return;
	}

	
	if (object().switch_if_enemy()&&(!object().switch_if_porsuit()||!object().switch_if_home())) {
		object().m_state_manager->push_state	(CAI_Rat::aiRatAttackMelee);
		return;
	}

	if (!object().get_morale()) {
		object().m_state_manager->push_state	(CAI_Rat::aiRatUnderFire);
		return;
	}

	if (object().switch_to_free_recoil())
	{
		object().m_state_manager->push_state	(CAI_Rat::aiRatFreeRecoil);
		return;
	}

	if (object().switch_to_eat())
	{
		object().m_state_manager->push_state	(CAI_Rat::aiRatEatCorpse);
		return;
	}

	object().activate_state_free_active();

}

void rat_state_free_active::finalize	()
{
}

////////////////////////////////////////////////////////////////////////////
// rat_state_free_passive
////////////////////////////////////////////////////////////////////////////

void rat_state_free_passive::initialize	()
{
}

void rat_state_free_passive::execute	()
{
	if (!object().get_alife()) return;

	if (object().m_walk_on_way)
	{
		object().m_state_manager->change_state	(CAI_Rat::aiRatNoWay);
		return;
	}

	object().activate_state_free_passive();
}

void rat_state_free_passive::finalize	()
{
}

////////////////////////////////////////////////////////////////////////////
// rat_state_attack_range
////////////////////////////////////////////////////////////////////////////

void rat_state_attack_range::initialize	()
{
}

void rat_state_attack_range::execute	()
{
	if (!object().get_alife()) return;
	if (!object().switch_if_enemy())
	{
		object().m_state_manager->pop_state();
		return;
	}
	if (!object().m_attack_rebuild && !object().can_stand_in_position() || object().switch_if_position() || object().switch_if_diff())
	{
		object().m_state_manager->push_state(CAI_Rat::aiRatNoWay);
		return;
	}

	object().activate_state_attack_range();
}

void rat_state_attack_range::finalize	()
{
	object().fire(false);
}

////////////////////////////////////////////////////////////////////////////
// rat_state_attack_melee
////////////////////////////////////////////////////////////////////////////

void rat_state_attack_melee::initialize	()
{
}

void rat_state_attack_melee::execute	()
{
	if (!object().get_alife()) return;
	if (object().get_state()!=CAI_Rat::aiRatAttackMelee)
	{
		object().m_state_manager->change_state(object().get_state());
		return;
	}
	if (object().switch_if_enemy())
	{
		if (object().switch_if_porsuit())
		{
			object().m_state_manager->change_state(CAI_Rat::aiRatReturnHome);
			return;
		}
	} else {
		object().m_state_manager->pop_state();
		return;
	}
	if (object().switch_if_dist_angle())
	{
		object().m_state_manager->change_state(CAI_Rat::aiRatAttackRange);
		return;
	}
	if (object().switch_if_dist_no_angle())
	{
		object().activate_turn();
		object().activate_state_move();
		//object().m_state_manager->change_state(CAI_Rat::aiRatAttackRange);
		return;
	}

	CMonsterSquad *squad	= monster_squad().get_squad(object().cast_entity());

	if (squad && 
		((squad->GetLeader() != object().cast_entity() && !squad->GetLeader()->g_Alive()) ||
		squad->get_index(object().cast_entity()) == u32(-1))
		) squad->SetLeader(object().cast_entity());

	if (squad &&
		squad->SquadActive() &&
		squad->GetLeader() == object().cast_entity() &&
		object().m_squad_count != squad->squad_alife_count()
		) 
	{
		squad->set_rat_squad_index(object().get_enemy());
		object().m_squad_count = squad->squad_alife_count();
	}

	object().set_dir();
	object().activate_state_move();
}

void rat_state_attack_melee::finalize	()
{
}

////////////////////////////////////////////////////////////////////////////
// rat_state_under_fire
////////////////////////////////////////////////////////////////////////////

void rat_state_under_fire::initialize	()
{
	object().init_state_under_fire();
}

void rat_state_under_fire::execute		()
{
	if (!object().get_alife()) return;
	if (object().switch_if_enemy())
	{
		object().m_state_manager->change_state	(CAI_Rat::aiRatAttackMelee);
		return;
	} else {
		if (object().get_if_dw_time())
		{
			if (object().get_if_tp_entity()) {
				object().m_state_manager->push_state	(CAI_Rat::aiRatAttackMelee);
				return;
			}
			object().set_previous_query_time();
		}
		if (object().get_morale()) {
			object().m_state_manager->pop_state();
			return;
		}

	}
	object().activate_state_move();	
}

void rat_state_under_fire::finalize		()
{
}

////////////////////////////////////////////////////////////////////////////
// rat_state_retreat
////////////////////////////////////////////////////////////////////////////

void rat_state_retreat::initialize		()
{
}

void rat_state_retreat::execute			()
{
	if (!object().get_alife()) return;

	if (object().m_walk_on_way)
	{
		object().m_state_manager->push_state	(CAI_Rat::aiRatNoWay);
		return;
	}

	if (object().switch_if_no_enemy())
	{
		object().m_state_manager->pop_state();
		return;
	}
	if (object().switch_if_enemy()&&object().switch_if_alife())
	{
		if (object().get_state()!=CAI_Rat::aiRatRetreat)
		{
			object().m_state_manager->change_state(object().get_state());
			return;
		}
		if (object().switch_if_dist_no_angle())
		{
			object().activate_state_move();
			return;
		}
		if (object().switch_if_dist_angle())
		{
			object().m_state_manager->change_state(CAI_Rat::aiRatAttackRange);
			return;
		}
		object().set_rew_position();
	}
	object().set_sp_dir();
	object().activate_state_move();
}

void rat_state_retreat::finalize		()
{
}

////////////////////////////////////////////////////////////////////////////
// rat_state_pursuit
////////////////////////////////////////////////////////////////////////////

void rat_state_pursuit::initialize		()
{
}

void rat_state_pursuit::execute			()
{
	if (!object().get_alife()) return;

	if (object().m_walk_on_way)
	{
		object().m_state_manager->push_state	(CAI_Rat::aiRatNoWay);
		return;
	}

	if (object().switch_if_lost_time())
	{
		object().m_state_manager->pop_state();
		return;
	}
	if (object().switch_if_enemy())
	{
		object().m_state_manager->push_state(CAI_Rat::aiRatAttackMelee);
		return;
	}
	if (!object().get_morale())
	{
		object().m_state_manager->push_state(CAI_Rat::aiRatUnderFire);
		return;
	}
	if (object().switch_to_free_recoil())
	{
		object().m_state_manager->change_state(CAI_Rat::aiRatFreeRecoil);
		return;
	}

	object().set_dir_m();

	object().activate_state_move();
}

void rat_state_pursuit::finalize		()
{
}

////////////////////////////////////////////////////////////////////////////
// rat_state_free_recoil
////////////////////////////////////////////////////////////////////////////

void rat_state_free_recoil::initialize	()
{
	if (object().m_walk_on_way) return;

	object().init_free_recoil();
	object().set_rew_cur_position();
}

void rat_state_free_recoil::execute		()
{
	if (!object().get_alife()) return;

	if (object().m_walk_on_way)
	{
		object().m_state_manager->change_state	(CAI_Rat::aiRatNoWay);
		return;
	}

	if (object().switch_if_enemy())
	{
		object().m_state_manager->pop_state();
		return;
	}
	if (object().switch_if_time())
	{
		object().m_state_manager->pop_state();
		return;
	}
	if (object().switch_if_enemy()&&object().switch_if_lost_rtime())
	{
		object().m_state_manager->change_state(CAI_Rat::aiRatPursuit);
		return;
	}
	object().activate_state_free_recoil();
}

void rat_state_free_recoil::finalize	()
{
}

////////////////////////////////////////////////////////////////////////////
// rat_state_return_home
////////////////////////////////////////////////////////////////////////////

void rat_state_return_home::initialize	()
{
}

void rat_state_return_home::execute		()
{
	if (!object().get_alife()) return;

	if (object().m_walk_on_way)
	{
		object().m_state_manager->push_state	(CAI_Rat::aiRatNoWay);
		return;
	}

	if (object().switch_if_enemy()&&!object().switch_if_porsuit()){
		object().set_goal_time();
		object().m_state_manager->push_state(CAI_Rat::aiRatAttackMelee);
		return;
	}
	if (object().switch_if_enemy()){
		if (object().switch_if_dist_no_angle())
		{
			return;
		}
		if (object().switch_if_dist_angle())
		{
			object().m_state_manager->change_state(CAI_Rat::aiRatAttackRange);
			return;
		}
	}
	if (!object().switch_if_enemy()||object().switch_if_home()||!object().switch_if_alife())
	{
		object().m_state_manager->pop_state();
		return;
	}
	object().set_home_pos();
	object().activate_state_home();
}

void rat_state_return_home::finalize	()
{
}

////////////////////////////////////////////////////////////////////////////
// rat_state_eat_corpse
////////////////////////////////////////////////////////////////////////////

void rat_state_eat_corpse::initialize	()
{
}

void rat_state_eat_corpse::execute		()
{
	if (!object().get_alife()) return;

	if (object().m_walk_on_way)
	{
		object().m_state_manager->push_state	(CAI_Rat::aiRatNoWay);
		return;
	}

	if (object().switch_if_enemy()&&object().switch_if_porsuit()||!object().switch_if_home())
	{
		object().m_state_manager->pop_state();
		return;
	}


	if (!object().switch_if_enemy()){
		if (!object().get_morale())
		{
			object().m_state_manager->pop_state();
			return;
		}
	}
	object().set_goal_time(10.f);
	if (object().switch_to_free_recoil())
	{
		object().m_state_manager->push_state(CAI_Rat::aiRatFreeRecoil);
		return;
	}
	object().activate_state_eat();
}

void rat_state_eat_corpse::finalize		()
{
	object().set_firing();
}

////////////////////////////////////////////////////////////////////////////
// rat_state_no_way
////////////////////////////////////////////////////////////////////////////

void rat_state_no_way::initialize		()
{
	object().time_old_attack = Device.dwTimeGlobal;
}

void rat_state_no_way::execute			()
{
	if (!object().get_alife()) return;

	if (!object().m_walk_on_way)
	{
		if (object().check_completion_no_way())
		{
			object().m_state_manager->push_state(CAI_Rat::aiRatAttackMelee);
			return;
		}
		object().set_rew_cur_position();
		object().set_sp_dir();
		object().activate_state_move();	
	} else {
		object().set_way_point();
		object().set_movement_type(!!object().m_bWayCanAdjustSpeed, !!object().m_bWayStraightForward);
		object().activate_move();
	}
}

void rat_state_no_way::finalize			()
{

}
