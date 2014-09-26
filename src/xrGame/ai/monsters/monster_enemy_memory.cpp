#include "pch_script.h"
#include "monster_enemy_memory.h"
#include "BaseMonster/base_monster.h"
#include "../../memory_manager.h"
#include "../../visual_memory_manager.h"
#include "../../enemy_manager.h"
#include "../../ai_object_location.h"
#include "monster_home.h"
#include "Dog/dog.h"
#include "ai_monster_squad.h"
#include "ai_monster_squad_manager.h"
#include "../../Actor.h"
#include "../../actor_memory.h"

CMonsterEnemyMemory::CMonsterEnemyMemory()
{
	monster			= 0;
	time_memory		= 15000; 
}

CMonsterEnemyMemory::~CMonsterEnemyMemory()
{
}

void CMonsterEnemyMemory::init_external(CBaseMonster *M, TTime mem_time) 
{
	monster = M; 
	time_memory = mem_time;
}

extern CActor*	g_actor;

void CMonsterEnemyMemory::update() 
{
	VERIFY		(monster->g_Alive());

	CMonsterHitMemory& monster_hit_memory = monster->HitMemory;

	typedef CObjectManager<const CEntityAlive>::OBJECTS	objects_list;

	objects_list const& objects	=	monster->memory().enemy().objects();

	if ( monster_hit_memory.is_hit() && time() < monster_hit_memory.get_last_hit_time() + 1000 )
	{
		if ( CEntityAlive* enemy = smart_cast<CEntityAlive*>(monster->HitMemory.get_last_hit_object()) )
		{
			if ( monster->CCustomMonster::useful(&monster->memory().enemy(), enemy) && 
				 monster->Position().distance_to(enemy->Position()) 
				                        < 
				 monster->get_feel_enemy_who_just_hit_max_distance() )
			{
				add_enemy					(enemy);

				bool const self_is_dog	=	!!smart_cast<const CAI_Dog*>(monster);
				if ( self_is_dog )
				{
					CMonsterSquad* const squad	=	monster_squad().get_squad(monster);
					squad->set_home_in_danger	();
				}
			}			
		}
	}

	if ( monster->SoundMemory.IsRememberSound() )
	{
		SoundElem sound;
		bool dangerous;
		monster->SoundMemory.GetSound(sound, dangerous);
		if ( dangerous && Device.dwTimeGlobal < sound.time + 2000 )
		{
			if ( CEntityAlive const* enemy = smart_cast<CEntityAlive const*>(sound.who) )
			{
				float const xz_dist	=	monster->Position().distance_to_xz(g_actor->Position());
				float const y_dist	=	_abs(monster->Position().y - g_actor->Position().y);

				if ( monster->CCustomMonster::useful(&monster->memory().enemy(), enemy) && 
					 y_dist < 10 &&
					 xz_dist < monster->get_feel_enemy_who_made_sound_max_distance() &&
					 g_actor->memory().visual().visible_now(monster)	)
				{
					add_enemy					(enemy);

					bool const self_is_dog	=	!!smart_cast<const CAI_Dog*>(monster);
					if ( self_is_dog )
					{
						CMonsterSquad* const squad	=	monster_squad().get_squad(monster);
						squad->set_home_in_danger	();
					}
				}
			}
		}
	}

	for ( objects_list::const_iterator	I	=	objects.begin();
										I	!=	objects.end(); 
									  ++I	) 
	{
		const CEntityAlive* enemy = *I;
		const bool feel_enemy	  = monster->Position().distance_to(enemy->Position()) 
													< 
									monster->get_feel_enemy_max_distance();

		if ( feel_enemy || monster->memory().visual().visible_now(*I) )
			add_enemy(*I);
	}

	float const feel_enemy_max_distance	=	monster->get_feel_enemy_max_distance();
	if ( g_actor )
	{
		float const xz_dist	=	monster->Position().distance_to_xz(g_actor->Position());
		float const y_dist	=	_abs(monster->Position().y - g_actor->Position().y);

		if ( xz_dist < feel_enemy_max_distance && 
			 y_dist < 10 &&
			 monster->memory().enemy().is_useful(g_actor) &&
			 g_actor->memory().visual().visible_now(monster) )
		{
			add_enemy(g_actor);
		}
	}
	
	// удалить устаревших врагов
	remove_non_actual();

	// обновить опасность 
	for (ENEMIES_MAP_IT it = m_objects.begin(); it != m_objects.end(); it++) {
		u8		relation_value = u8(monster->tfGetRelationType(it->first));
		float	dist = monster->Position().distance_to(it->second.position);
		it->second.danger = (1 + relation_value*relation_value*relation_value) / (1 + dist);
	}
}

void CMonsterEnemyMemory::add_enemy(const CEntityAlive *enemy)
{
	SMonsterEnemy enemy_info;
	enemy_info.position = enemy->Position();
	enemy_info.vertex   = enemy->ai_location().level_vertex_id();
	enemy_info.time		= Device.dwTimeGlobal;
	enemy_info.danger	= 0.f;

	ENEMIES_MAP_IT it = m_objects.find(enemy);
	if (it != m_objects.end()) {
		// обновить данные о враге
		it->second = enemy_info;
	} else {
		// добавить врага в список объектов
		m_objects.insert(mk_pair(enemy, enemy_info));
	}
}

void CMonsterEnemyMemory::add_enemy(const CEntityAlive *enemy, const Fvector &pos, u32 vertex, u32 time)
{
	SMonsterEnemy enemy_info;
	enemy_info.position = pos;
	enemy_info.vertex   = vertex;
	enemy_info.time		= time;
	enemy_info.danger	= 0.f;

	ENEMIES_MAP_IT it = m_objects.find(enemy);
	if (it != m_objects.end()) {
		// обновить данные о враге
		if (it->second.time < enemy_info.time) it->second = enemy_info;
	} else {
		// добавить врага в список объектов
		m_objects.insert(mk_pair(enemy, enemy_info));
	}
}

void CMonsterEnemyMemory::remove_non_actual() 
{
	TTime cur_time = Device.dwTimeGlobal;

	// удалить 'старых' врагов и тех, расстояние до которых > 30м и др.
	for ( ENEMIES_MAP_IT	it	=	m_objects.begin(), nit; 
							it	!=	m_objects.end(); 
							it	=	nit	)
	{
		nit = it; ++nit;
		// проверить условия удаления
		if ( !it->first									|| 
			 !it->first->g_Alive()						|| 
			 it->first->getDestroy()					||
			 (it->second.time + time_memory < cur_time) ||
			 (it->first->g_Team() == monster->g_Team()) ||
			 !monster->memory().enemy().is_useful(it->first) ) 
		{
			m_objects.erase (it);
		}
	}
}

const CEntityAlive *CMonsterEnemyMemory::get_enemy()
{
	ENEMIES_MAP_IT	it = find_best_enemy();
	if (it != m_objects.end()) return it->first;
	return (0);
}

SMonsterEnemy CMonsterEnemyMemory::get_enemy_info()
{
	SMonsterEnemy ret_val;
	ret_val.time = 0;

	ENEMIES_MAP_IT	it = find_best_enemy();
	if (it != m_objects.end()) ret_val = it->second;

	return ret_val;
}

ENEMIES_MAP_IT CMonsterEnemyMemory::find_best_enemy()
{
	ENEMIES_MAP_IT	it = m_objects.end();
	float			max_value = 0.f;

	// find best at home first
	for (ENEMIES_MAP_IT I = m_objects.begin(); I != m_objects.end(); I++) {
		if (!monster->Home->at_home(I->second.position)) continue;
		if (I->second.danger > max_value) {
			max_value = I->second.danger;
			it = I;
		}
	}

	// there is no best enemies at home
	if (it == m_objects.end()) {
		// find any
		max_value = 0.f;
		for (ENEMIES_MAP_IT I = m_objects.begin(); I != m_objects.end(); I++) {
			if (I->second.danger > max_value) {
				max_value = I->second.danger;
				it = I;
			}
		}
	}

	return it;
}

void CMonsterEnemyMemory::remove_links(CObject *O)
{
	if ( monster )
	{
		monster->EnemyMan.remove_links(O);
	}

	for (ENEMIES_MAP_IT	I = m_objects.begin();I!=m_objects.end();++I) {
		if ((*I).first == O) {
			m_objects.erase(I);
			break;
		}
	}
}

