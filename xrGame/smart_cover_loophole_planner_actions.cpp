////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_loophole_planner_actions.cpp
//	Created 		: 04.09.2007
//	Author		: Alexander Dudin
//	Description 	: Smart cover loophole planner action classes
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_loophole_planner_actions.h"
#include "script_game_object.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/stalker/ai_stalker_impl.h"
#include "ai/stalker/ai_stalker_space.h"
#include "smart_cover_loophole.h"
#include "smart_cover_action.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "ai_monster_space.h"
#include "stalker_decision_space.h"
#include "object_handler_space.h"
#include "object_handler_planner.h"
#include "smart_cover_animation_planner.h"
#include "property_storage.h"
#include "stalker_movement_manager_smart_cover.h"
#include "sight_manager.h"
#include "debug_renderer.h"
#include "smart_cover.h"
#include "smart_cover_description.h"
#include "sight_manager_space.h"
#include "stalker_animation_manager.h"
#include "stalker_animation_data.h"
#include "inventory_item.h"
#include "Weapon.h"
#include "animation_movement_controller.h"
#include "visual_memory_manager.h"

using smart_cover::loophole_action_base;
using smart_cover::loophole_action;
using smart_cover::loophole_action_no_sight;
using smart_cover::loophole_lookout;
using smart_cover::loophole_fire;
using smart_cover::loophole_reload;
using smart_cover::transition;
using smart_cover::idle_2_fire_transition;
using smart_cover::fire_2_idle_transition;
using smart_cover::idle_2_lookout_transition;
using smart_cover::lookout_2_idle_transition;
using smart_cover::animation_planner;
using namespace MonsterSpace;
using namespace StalkerDecisionSpace;
using namespace ObjectHandlerSpace;

loophole_action_base::loophole_action_base	(CAI_Stalker *object, LPCSTR action_name) :
	inherited	(object, action_name)
{
}

Fvector loophole_action_base::nearest_loophole_direction(Fvector const& position) const
{
	stalker_movement_manager_smart_cover&	movement = object().movement();
	
	VERIFY						(movement.current_params().cover());
	VERIFY						(movement.current_params().cover_loophole());

	smart_cover::cover const&	cover = *movement.current_params().cover();
	smart_cover::loophole const&loophole = *movement.current_params().cover_loophole();
	Fvector						fov_direction = cover.fov_direction(loophole);
	float						half_fov = .5f*loophole.fov();
	float						h, p;
	fov_direction.getHP			(h, p);
	Fvector						fov_direction_left =  Fvector().setHP(h - half_fov, p);
	Fvector						fov_direction_right = Fvector().setHP(h + half_fov, p);

	Fvector						fov_position = cover.fov_position(loophole);
	Fvector						direction = Fvector().sub(position, fov_position);
	direction.normalize			();

	if (fov_direction_left.dotproduct(direction) > fov_direction_right.dotproduct(direction))
		return					(fov_direction_left);

	return						(fov_direction_right);
}

void loophole_action_base::process_fire_position(bool const& change_sight)
{
	VERIFY						(object().movement().current_params().cover_fire_position());

	Fvector const&				position = *object().movement().current_params().cover_fire_position();
	if (!object().movement().in_current_loophole_fov(position)) {
		object().sight().setup	(
			CSightAction(
				SightManager::eSightTypeDirection,
				nearest_loophole_direction(*object().movement().current_params().cover_fire_position()),
				true
			)
		);

#pragma todo("insert here loophole selection")
		return;
	}

	object().sight().setup		(CSightAction(SightManager::eSightTypePosition, *object().movement().current_params().cover_fire_position(), true));
	object().sight().update		();

	if (!change_sight)
		return;

	object().sight().Exec_Look	(0.f);
}

void loophole_action_base::process_fire_object	(bool const& change_sight)
{
	VERIFY						(object().movement().current_params().cover_fire_object());

	CGameObject const*			fire_object = object().movement().current_params().cover_fire_object();
	if (object().movement().in_current_loophole_fov(fire_object->Position())) {
		object().sight().setup	( CSightAction( fire_object, true, true));
		object().sight().update	();

		if (!change_sight)
			return;

		object().sight().Exec_Look	(0.f);
		return;
	}

	object().sight().setup		(
		CSightAction(
			SightManager::eSightTypeDirection,
			nearest_loophole_direction(fire_object->Position()),
			true
		)
	);

	object().sight().update		();

	if (!change_sight)
		return;

	object().sight().Exec_Look	(0.f);

#pragma todo("insert here loophole selection")
}

void loophole_action_base::process_default	(bool const& change_sight)
{
	stalker_movement_manager_smart_cover&	movement = object().movement();
	
	VERIFY						(movement.current_params().cover());
	VERIFY						(movement.current_params().cover_loophole());

//	smart_cover::cover const&	cover = *movement.current_params().cover();
//	smart_cover::loophole const&loophole = *movement.current_params().cover_loophole();
	object().sight().setup		(
		CSightAction(
			SightManager::eSightTypeAnimationDirection,
			true,
			false
		)
	);
	object().sight().update		();

	if (!change_sight)
		return;

	object().sight().Exec_Look	(0.f);

	if (object().movement().current_params().cover() != object().movement().target_params().cover())
		return;

	if (object().movement().target_params().cover_loophole())
		return;

#pragma todo("insert here loophole selection")
}

bool loophole_action_base::enemy_in_fov		() const
{
	CEntityAlive const*			enemy = object().memory().enemy().selected();
	if (!enemy)
		return					(false);

	if (!object().movement().in_current_loophole_fov(enemy->Position()))
		return					(false);

	return						(true);
}

bool loophole_action_base::process_enemy	(bool const& change_sight)
{
	CEntityAlive const*				enemy = object().memory().enemy().selected();
	VERIFY							(enemy);

	if (enemy_in_fov()) {
		if (object().memory().visual().visible_now(enemy))
			object().sight().setup	(CSightAction(enemy, true, true));
		else
			object().sight().setup	(
				CSightAction(
					SightManager::eSightTypePosition,
					object().memory().memory_position(enemy),
					true
				)
			);

		object().sight().update		();
		if (!change_sight)
			return					(true);

		object().sight().Exec_Look	(0.f);
		return						(true);
	}
	
	object().sight().setup	(
		CSightAction(
			SightManager::eSightTypeDirection,
			nearest_loophole_direction(enemy->Position()),
			true
		)
	);
	object().sight().update			();

	if (!change_sight)
		return						(true);

	object().sight().Exec_Look		(0.f);
	return							(true);
}

bool loophole_action_base::setup_sight		(bool const& change_sight)
{
	if (object().movement().current_params().cover_fire_position()) {
		process_fire_position			(change_sight);
		return							(true);
	}

	if (object().movement().current_params().cover_fire_object()) {
		process_fire_object				(change_sight);
		return							(true);
	}

	if (!object().memory().enemy().selected()) {
		process_default					(change_sight);
		return							(true);
	}

	return								(process_enemy(change_sight));
}

//////////////////////////////////////////////////////////////////////////
// loophole_action
//////////////////////////////////////////////////////////////////////////

loophole_action::loophole_action		(CAI_Stalker *object, LPCSTR action_name) :
	inherited							(object, action_name),
	m_action_id							(action_name)
{

}

void loophole_action::initialize		()
{
	inherited::initialize		();

	LPCSTR animation_id			= "idle";
	typedef smart_cover::loophole::Animations ActionAnimations;
	ActionAnimations const		&animations = object().movement().current_params().cover_loophole()->action_animations(m_action_id, animation_id);
	m_animation					= animations[m_random.randI(animations.size())];
}

void loophole_action::execute			()
{
	inherited::execute			();
}

void loophole_action::finalize			()
{
	inherited::finalize			();
}

void loophole_action::select_animation	(shared_str &result)
{
	result = m_animation;
}

void loophole_action::on_animation_end	()
{
}

//////////////////////////////////////////////////////////////////////////
// loophole_action_no_sight
//////////////////////////////////////////////////////////////////////////

loophole_action_no_sight::loophole_action_no_sight	(CAI_Stalker *object, LPCSTR action_name) :
	inherited	(object, action_name)
{
}

void loophole_action_no_sight::initialize		()
{
	inherited::initialize						();

	object().sight().setup						(CSightAction(SightManager::eSightTypeAnimationDirection, true, false));
}

void loophole_action_no_sight::finalize			()
{
	inherited::finalize							();
}

//////////////////////////////////////////////////////////////////////////
// loophole_reload
//////////////////////////////////////////////////////////////////////////

loophole_reload::loophole_reload		(CAI_Stalker *object, LPCSTR action_name) :
	inherited							(object, action_name)
{
}

void loophole_reload::select_animation	(shared_str &result)
{
	inherited::select_animation	(result);
	object().set_goal			(eObjectActionAimForceFull1,object().best_weapon());
}

//////////////////////////////////////////////////////////////////////////
// transition
//////////////////////////////////////////////////////////////////////////

transition::transition					(CAI_Stalker *object, LPCSTR action_name, LPCSTR action_from, LPCSTR action_to, StalkerDecisionSpace::EWorldProperties state_from, StalkerDecisionSpace::EWorldProperties state_to, animation_planner *planner) :
	inherited					(object, action_name),
	m_action_from				(action_from),
	m_action_to					(action_to),
	m_state_from				(state_from),
	m_state_to					(state_to),
	m_planner					(planner)
{
}

void transition::initialize				()
{
	inherited::initialize		();

	typedef smart_cover::loophole::TransitionData TransitionData;
	TransitionData const		&animations = object().movement().current_params().cover_loophole()->transition_animations(m_action_from, m_action_to);
	m_animation					= animations[m_random.randI(animations.size())];
}

void transition::finalize				()
{
	inherited::finalize			();
}

void transition::select_animation		(shared_str &result)
{
	result = m_animation;
}

void transition::on_animation_end		()
{
	m_planner->m_storage.set_property(m_state_from, false);
	m_planner->m_storage.set_property(m_state_to, true);

	m_planner->last_transition_time(Device.dwTimeGlobal);
}

//////////////////////////////////////////////////////////////////////////
// loophole_lookout
//////////////////////////////////////////////////////////////////////////

loophole_lookout::loophole_lookout		(CAI_Stalker *object, LPCSTR action_name) :
	inherited							(object, action_name)
{
}

void loophole_lookout::initialize		()
{
	inherited::initialize		();

	object().sight().bone_aiming(m_animation, CSightManager::animation_frame_start, CSightManager::aiming_head);
}

void loophole_lookout::execute			()
{
	inherited::execute			();

	setup_sight					(false);
}

void loophole_lookout::finalize			()
{
	object().sight().bone_aiming();

	inherited::finalize			();
}

//////////////////////////////////////////////////////////////////////////
// loophole_fire
//////////////////////////////////////////////////////////////////////////

loophole_fire::loophole_fire			(CAI_Stalker *object, LPCSTR action_name) :
	inherited							(object, action_name)
{
}

void loophole_fire::initialize			()
{
	inherited::initialize		();

	m_firing					= true;

	object().sight().bone_aiming(m_animation, CSightManager::animation_frame_start, CSightManager::aiming_weapon);
}

void loophole_fire::execute				()
{
	inherited::execute			();

	LPCSTR						animation_id = "idle";
	if	(
			object().sight().current_action().target_reached() &&
			m_firing &&
			(
				!object().movement().check_can_kill_enemy() ||
				object().fire_make_sense()
			)
		)
		animation_id			= "shoot";
	else
		m_firing				= false;

	typedef smart_cover::loophole::Animations Animations;
	Animations const			&animations = object().movement().current_params().cover_loophole()->action_animations(m_action_id, animation_id);
	m_animation					= animations[m_random.randI(animations.size())];

	setup_sight					(false);
}

void loophole_fire::finalize			()
{
	object().sight().bone_aiming();

	inherited::finalize			();
}

void loophole_fire::select_animation	(shared_str &result)
{
	inherited::select_animation	(result);
}

void loophole_fire::on_animation_end	()
{
	inherited::on_animation_end	();

	m_firing					= !m_firing;
}

void loophole_fire::on_mark				()
{
	CWeapon *best_weapon		= smart_cast<CWeapon *>(object().best_weapon());
	if (!best_weapon)
		return;

	u32 const magazine_size		= best_weapon->GetAmmoMagSize();
//	Msg							( "started firing: %d", magazine_size );
	object().set_goal			(eObjectActionFireNoReload,object().best_weapon(), magazine_size, magazine_size);
}

void loophole_fire::on_no_mark			()
{
	VERIFY						(object().movement().current_params().cover());
	if (!object().movement().current_params().cover()->can_fire())
		return;

	object().set_goal			(eObjectActionIdle,object().best_weapon(), 1, 3);
}

//////////////////////////////////////////////////////////////////////////
// idle_2_fire_transition
//////////////////////////////////////////////////////////////////////////

idle_2_fire_transition::idle_2_fire_transition	(CAI_Stalker *object, LPCSTR action_name, LPCSTR action_from, LPCSTR action_to, StalkerDecisionSpace::EWorldProperties state_from, StalkerDecisionSpace::EWorldProperties state_to, animation_planner *planner, bool const& use_weapon) :
	inherited	(object, action_name, action_from, action_to, state_from, state_to, planner)
{
}

void idle_2_fire_transition::initialize			()
{
	inherited::initialize	();

	object().animation().remove_bone_callbacks		();
	object().animation().assign_bone_blend_callbacks(true);
	object().sight().bone_aiming					(m_animation, CSightManager::animation_frame_end, CSightManager::aiming_weapon);
	setup_sight										(true);
//	object().sight().enable							(false);
}

void idle_2_fire_transition::finalize			()
{
	object().sight().bone_aiming					();
	object().animation().remove_bone_callbacks		();
	object().animation().assign_bone_callbacks		();

	inherited::finalize		();
}

//////////////////////////////////////////////////////////////////////////
// fire_2_idle_transition
//////////////////////////////////////////////////////////////////////////

fire_2_idle_transition::fire_2_idle_transition	(CAI_Stalker *object, LPCSTR action_name, LPCSTR action_from, LPCSTR action_to, StalkerDecisionSpace::EWorldProperties state_from, StalkerDecisionSpace::EWorldProperties state_to, animation_planner *planner) :
	inherited	(object, action_name, action_from, action_to, state_from, state_to, planner)
{
}

void fire_2_idle_transition::initialize			()
{
	inherited::initialize							();

	object().animation().remove_bone_callbacks		();
	object().animation().assign_bone_blend_callbacks(false);
	object().sight().bone_aiming					(m_animation, CSightManager::animation_frame_start, CSightManager::aiming_weapon);
	setup_sight										(true);
	object().sight().enable							(false);
}

void fire_2_idle_transition::finalize			()
{
	object().sight().bone_aiming					();
	object().animation().remove_bone_callbacks		();
	object().animation().assign_bone_callbacks		();
	object().sight().enable							(true);

	inherited::finalize								();
}

//////////////////////////////////////////////////////////////////////////
// idle_2_lookout_transition
//////////////////////////////////////////////////////////////////////////

idle_2_lookout_transition::idle_2_lookout_transition(CAI_Stalker *object, LPCSTR action_name, LPCSTR action_from, LPCSTR action_to, StalkerDecisionSpace::EWorldProperties state_from, StalkerDecisionSpace::EWorldProperties state_to, animation_planner *planner) :
	inherited	(object, action_name, action_from, action_to, state_from, state_to, planner)
{
}

void idle_2_lookout_transition::initialize			()
{
	inherited::initialize	();

	object().animation().remove_bone_callbacks		();
	object().animation().assign_bone_blend_callbacks(true);
	object().sight().bone_aiming					(m_animation, CSightManager::animation_frame_end, CSightManager::aiming_head);
	setup_sight										(true);
//	object().sight().enable							(false);
}

void idle_2_lookout_transition::finalize			()
{
	object().sight().bone_aiming					();
	object().animation().remove_bone_callbacks		();
	object().animation().assign_bone_callbacks		();

	inherited::finalize		();
}

//////////////////////////////////////////////////////////////////////////
// lookout_2_idle_transition
//////////////////////////////////////////////////////////////////////////

lookout_2_idle_transition::lookout_2_idle_transition(CAI_Stalker *object, LPCSTR action_name, LPCSTR action_from, LPCSTR action_to, StalkerDecisionSpace::EWorldProperties state_from, StalkerDecisionSpace::EWorldProperties state_to, animation_planner *planner) :
	inherited	(object, action_name, action_from, action_to, state_from, state_to, planner)
{
}

void lookout_2_idle_transition::initialize			()
{
	inherited::initialize							();

	object().animation().remove_bone_callbacks		();
	object().animation().assign_bone_blend_callbacks(false);
	object().sight().bone_aiming					(m_animation, CSightManager::animation_frame_start, CSightManager::aiming_head);
	setup_sight										(true);
	object().sight().enable							(false);
}

void lookout_2_idle_transition::finalize			()
{
	object().sight().bone_aiming					();
	object().animation().remove_bone_callbacks		();
	object().animation().assign_bone_callbacks		();
	object().sight().enable							(true);

	inherited::finalize								();
}
