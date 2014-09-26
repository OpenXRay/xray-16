////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_movement_manager.h
//	Created 	: 31.10.2005
//  Modified 	: 22.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster movement manager class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

class CMovementManagerHolder;
class CALifeMonsterDetailPathManager;
class CALifeMonsterPatrolPathManager;

namespace MovementManager {
	enum EPathType;
};

class CALifeMonsterMovementManager {
public:
	typedef CMovementManagerHolder							object_type;
	typedef CALifeMonsterDetailPathManager					detail_path_type;
	typedef CALifeMonsterPatrolPathManager					patrol_path_type;
	typedef MovementManager::EPathType						EPathType;

private:
	object_type					*m_object;
	detail_path_type			*m_detail;
	patrol_path_type			*m_patrol;
	EPathType					m_path_type;

public:
								CALifeMonsterMovementManager	(object_type *object);
								~CALifeMonsterMovementManager	();
	IC		object_type			&object						() const;
	IC		detail_path_type	&detail						() const;
	IC		patrol_path_type	&patrol						() const;
	IC		const EPathType		&path_type					() const;

public:
			void				update						();
			void				on_switch_online			();
			void				on_switch_offline			();
	IC		void				path_type					(const EPathType &path_type);

public:
			bool				completed					() const;
			bool				actual						() const;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CALifeMonsterMovementManager)
#undef script_type_list
#define script_type_list save_type_list(CALifeMonsterMovementManager)

#include "alife_monster_movement_manager_inline.h"