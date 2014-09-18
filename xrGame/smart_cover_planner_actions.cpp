////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_planner_actions.cpp
//	Created 	: 04.09.2007
//	Author		: Alexander Dudin
//	Description : Smart cover planner action classes
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_planner_actions.h"
#include "script_game_object.h"
#include "ai/stalker/ai_stalker.h"
#include "ai/stalker/ai_stalker_impl.h"
#include "ai/stalker/ai_stalker_space.h"
#include "stalker_movement_manager_smart_cover.h"
#include "sight_manager.h"
#include "stalker_movement_manager_smart_cover.h"
#include "movement_manager_space.h"
#include "sight_manager_space.h"
#include "smart_cover.h"
#include "stalker_planner.h"
#include "memory_manager.h"
#include "memory_space.h"
#include "enemy_manager.h"
#include "smart_cover_description.h"
#include "stalker_animation_manager.h"
#include "stalker_decision_space.h"
#include "ai_monster_space.h"
#include "smart_cover_animation_planner.h"
#include "smart_cover_transition.hpp"
#include "smart_cover_transition_animation.hpp"
#include "animation_movement_controller.h"

using smart_cover::action_base;
using smart_cover::change_loophole;
using smart_cover::animation_planner;
using smart_cover::non_animated_change_loophole;
using smart_cover::exit;
using smart_cover::transitions::action;
using smart_cover::transitions::animation_action;
using namespace MonsterSpace;
using namespace StalkerDecisionSpace;

namespace smart_cover {
	shared_str	transform_vertex(shared_str const &vertex_id, bool const &in);
} // namespace smart_cover

//////////////////////////////////////////////////////////////////////////
// action_base
//////////////////////////////////////////////////////////////////////////

action_base::action_base			(CAI_Stalker *object, LPCSTR action_name) :
	inherited						(object, action_name)
{

}

void action_base::on_mark			()
{
}

void action_base::on_no_mark		()
{
}

bool action_base::is_animated_action()
{
	return					(true);
}

void action_base::setup_orientation	()
{
//	VERIFY										(!object().sight().enabled());
	object().sight().enable						(true);
	object().animation().assign_bone_callbacks	();
}

//////////////////////////////////////////////////////////////////////////
// change_loophole
//////////////////////////////////////////////////////////////////////////

change_loophole::change_loophole	(CAI_Stalker *object, LPCSTR action_name) :
	inherited						(object, action_name)
{
}

void change_loophole::initialize		()
{
	inherited::initialize			();
	object().sight().enable			(false);
}

void change_loophole::execute		()
{
	inherited::execute				();
}

void change_loophole::finalize		()
{
	inherited::finalize				();
	object().sight().enable			(true);
}

void change_loophole::select_animation	(shared_str &result)
{
	if (!object().movement().exit_transition()) {
		result						= object().movement().current_transition().animation().animation_id();
		return;
	}

	smart_cover::transitions::animation_action const& animation = object().movement().current_transition().animation(object().movement().target_body_state());
	VERIFY							(object().movement().current_params().cover());
	smart_cover::cover const&		cover = *object().movement().current_params().cover();
	shared_str const&				cover_loophole_id = object().movement().current_params().cover_loophole_id();

	VERIFY2							(
		cover.description()->transitions().edge(cover_loophole_id, smart_cover::transform_vertex("", false)),
		make_string(
			"current loophole_id[%s], next_loophole_id[%s]",
			cover_loophole_id.c_str(),
			smart_cover::transform_vertex("", false).c_str()
		)
	);
	VERIFY2							(
		animation.has_animation(),
		make_string(
			"cover[%s], transition[%s][%s] has no animation",
			cover.id().c_str(),
			cover_loophole_id.c_str(),
			smart_cover::transform_vertex("", false).c_str()
		)
	);

	result							= animation.animation_id();
}

void change_loophole::on_animation_end	()
{
	stalker_movement_manager_smart_cover			&movement = object().movement();
	movement.go_next_loophole		();
}

//////////////////////////////////////////////////////////////////////////
// non_animated_change_loophole
//////////////////////////////////////////////////////////////////////////

non_animated_change_loophole::non_animated_change_loophole(CAI_Stalker *object, LPCSTR action_name) :
	inherited					(object, action_name)
{

}

void non_animated_change_loophole::initialize			()
{
	inherited::initialize		();

	object().sight().enable		(false); // to force adjust_orientation
	setup_orientation			();

	object().movement().set_movement_type	(eMovementTypeRun);

	object().movement().start_non_animated_loophole_change	();
}

void non_animated_change_loophole::execute				()
{
	inherited::execute			();
}

void non_animated_change_loophole::finalize			()
{
	object().movement().stop_non_animated_loophole_change	();
	inherited::finalize			();
}

bool  non_animated_change_loophole::is_animated_action	()
{
	return						(false);
}

void non_animated_change_loophole::select_animation	(shared_str &result)
{

}

void non_animated_change_loophole::on_animation_end	()
{

}

//////////////////////////////////////////////////////////////////////////
// exit
//////////////////////////////////////////////////////////////////////////

exit::exit(CAI_Stalker *object, LPCSTR action_name) :
	inherited					(object, action_name)
{

}

void exit::initialize			()
{
	inherited::initialize		();

	if (!object().movement().current_transition().animation().has_animation())
		return;

	object().sight().enable		(false);
}

void exit::execute				()
{
	inherited::execute			();

	if (object().movement().current_transition().animation().has_animation())
		return;

	setup_orientation										();
	object().movement().go_next_loophole					();
	object().movement().set_movement_type					(eMovementTypeRun);
//	object().movement().start_non_animated_loophole_change	();
}

void exit::finalize				()
{
//	object().movement().stop_non_animated_loophole_change	();
	inherited::finalize			();
}

bool exit::is_animated_action	()
{
	return						(object().movement().current_transition().animation().has_animation());
}

void exit::select_animation		(shared_str& result)
{
	VERIFY						(object().movement().current_transition().animation().has_animation());
	result						= object().movement().current_transition().animation().animation_id();
}

void exit::on_animation_end	()
{
	setup_orientation						();
	object().movement().go_next_loophole	();
	object().movement().set_movement_type	(eMovementTypeRun);
}