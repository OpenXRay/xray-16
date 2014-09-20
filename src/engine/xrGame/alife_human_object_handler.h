////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_object_handler.h
//	Created 	: 07.10.2005
//  Modified 	: 07.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human object handler class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_space.h"

class CSE_ALifeItemWeapon;
class CSE_ALifeInventoryItem;
class CSE_ALifeGroupAbstract;
class CSE_ALifeHumanAbstract;

class CALifeHumanObjectHandler {
public:
	typedef CSE_ALifeHumanAbstract	object_type;

private:
	object_type						*m_object;

public:
	IC								CALifeHumanObjectHandler	(object_type *object);
	IC		object_type				&object						() const;

public:
			u16						get_available_ammo_count	(const CSE_ALifeItemWeapon *weapon, ALife::OBJECT_VECTOR &objects);
			u16						get_available_ammo_count	(const CSE_ALifeItemWeapon *weapon, ALife::ITEM_P_VECTOR &items, ALife::OBJECT_VECTOR *objects = 0);
			void					attach_available_ammo		(CSE_ALifeItemWeapon *weapon, ALife::ITEM_P_VECTOR	&items, ALife::OBJECT_VECTOR *objects = 0);
			bool					can_take_item				(CSE_ALifeInventoryItem	*inventory_item);
			void					collect_ammo_boxes			();

public:
			void					detach_all					(bool fictitious);
			void					update_weapon_ammo			();
			void					process_items				();
			CSE_ALifeDynamicObject	*best_detector				();
			CSE_ALifeItemWeapon		*best_weapon				();

public:
			int						choose_equipment			(ALife::OBJECT_VECTOR *objects = 0);
			int						choose_weapon				(const ALife::EWeaponPriorityType &weapon_priority_type, ALife::OBJECT_VECTOR *objects = 0);
			int						choose_food					(ALife::OBJECT_VECTOR *objects = 0);
			int						choose_medikit				(ALife::OBJECT_VECTOR *objects = 0);
			int						choose_detector				(ALife::OBJECT_VECTOR *objects = 0);
			int						choose_valuables			();
			bool					choose_fast					();
			void					choose_group				(CSE_ALifeGroupAbstract *group_abstract);
			void					attach_items				();
};

#include "alife_human_object_handler_inline.h"