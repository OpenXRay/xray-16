////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_animation_selector.cpp
//	Created 	: 07.09.2007
//	Author		: Alexander Dudin
//	Description : Animation selector for smart covers
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_animation_selector.h"
#include "ai/stalker/ai_stalker.h"
#include "smart_cover_animation_planner.h"
#include "smart_cover_planner_actions.h"
#include "script_game_object.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "stalker_animation_manager.h"
#include "smart_cover_planner_actions.h"
#include "stalker_movement_manager_smart_cover.h"
#include "Inventory.h"
#include "smart_cover.h"
#include "HudItem.h"

float g_smart_cover_animation_speed_factor = 1.f;

using smart_cover::animation_selector;
using smart_cover::action_base;
using smart_cover::wait_after_exit;

animation_selector::animation_selector(CAI_Stalker* object)
    : m_object(object), m_callback_called(false), m_first_time(true), m_previous_time(flt_max)
{
    m_skeleton_animated = smart_cast<IKinematicsAnimated*>(object->Visual());
    VERIFY(m_skeleton_animated);
    m_planner = new animation_planner(object, "animation planner");
}

animation_selector::~animation_selector() { xr_delete(m_planner); }
void animation_selector::initialize()
{
    m_planner->initialize();

    // we need this, since we could change bone callbacks
    // which needs animation blends
    // but blends are not accessible since
    // we didn't called animation().update()
    // which is called on UpdateCL
    // and in case of RayQuery
    // our bone callback could be called
    // but UpdateCL is not called yet
    // and we will have
    // VERIFY	(*parameter->m_blend);
    m_callback_called = true;
    m_object->animation().update();

    m_first_time = true;
}

void animation_selector::finalize()
{
    if (!m_planner->initialized())
        return;

    m_planner->finalize();
}

action_base* animation_selector::current_operator() const
{
    return (&smart_cast<smart_cover::action_base&>(m_planner->current_action()));
}

MotionID animation_selector::select_animation(bool& animation_movement_controller)
{
    animation_movement_controller = true;

    if (m_callback_called)
    {
        if (m_planner->initialized())
        {
            current_operator()->on_animation_end();
            m_callback_called = false;

            m_previous_time = 0.f;
            if (!m_planner->initialized())
            {
                //				Msg				("%6d no planner update, planner is not initialized, exiting",
                // Device.dwTimeGlobal);
                return (m_object->animation().assign_global_animation(animation_movement_controller));
            }
        }

        //		Msg					("%6d updating planner", Device.dwTimeGlobal);
        m_planner->update();

        if (!m_planner->initialized())
        {
            //			Msg				("%6d planner is not initialized after update, exiting", Device.dwTimeGlobal);
            return (m_object->animation().assign_global_animation(animation_movement_controller));
        }

        current_operator()->on_no_mark();
        if (!current_operator()->is_animated_action())
            return (m_object->animation().assign_global_animation(animation_movement_controller));

        current_operator()->select_animation(m_animation);

        VERIFY(m_object->movement().current_params().cover());
        if (!m_object->movement().current_params().cover()->can_fire())
            return (m_skeleton_animated->ID_Cycle(m_animation.c_str()));

#if 0 // ndef MASTER_GOLD
		if (!psAI_Flags.test((u32)aiUseSmartCoversAnimationSlot))
			return			(m_skeleton_animated->ID_Cycle( m_animation.c_str()));

		VERIFY				( m_object->inventory().ActiveItem() );
		CHudItem* const		hud_item = smart_cast<CHudItem*>(m_object->inventory().ActiveItem());
		VERIFY				( hud_item );

		string16			animation_slot_string;
		R_ASSERT			( !_itoa_s( hud_item->animation_slot(), animation_slot_string, sizeof(animation_slot_string), 10 ) );

		LPSTR				result;
		STRCONCAT			( result, m_animation, "_slot_", animation_slot_string);

		MotionID			animation_id = m_skeleton_animated->ID_Cycle_Safe( result );
		if (animation_id)
			return			(animation_id);

		STRCONCAT			( result, m_animation, "_slot_2" );
		animation_id		= m_skeleton_animated->ID_Cycle_Safe( result );
		VERIFY				(animation_id);
		return				(animation_id);
#else // #ifndef MASTER_GOLD
        return (m_skeleton_animated->ID_Cycle(m_animation.c_str()));
#endif // #ifndef MASTER_GOLD
    }

    VERIFY(m_animation._get());
    //	VERIFY				(m_first_time || m_object->animation().global().blend());
    MotionID result = m_skeleton_animated->ID_Cycle(m_animation.c_str());
    if (m_first_time)
    {
        m_first_time = false;
        m_previous_time = 0.f;
        current_operator()->on_no_mark();
        return (result);
    }

    CBlend const* const blend = m_object->animation().global().blend();
    if (!blend)
    {
        m_previous_time = 0.f;
        current_operator()->on_no_mark();
        return (result);
    }

    VERIFY(blend->motionID == result);
    CMotionDef* motion_def = m_skeleton_animated->LL_GetMotionDef(result);

    typedef xr_vector<motion_marks> Marks;
    Marks const& marks = motion_def->marks;
    if (marks.size() < 3)
    {
        current_operator()->on_no_mark();
        return (result);
    }

    float previous_time = m_previous_time;
    float time_current = blend->timeCurrent + .1f;
    m_previous_time = time_current;
    // Slipch told me, that timeCurrent can decrease (setup to 0.f) during animation playing
    // therefore we need here to clamp previous value
    clamp(previous_time, 0.f, time_current);
    // first 2 should be footsteps
    if (!marks[2].is_mark_between(previous_time, time_current))
    {
        current_operator()->on_no_mark();
        return (result);
    }

    //	Msg					( "%d on_mark", Device.dwTimeGlobal );
    current_operator()->on_mark();
    return (result);
}

void animation_selector::on_animation_end()
{
    VERIFY(!m_callback_called);
    m_callback_called = true;
}

void animation_selector::modify_animation(CBlend* blend)
{
    if (!blend)
        return;

    CMotionDef* motion_def = m_skeleton_animated->LL_GetMotionDef(blend->motionID);
    VERIFY(motion_def);
    blend->speed = motion_def->Speed() * g_smart_cover_animation_speed_factor;
}

void animation_selector::save(NET_Packet& packet) { m_planner->save(packet); }
void animation_selector::load(IReader& packet) { m_planner->load(packet); }
void animation_selector::setup(CAI_Stalker* object, CPropertyStorage* storage) { m_planner->setup(object, storage); }
