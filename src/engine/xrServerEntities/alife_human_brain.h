////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_brain.h
//	Created 	: 06.10.2005
//  Modified 	: 06.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human brain class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_monster_brain.h"

class CALifeHumanObjectHandler;
class CSE_ALifeHumanAbstract;

class CALifeHumanBrain : public CALifeMonsterBrain {
private:
	typedef CALifeMonsterBrain			inherited;

public:
	typedef CSE_ALifeHumanAbstract		object_type;
	typedef CALifeHumanObjectHandler	object_handler_type;

private:
	object_type							*m_object;
	object_handler_type					*m_object_handler;

// old not yet obsolete stuff
public:
	svector<char,5>						m_cpEquipmentPreferences;
	svector<char,4>						m_cpMainWeaponPreferences;

// old, to be obsolete
public:
	u32									m_dwTotalMoney;

public:
										CALifeHumanBrain	(object_type *object);
	virtual								~CALifeHumanBrain	();

public:
			void						on_state_write		(NET_Packet &packet);
			void						on_state_read		(NET_Packet &packet);

public:
	IC		object_type					&object				() const;
	IC		object_handler_type			&objects			() const;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CALifeHumanBrain)
#undef script_type_list
#define script_type_list save_type_list(CALifeHumanBrain)

#include "alife_human_brain_inline.h"