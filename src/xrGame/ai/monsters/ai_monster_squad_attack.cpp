#include "stdafx.h"
#include "ai_monster_squad.h"
#include "../../entity.h"
#include "../../entity_alive.h"
#include "../monsters/basemonster/base_monster.h "
#include "monster_home.h"

void CMonsterSquad::ProcessAttack()
{
	m_enemy_map.clear		();
	m_temp_entities.clear	();

	// Выделить элементы с общими врагами и состянием атаки 
	for (MEMBER_GOAL_MAP_IT it_goal = m_goals.begin(); it_goal != m_goals.end(); it_goal++) {
//		CEntity *member = it_goal->first;
		SMemberGoal goal = it_goal->second;

		if (goal.type == MG_AttackEnemy) {
			VERIFY(goal.entity && !goal.entity->getDestroy());

			ENEMY_MAP_IT it = m_enemy_map.find(goal.entity);
			if (it != m_enemy_map.end()) {
				it->second.push_back(it_goal->first);
			} else {
				m_temp_entities.push_back	(it_goal->first);
				m_enemy_map.insert			(mk_pair(goal.entity, m_temp_entities));
			}
		}
	}

	// Пройти по всем группам и назначить углы всем елементам в группе
	for ( ENEMY_MAP_IT it_enemy=m_enemy_map.begin(); it_enemy!=m_enemy_map.end(); ++it_enemy ) 
	{
		ENTITY_VEC* monsters = &(*it_enemy).second;
		if ( !monsters->size() )
		{
			continue;
		}

		Attack_AssignTargetDir(it_enemy->second, it_enemy->first);

		// a squad of CBaseMonster-s ? 
// 		if ( smart_cast<CBaseMonster*>(*(monsters->begin())) )
// 		{
// 			assign_monsters_target_dirs(it_enemy->second, it_enemy->first);
// 		}
// 		else
// 		{
// 			Attack_AssignTargetDir(it_enemy->second, it_enemy->first);
// 		}		
	}
}


struct sort_predicate {
	const CEntity *enemy;

			
			sort_predicate	(const CEntity *pEnemy) : enemy(pEnemy) {}

	bool	operator()		(const CEntity *pE1, const CEntity *pE2) const
	{
		return	(pE1->Position().distance_to(enemy->Position()) > 
			pE2->Position().distance_to(enemy->Position()));
	};
};

void CMonsterSquad::set_rat_squad_index(const CEntity *m_enemy)
{
	ENEMY_MAP		m_enemy_maps;
	ENTITY_VEC		m_entities;
	m_enemy_maps.clear		();
	m_entities.clear	();

	// Выделить элементы с общей целью

	for (MEMBER_GOAL_MAP_IT it_goal = m_goals.begin(); it_goal != m_goals.end(); it_goal++) {
		if (it_goal->first->g_Alive()) {
			ENEMY_MAP_IT it = m_enemy_maps.find(m_enemy);
			if (it != m_enemy_maps.end()) {
				it->second.push_back(it_goal->first);
			} else {
				m_entities.push_back	(it_goal->first);
				m_enemy_maps.insert		(mk_pair(m_enemy, m_entities));
			}
		}
	}

	for (ENEMY_MAP_IT it_enemy = m_enemy_maps.begin(); it_enemy != m_enemy_maps.end(); it_enemy++) {
		get_index_in_rat_squad(it_enemy->second,it_enemy->first);
	}
}

void CMonsterSquad::get_index_in_rat_squad(ENTITY_VEC &members, const CEntity *m_enemy)
{
	u8 m_index = 0;
	std::sort(members.begin(), members.end(), sort_predicate(m_enemy));
	while (!members.empty()) {
		CEntity *pEntity;
		m_index++;
		pEntity = members.back();
		pEntity->cast_entity_alive()->m_squad_index = m_index;
		members.pop_back();
	}
}


void CMonsterSquad::set_squad_index(const CEntity *m_enemy)
{
	ENEMY_MAP		m_enemy_maps;
	ENTITY_VEC		m_entities;
	m_enemy_maps.clear		();
	m_entities.clear	();

	// Выделить элементы с общей целью

	for (MEMBER_GOAL_MAP_IT it_goal = m_goals.begin(); it_goal != m_goals.end(); it_goal++) {
		if (it_goal->first->g_Alive()) {
			ENEMY_MAP_IT it = m_enemy_maps.find(m_enemy);
			if (it != m_enemy_maps.end()) {
				it->second.push_back(it_goal->first);
			} else {
				m_entities.push_back	(it_goal->first);
				m_enemy_maps.insert		(mk_pair(m_enemy, m_entities));
			}
		}
	}
	
	for (ENEMY_MAP_IT it_enemy = m_enemy_maps.begin(); it_enemy != m_enemy_maps.end(); it_enemy++) {
		get_index_in_squad(it_enemy->second,it_enemy->first);
	}
}

void CMonsterSquad::get_index_in_squad(ENTITY_VEC &members, const CEntity *m_enemy)
{
	u8 m_index = 0;
	std::sort(members.begin(), members.end(), sort_predicate(m_enemy));
	while (!members.empty()) {
		CEntity *pEntity;
		m_index++;
		pEntity = members.back();
		pEntity->cast_entity_alive()->m_squad_index = m_index;
		smart_cast<CBaseMonster&>(*pEntity).SetEnemy(smart_cast<const CEntityAlive*>(m_enemy));
		members.pop_back();
	}
}

void CMonsterSquad::Attack_AssignTargetDir(ENTITY_VEC &members, const CEntity *enemy)
{
	_elem	first;
	_elem	last;

	lines.clear();

	// сортировать по убыванию расстояния от npc до врага 
	std::sort(members.begin(), members.end(), sort_predicate(enemy));
	if (members.empty()) return;

	float delta_yaw = PI_MUL_2 / members.size();

	// обработать ближний элемент
	first.pE		= members.back();
	first.p_from	= first.pE->Position();
	first.yaw		= 0;
	members.pop_back();

	lines.push_back(first);

	// обработать дальний элемент
	if (!members.empty()) {
		last.pE			= members[0];
		last.p_from		= last.pE->Position();
		last.yaw		= PI;
		members.erase	(members.begin());

		lines.push_back(last);
	}

	Fvector target_pos = enemy->Position();
	float	next_right_yaw	= delta_yaw;
	float	next_left_yaw	= delta_yaw;

	// проходим с конца members в начало (начиная с наименьшего расстояния)
	while (!members.empty()) {
		CEntity *pCur;

		pCur = members.back();
		members.pop_back();

		_elem cur_line;
		cur_line.p_from		= pCur->Position();
		cur_line.pE			= pCur;

		// определить cur_line.yaw

		float h1,p1,h2,p2;
		Fvector dir;
		dir.sub(target_pos, first.p_from);
		dir.getHP(h1,p1);	
		dir.sub(target_pos, cur_line.p_from);
		dir.getHP(h2,p2);

		bool b_add_left = false;

		if (angle_normalize_signed(h2 - h1) > 0)  {		// right
			if ((next_right_yaw < PI) && !fsimilar(next_right_yaw, PI, PI/60.f)) b_add_left = false;
			else b_add_left = true;
		} else {										// left
			if ((next_left_yaw < PI) && !fsimilar(next_left_yaw, PI, PI/60.f)) b_add_left = true;
			else b_add_left = false;
		}

		if (b_add_left) {
			cur_line.yaw = -next_left_yaw;
			next_left_yaw += delta_yaw;
		} else {
			cur_line.yaw = next_right_yaw;
			next_right_yaw += delta_yaw;
		}

		lines.push_back(cur_line);
	}

	// Пройти по всем линиям и заполнить таргеты у npc
	float first_h, first_p;
	Fvector d; d.sub(target_pos,first.p_from);
	d.getHP(first_h, first_p);

	for (u32 i = 0; i < lines.size(); i++){
		SSquadCommand command;
		command.type			= SC_ATTACK;
		command.entity			= enemy;
		command.direction.setHP	(first_h + lines[i].yaw, first_p);
		UpdateCommand(lines[i].pE, command);
	}
}

Fvector   CMonsterSquad::calc_monster_target_dir (CBaseMonster* monster, const CEntity* enemy)
{
	VERIFY(monster);
	VERIFY(enemy);

	const Fvector enemy_pos = enemy->Position();
	Fvector home2enemy = enemy_pos;
	home2enemy.sub(monster->Home->get_home_point());

	const float home2enemy_mag = home2enemy.magnitude();

	// enemy pos == home pos?
	const float near_zero = 0.00001f;
	if ( home2enemy_mag < near_zero )
	{
		Fvector enemy2monster = monster->Position();
		enemy2monster.sub(enemy_pos);
		const float enemy2monster_mag = enemy2monster.magnitude();
		// monster pos == enemy pos?
		if ( enemy2monster_mag < near_zero )
		{
			VERIFY2(false, "Enemy and Monster should not have same pos!");
			Fvector dir = { 1.f, 0.f, 0.f }; // happy with random dir then :)
			return dir;
		}

		enemy2monster.normalize();
		return enemy2monster;
	}

	const u8 squad_size  = squad_alife_count();
	VERIFY(squad_size);

	u8 squad_index = get_index(monster);
	if ( squad_index == -1 )
	{
		squad_index = 0;
	}

	float heading, pitch;
	home2enemy.getHP(heading, pitch);

	// 2pi * index/num - encircle
	heading += M_PI * 2.f * squad_index / squad_size;
	heading = angle_normalize(heading);

	Fvector dir;
	dir.setHP(heading, pitch);
	dir.normalize();

	return dir;
}

void   CMonsterSquad::assign_monsters_target_dirs (ENTITY_VEC &members, const CEntity *enemy)
{
	for ( ENTITY_VEC_IT i=members.begin(), e=members.end(); i!=e; ++i )
	{
		CBaseMonster* monster = smart_cast<CBaseMonster*>(*i);
		SSquadCommand command;
		command.type      = SC_ATTACK;
		command.entity    = enemy;
		command.direction = calc_monster_target_dir(monster, enemy);
		UpdateCommand(monster, command);
	}
}
