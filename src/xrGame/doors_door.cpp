////////////////////////////////////////////////////////////////////////////
//	Created		: 24.06.2009
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "doors_door.h"
#include "PhysicObject.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "script_game_object.h"
#include "doors.h"
#include "doors_actor.h"

using doors::door;
using doors::actor;
using doors::door_state;

static bool valid(door_state const state)
{
    return (state == doors::door_state_open) || (state == doors::door_state_closed);
}

door::door(CPhysicObject* object)
    : m_object(*object), m_state(door_state_open), m_previous_state(door_state_open), m_target_state(door_state_open),
      m_registered_position(object->Position()), m_locked(false)
{
    VERIFY(valid(m_state));
    VERIFY(valid(m_target_state));
    VERIFY(valid(m_previous_state));

    R_ASSERT(m_object.get_door_vectors(m_closed_vector, m_open_vector));

    Fmatrix invert;
    invert.invert(m_object.XFORM());
    invert.transform_dir(m_open_vector);
    invert.transform_dir(m_closed_vector);

    float const length = 1.1f;
    m_open_vector.mul(length);
    m_closed_vector.mul(length);

    m_object.spatial.type |= STYPE_VISIBLEFORAI;
}

door::~door()
{
    m_object.spatial.type &= ~STYPE_VISIBLEFORAI;

    if (m_initiators.empty())
        return;

    actors_type::const_iterator i = m_initiators.begin();
    actors_type::const_iterator const e = m_initiators.end();
    for (; i != e; ++i)
        (*i)->on_door_destroy(*this);

#ifdef DEBUG
    m_initiators.clear();
#endif // #ifdef DEBUG
}

Fvector const& door::position() const
{
    VERIFY(valid(m_state));
    VERIFY(valid(m_target_state));
    VERIFY(valid(m_previous_state));

    return m_registered_position;
}

Fmatrix const& door::get_matrix() const
{
    VERIFY(valid(m_state));
    VERIFY(valid(m_target_state));
    VERIFY(valid(m_previous_state));

    return m_object.XFORM();
}

Fvector const& door::get_vector(door_state const state) const
{
    VERIFY(valid(m_state));
    VERIFY(valid(m_target_state));
    VERIFY(valid(m_previous_state));

    if (state == door_state_open)
        return m_open_vector;
    else
        return m_closed_vector;
}

bool door::is_locked(door_state const state) const
{
    VERIFY(valid(state));
    VERIFY(valid(m_state));
    VERIFY(valid(m_target_state));
    VERIFY(valid(m_previous_state));

    if (m_state != state)
        return m_locked;

    return false;
}

bool door::is_blocked(door_state const state) const
{
    VERIFY(valid(m_state));
    VERIFY(valid(m_target_state));
    VERIFY(valid(m_previous_state));

    if (m_initiators.empty())
        return false;

    return (m_target_state != state);
}

#ifdef DEBUG
extern BOOL g_debug_doors;
#endif // #ifdef DEBUG

void door::lock()
{
    VERIFY(valid(m_state));
    VERIFY(valid(m_target_state));
    VERIFY(valid(m_previous_state));

    VERIFY(!m_locked);
    m_locked = true;
#ifdef DEBUG
    if (g_debug_doors)
        Msg("door[%s] is locked", m_object.cName().c_str());
#endif // #ifdef DEBUG
}

void door::unlock()
{
    VERIFY(valid(m_state));
    VERIFY(valid(m_target_state));
    VERIFY(valid(m_previous_state));

    VERIFY(m_locked);
    m_locked = false;
#ifdef DEBUG
    if (g_debug_doors)
        Msg("door[%s] is unlocked", m_object.cName().c_str());
#endif // #ifdef DEBUG
}

//Alundaio: Modified to pass the initiator to ph_door:use_callback
void door::change_state(actor* initiator)
{
    VERIFY(valid(m_state));
    VERIFY(valid(m_target_state));
    VERIFY(valid(m_previous_state));

    if (m_state == m_target_state)
        return;

    m_object.callback(GameObject::eUseObject)(m_object.lua_game_object(), initiator ? static_cast<CScriptGameObject*>(initiator->lua_game_object()) : nullptr);
#ifdef DEBUG
    if (g_debug_doors)
        Msg("door[%s] started to change its state to [%s]", m_object.cName().c_str(),
            m_target_state == door_state_open ? "open" : "closed");
#endif // #ifdef DEBUG
}
//Alundaio: END

void door::change_state(actor* const initiator, door_state const start_state, door_state const stop_state)
{
    VERIFY(valid(start_state));
    VERIFY(valid(stop_state));

    VERIFY(valid(m_state));
    VERIFY(valid(m_target_state));
    VERIFY(valid(m_previous_state));

    if (m_initiators.empty())
    {
        m_initiators.push_back(initiator);
        m_target_state = start_state;
#ifdef DEBUG
        if (g_debug_doors)
            Msg("door[%s] added initiator[%s] to keep door %s", m_object.cName().c_str(), initiator->get_name(),
                m_target_state == door_state_open ? "open" : "closed");
#endif // #ifdef DEBUG
        //		if ( !xr_strcmp( "sim_default_duty_28212", initiator->get_name()) ) {
        //			int i=0; (void)i;
        //		}
        change_state(initiator); //Alundaio: Pass the initiator! We need to know who is trying to open door!
        return;
    }

    if (m_target_state == start_state)
    {
#ifdef DEBUG
        if (g_debug_doors)
            Msg("door[%s] added initiator[%s] to keep door %s", m_object.cName().c_str(), initiator->get_name(),
                m_target_state == door_state_open ? "open" : "closed");
#endif // #ifdef DEBUG
        //		if ( !xr_strcmp( "sim_default_duty_28212", initiator->get_name()) ) {
        //			int i=0; (void)i;
        //		}
        VERIFY(std::find(m_initiators.begin(), m_initiators.end(), initiator) == m_initiators.end());
        m_initiators.push_back(initiator);
        return;
    }

    VERIFY(m_target_state == stop_state);
    actors_type::iterator const found = std::find(m_initiators.begin(), m_initiators.end(), initiator);
    VERIFY2(found != m_initiators.end(), make_string("cannot find initiator %s", initiator->get_name()));
    if (found != m_initiators.end())
    {
#ifdef DEBUG
        if (g_debug_doors)
            Msg("door[%s] removed initiator[%s] to keep door %s", m_object.cName().c_str(), initiator->get_name(),
                m_target_state == door_state_open ? "open" : "closed");
#endif // #ifdef DEBUG
        //		if ( !xr_strcmp( "sim_default_duty_28212", initiator->get_name()) ) {
        //			int i=0; (void)i;
        //		}
        m_initiators.erase(found);
    }

    if (!m_initiators.empty())
        return;

    if (m_previous_state != stop_state)
    {
        m_target_state = m_previous_state;
#ifdef DEBUG
        if (g_debug_doors)
            Msg("door[%s] restores its state to %s", m_object.cName().c_str(),
                m_target_state == door_state_open ? "open" : "closed");
#endif // #ifdef DEBUG
        //		if ( !xr_strcmp( "sim_default_duty_28212", initiator->get_name()) ) {
        //			int i=0; (void)i;
        //		}
        change_state(initiator); //Alundaio: Pass the initiator! We need to know who is trying to open door!
    }
    else
        VERIFY(m_previous_state == stop_state);
}

void door::change_state(actor* const initiator, doors::door_state const state)
{
    VERIFY(valid(state));
    VERIFY(valid(m_state));
    VERIFY(valid(m_target_state));
    VERIFY(valid(m_previous_state));

    change_state(initiator, state, state == door_state_open ? door_state_closed : door_state_open);
}

void door::on_change_state(door_state const state)
{
    VERIFY(valid(state));
    VERIFY(valid(m_state));
    VERIFY(valid(m_target_state));
    VERIFY(valid(m_previous_state));

    //	this could happen when doors hasn't been open but is used to be closed already or vice versa
    //	VERIFY						( m_state == door_state_open );
    m_state = state;

    if (m_initiators.empty())
    {
        m_previous_state = state;
        return;
    }

    change_state(nullptr); //Alundaio: NULL - no need to know who
}

#ifdef DEBUG
LPCSTR door::get_name() const { return m_object.cName().c_str(); }
shared_str door::get_initiators_ids() const
{
    u32 buffer_size = 1;
    actors_type::const_iterator i = m_initiators.begin();
    actors_type::const_iterator const e = m_initiators.end();
    for (; i != e; ++i)
        buffer_size += xr_strlen((*i)->get_name()) + 2;

    LPSTR const result = (LPSTR)_alloca(buffer_size);
    *result = 0;
    LPSTR j = result;
    u32 left_size = buffer_size;
    for (i = m_initiators.begin(); i != e; ++i)
    {
        u32 const length = xr_strlen((*i)->get_name());
        memcpy(j, (*i)->get_name(), length);
        j += length;
        *j++ = ',';
        *j++ = ' ';
        left_size = buffer_size - (j - result);
    }

    if (buffer_size > 1)
        result[buffer_size - 2] = 0;

    return result;
}

bool door::check_initiator(actor const* const initiator) const
{
    return std::find(m_initiators.begin(), m_initiators.end(), initiator) != m_initiators.end();
}
#endif // #ifdef DEBUG
