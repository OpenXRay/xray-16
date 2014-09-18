////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_human_object_handler.cpp
//	Created 	: 07.10.2005
//  Modified 	: 07.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife human object handler class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "alife_human_object_handler.h"
#include "xrServer_Objects_ALife_Monsters.h"

u16	 CALifeHumanObjectHandler::get_available_ammo_count			(const CSE_ALifeItemWeapon *weapon, ALife::OBJECT_VECTOR &objects)
{
	return	(0);
}

u16	 CALifeHumanObjectHandler::get_available_ammo_count			(const CSE_ALifeItemWeapon *weapon, ALife::ITEM_P_VECTOR &items, ALife::OBJECT_VECTOR *objects)
{
	return	(0);
}

void CALifeHumanObjectHandler::attach_available_ammo			(CSE_ALifeItemWeapon *weapon, ALife::ITEM_P_VECTOR &items, ALife::OBJECT_VECTOR *objects)
{
}

bool CALifeHumanObjectHandler::can_take_item					(CSE_ALifeInventoryItem	*inventory_item)
{
	return	(false);
}

void CALifeHumanObjectHandler::collect_ammo_boxes				()
{
}

int	 CALifeHumanObjectHandler::choose_equipment					(ALife::OBJECT_VECTOR *objects)
{
	return	(-1);
}

int	 CALifeHumanObjectHandler::choose_weapon					(const ALife::EWeaponPriorityType &weapon_priority_type, ALife::OBJECT_VECTOR *objects)
{
	return	(-1);
}

int	 CALifeHumanObjectHandler::choose_food						(ALife::OBJECT_VECTOR *objects)
{
	return	(-1);
}

int	 CALifeHumanObjectHandler::choose_medikit					(ALife::OBJECT_VECTOR *objects)
{
	return	(-1);
}

int	 CALifeHumanObjectHandler::choose_detector					(ALife::OBJECT_VECTOR *objects)
{
	return	(-1);
}

int	 CALifeHumanObjectHandler::choose_valuables					()
{
	return	(-1);
}

bool CALifeHumanObjectHandler::choose_fast						()
{
	return	(false);
}

void CALifeHumanObjectHandler::choose_group						(CSE_ALifeGroupAbstract *group_abstract)
{
}


void CALifeHumanObjectHandler::detach_all						(bool fictitious)
{
}

void CALifeHumanObjectHandler::update_weapon_ammo				()
{
}

void CALifeHumanObjectHandler::process_items					()
{
}

CSE_ALifeDynamicObject *CALifeHumanObjectHandler::best_detector	()
{
	return	(0);
}

CSE_ALifeItemWeapon *CALifeHumanObjectHandler::best_weapon		()
{
	return	(0);
}

void CALifeHumanObjectHandler::attach_items						()
{
}
