////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_animation_global.cpp
//	Created 	: 25.02.2003
//  Modified 	: 19.11.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker animation manager : global animations
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "stalker_animation_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "inventory.h"
#include "fooditem.h"
#include "property_storage.h"
#include "stalker_movement_manager_smart_cover.h"
#include "ai/stalker/ai_stalker_space.h"
#include "stalker_animation_data.h"
#include "weapon.h"
#include "missile.h"
#include "stalker_animation_manager_impl.h"

using namespace StalkerSpace;

void CStalkerAnimationManager::global_play_callback			(CBlend *blend)
{
	CAI_Stalker					*object = (CAI_Stalker*)blend->CallbackParam;
	VERIFY						(object);

	CStalkerAnimationManager	&manager = object->animation();
	CStalkerAnimationPair		&pair = manager.global();
	pair.on_animation_end		();

//	std::pair<LPCSTR,LPCSTR>	pair_id = smart_cast<IKinematicsAnimated*>(object->Visual())->LL_MotionDefName_dbg(blend->motionID);
//	Msg							("[%6d] global callback [%s][%s]", Device.dwTimeGlobal, pair_id.first, pair_id.second);

	if (!manager.m_global_callback)
		return;

	manager.m_call_global_callback	= true;
}

MotionID CStalkerAnimationManager::global_critical_hit		()
{
	if (!object().critically_wounded())
		return					(MotionID());

	if (global().animation())
		return					(global().animation());

	CWeapon						*weapon = smart_cast<CWeapon*>(object().inventory().ActiveItem());
	VERIFY2						(
		weapon,
		make_string(
			"current active item: %s",
			object().inventory().ActiveItem() ? 
			*object().inventory().ActiveItem()->object().cName() : 
			"no active item"
		)
	);

	u32							animation_slot = weapon->animation_slot();
	VERIFY						(animation_slot >= 1);
	VERIFY						(animation_slot <= 3);

	return						(
		global().select(
			m_data_storage->m_part_animations.A[
				eBodyStateStand
			].m_global.A[
				object().critical_wound_type() + 6*(animation_slot - 1)
			].A,
			&object().critical_wound_weights()
		)
	);
}

MotionID CStalkerAnimationManager::assign_global_animation	(bool &animation_movement_controller)
{
	if (m_global_selector)
		return					(m_global_selector(animation_movement_controller));

	animation_movement_controller	= false;

	if (eMentalStatePanic != object().movement().mental_state())
		return					(global_critical_hit());

	if (fis_zero(object().movement().speed(object().character_physics_support()->movement())))
		return					(MotionID());

	return						(
		global().select(
			m_data_storage->m_part_animations.A[
				body_state()
			].m_global.A[
				1
			].A
		)
	);
}
