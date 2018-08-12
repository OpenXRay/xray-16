////////////////////////////////////////////////////////////////////////////
//	Created		: 24.06.2009
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "doors_actor.h"
#include "doors_door.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_movement_manager_smart_cover.h"
#include "debug_renderer.h"
#include "script_game_object.h" //Alundaio

using doors::actor;
using doors::door;
using doors::doors_type;
using doors::door_state;

float const doors::g_door_length = 1.1f;
float const doors::g_door_open_time = 1.4f;

actor::actor(CAI_Stalker const& object) : m_object(object) {}
actor::~actor()
{
    revert_states(m_open_doors, door_state_closed);
    revert_states(m_closed_doors, door_state_open);
}

static void on_door_destroy(doors_type& doors, door& door)
{
    doors_type::iterator const found = std::find(doors.begin(), doors.end(), &door);
    if (found != doors.end())
        doors.erase(found);
}

void actor::on_door_destroy(door& door)
{
    ::on_door_destroy(m_open_doors, door);
    ::on_door_destroy(m_closed_doors, door);
}

pcstr actor::get_name() const { return m_object.cName().c_str(); }
void actor::revert_states(doors_type& doors, door_state const state)
{
    doors_type::const_iterator i = doors.begin();
    doors_type::const_iterator const e = doors.end();
    for (; i != e; ++i)
    {
        (*i)->change_state(this, state);
#ifdef DEBUG
        VERIFY(!(*i)->check_initiator(this));
#endif // #ifdef DEBUG
    }

    doors.clear();
}

//Alundaio: add the ability to get lua game object
CScriptGameObject* actor::lua_game_object() const
{
    return m_object.lua_game_object();
}
//Alundaio: END

Fvector const& actor::get_position() const { return m_object.Position(); }
bool actor::need_update() const { return !m_open_doors.empty() && !m_closed_doors.empty(); }
class passed_doors_predicate
{
public:
    inline passed_doors_predicate(stalker_movement_manager_smart_cover const& movement, actor& actor,
        door_state const state, float const danger_distance)
        : m_movement(&movement), m_actor(&actor), m_state(state), m_danger_distance(danger_distance)
    {
    }

    inline bool operator()(door* const door) const
    {
        if (door->is_locked(m_state))
            return false;

        Fmatrix matrix = door->get_matrix();
        Fvector const open = door->get_vector(doors::door_state_open);
        Fvector const closed = door->get_vector(doors::door_state_closed);
        Fvector const diagonal = Fvector(open).sub(closed);

        float distance_to_open_state = m_movement->is_going_through(matrix, open, m_danger_distance);
        if (distance_to_open_state != -1.f)
            return false;

        float distance_to_closed_state = m_movement->is_going_through(matrix, closed, m_danger_distance);
        if (distance_to_closed_state != -1.f)
            return false;

        Fvector temp;
        matrix.transform_tiny(temp, closed);
        matrix.c = temp;
        float distance_to_diagonal = m_movement->is_going_through(matrix, diagonal, m_danger_distance);
        if (distance_to_diagonal != -1.f)
            return false;

        door->change_state(m_actor, m_state);
        return true;
    }

private:
    stalker_movement_manager_smart_cover const* m_movement;
    actor* m_actor;
    door_state m_state;
    float m_danger_distance;
}; // class passed_doors_predicate

void actor::process_doors(float const average_speed, doors_type& processed_doors, temp_doors_type const& new_doors,
    door_state const start_state, door_state const stop_state)
{
    stalker_movement_manager_smart_cover const& movement = m_object.movement();

    float const danger_distance = average_speed * g_door_open_time;
    processed_doors.erase(std::remove_if(processed_doors.begin(), processed_doors.end(),
                              passed_doors_predicate(movement, *this, stop_state, danger_distance)),
        processed_doors.end());

    temp_doors_type::const_iterator i = new_doors.begin();
    temp_doors_type::const_iterator const e = new_doors.end();
    for (; i != e; ++i)
        (*i)->change_state(this, start_state);

    u32 const processed_count = processed_doors.size();
    processed_doors.insert(processed_doors.end(), new_doors.begin(), new_doors.end());
    std::inplace_merge(processed_doors.begin(), processed_doors.begin() + processed_count, processed_doors.end());
}

#ifdef DEBUG
BOOL g_debug_doors = 1;
#endif // #ifdef DEBUG

bool actor::add_new_door(float const average_speed, door* const door, doors_type const& processed_doors,
    doors_type& locked_doors, temp_doors_type& new_doors, door_state const state)
{
    if (door->is_locked(state))
    {
#ifdef DEBUG
        if (g_debug_doors)
            Msg("actor[%s] is waiting for the locked door[%s]", get_name(), door->get_name());
#endif // #ifdef DEBUG
        return false;
    }

    if (door->is_blocked(state))
    {
        doors_type::iterator const i = std::find(locked_doors.begin(), locked_doors.end(), door);
        if (i == locked_doors.end())
        {
#ifdef DEBUG
            if (g_debug_doors)
                Msg("actor[%s] is waiting for the door[%s] blocked by %s", get_name(), door->get_name(),
                    door->get_initiators_ids().c_str());
#endif // #ifdef DEBUG
            return false;
        }

        VERIFY(std::find(processed_doors.begin(), processed_doors.end(), door) == processed_doors.end());
        locked_doors.erase(i);
        door->change_state(this, state);
        if (door->is_blocked(state))
        {
#ifdef DEBUG
            if (g_debug_doors)
                Msg("actor[%s] is waiting for the door[%s] blocked by %s", get_name(), door->get_name(),
                    door->get_initiators_ids().c_str());
#endif // #ifdef DEBUG
            return false;
        }
    }

    if (std::find(processed_doors.begin(), processed_doors.end(), door) != processed_doors.end())
        return true;

    float const danger_distance = average_speed * g_door_open_time;
    Fvector const open = door->get_vector(doors::door_state_open);
    Fvector const closed = door->get_vector(doors::door_state_closed);
    Fvector const diagonal = Fvector(open).sub(closed);

    Fmatrix matrix = door->get_matrix();
    stalker_movement_manager_smart_cover const& movement = m_object.movement();

    float distance_to_open_state = movement.is_going_through(matrix, open, danger_distance);
    distance_to_open_state = distance_to_open_state == -1.f ? flt_max : distance_to_open_state;

    float distance_to_closed_state = movement.is_going_through(matrix, closed, danger_distance);
    distance_to_closed_state = distance_to_closed_state == -1.f ? flt_max : distance_to_closed_state;

    Fvector temp;
    matrix.transform_tiny(temp, closed);
    matrix.c = temp;
    float distance_to_diagonal = movement.is_going_through(matrix, diagonal, danger_distance);
    distance_to_diagonal = distance_to_diagonal == -1.f ? flt_max : distance_to_diagonal;

    float const min_distance =
        std::min(distance_to_diagonal, std::min(distance_to_open_state, distance_to_closed_state));
    if (min_distance > danger_distance)
        return true;

    new_doors.push_back(door);
    return true;
}

bool actor::update_doors(doors_type const& detected_doors, float const average_speed)
{
//	if ( !xr_strcmp( "sim_default_duty_28212", get_name()) ) {
//		int i=0; (void)i;
//	}

#ifdef DEBUG
    m_detected_doors = detected_doors;
#endif // #ifdef DEBUG

    stalker_movement_manager_smart_cover const& movement = m_object.movement();

    u32 const detected_doors_count = detected_doors.size();
    temp_doors_type new_doors_to_open(
        _alloca(detected_doors_count * sizeof(doors_type::value_type)), detected_doors_count);
    temp_doors_type new_doors_to_close(
        _alloca(detected_doors_count * sizeof(doors_type::value_type)), detected_doors_count);

    float const check_distance = average_speed * g_door_open_time + g_door_length;

    doors_type::const_iterator i = detected_doors.begin();
    doors_type::const_iterator e = detected_doors.end();
    for (; i != e; ++i)
    {
        Fmatrix const& matrix = (*i)->get_matrix();
        float const distance_to_open =
            movement.is_going_through(matrix, (*i)->get_vector(door_state_open), check_distance);
        float const distance_to_closed =
            movement.is_going_through(matrix, (*i)->get_vector(door_state_closed), check_distance);
        if (distance_to_open >= 0.f)
        {
            if (distance_to_closed >= 0.f)
                if (distance_to_open < distance_to_closed)
                {
                    if (!add_new_door(
                            average_speed, *i, m_open_doors, m_closed_doors, new_doors_to_open, door_state_open))
                        return false;
                }
                else
                {
                    if (!add_new_door(
                            average_speed, *i, m_closed_doors, m_open_doors, new_doors_to_close, door_state_closed))
                        return false;
                }
            else
            {
                if (!add_new_door(
                        average_speed, *i, m_closed_doors, m_open_doors, new_doors_to_close, door_state_closed))
                    return false;
            }

            continue;
        }

        if (distance_to_closed < 0.f)
            continue;

        if (!add_new_door(average_speed, *i, m_open_doors, m_closed_doors, new_doors_to_open, door_state_open))
            return false;
    }

    process_doors(average_speed, m_open_doors, new_doors_to_open, door_state_open, door_state_closed);
    process_doors(average_speed, m_closed_doors, new_doors_to_close, door_state_closed, door_state_open);

    return true;
}

#ifdef DEBUG
void actor::render() const
{
    CDebugRenderer& renderer = Level().debug_renderer();
    doors_type::const_iterator i = m_detected_doors.begin();
    doors_type::const_iterator e = m_detected_doors.end();
    for (; i != e; ++i)
    {
        Fmatrix const& matrix = (*i)->get_matrix();
        Fvector temp;
        matrix.transform_tiny(temp, (*i)->get_vector(door_state_open));
        renderer.draw_line(Fidentity, matrix.c, temp, color_xrgb(0, 255, 0));

        matrix.transform_tiny(temp, (*i)->get_vector(door_state_closed));
        renderer.draw_line(Fidentity, matrix.c, temp, color_xrgb(255, 0, 0));
    }
}
#endif // #ifdef DEBUG
