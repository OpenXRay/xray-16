////////////////////////////////////////////////////////////////////////////
//	Module 		: agent_enemy_manager.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Agent enemy manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "member_enemy.h"

class CAgentManager;
class CMemberOrder;
class CEntityAlive;
class CAI_Stalker;

class CAgentEnemyManager {
public:
	typedef xr_vector<CMemberEnemy>					ENEMIES;
	typedef MemorySpace::squad_mask_type			squad_mask_type;
	typedef std::pair<ALife::_OBJECT_ID,bool>		WOUNDED;
	typedef std::pair<const CEntityAlive *,WOUNDED>	WOUNDED_ENEMY;
	typedef xr_vector<WOUNDED_ENEMY>				WOUNDED_ENEMIES;

private:
	CAgentManager			*m_object;
	ENEMIES					m_enemies;
	WOUNDED_ENEMIES			m_wounded;
	bool					m_only_wounded_left;
	bool					m_is_any_wounded;

protected:
	template <typename T>
	IC		void			setup_mask			(xr_vector<T> &objects, CMemberEnemy &enemy, const squad_mask_type &non_combat_members);
	IC		void			setup_mask			(CMemberEnemy &enemy, const squad_mask_type &non_combat_members);
			void			fill_enemies		();
			void			compute_enemy_danger();
			void			assign_enemies		();
			void			permutate_enemies	();
			void			assign_wounded		();
			void			assign_enemy_masks	();
			float			evaluate			(const CEntityAlive *object0, const CEntityAlive *object1) const;
			void			exchange_enemies	(CMemberOrder &member0, CMemberOrder &member1);
	IC		CAgentManager	&object				() const;

public:
	IC						CAgentEnemyManager	(CAgentManager *object);
			void			update				();
			void			distribute_enemies	();
	IC		ENEMIES			&enemies			();
			void			remove_links		(CObject *object);

private:
			void			wounded_processor	(const CEntityAlive *object, const ALife::_OBJECT_ID &wounded_processor_id);

public:
		ALife::_OBJECT_ID	wounded_processor	(const CEntityAlive *object);
			void			wounded_processed	(const CEntityAlive *object, bool value);
			bool			wounded_processed	(const CEntityAlive *object) const;
			bool			assigned_wounded	(const CEntityAlive *wounded, const CAI_Stalker *member);
			bool			useful_enemy		(const CEntityAlive *enemy, const CAI_Stalker *member) const;
};

#include "agent_enemy_manager_inline.h"