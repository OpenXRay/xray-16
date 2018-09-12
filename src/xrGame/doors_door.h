////////////////////////////////////////////////////////////////////////////
//	Created		: 24.06.2009
//	Author		: Dmitriy Iassenev
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef DOORS_DOOR_H_INCLUDED
#define DOORS_DOOR_H_INCLUDED

#include "Common/Noncopyable.hpp"

class CPhysicObject;

namespace doors
{
class actor;
enum door_state : int;

class door : private Noncopyable
{
public:
    door(CPhysicObject* object);
    ~door();
    void change_state(actor* initiator, door_state state);
    void on_change_state(door_state const state);
#ifdef DEBUG
    LPCSTR get_name() const;
    shared_str get_initiators_ids() const;
    bool check_initiator(actor const* initiator) const;
#endif // #ifdef DEBUG

    void lock();
    void unlock();

    Fvector const& position() const;
    Fmatrix const& get_matrix() const;
    Fvector const& get_vector(door_state state) const;
    bool is_locked(door_state state) const;
    bool is_blocked(door_state state) const;

private:
    void change_state(actor* initiator, door_state start_state, door_state stop_state);
    void change_state(actor* initiator); //Alundaio: Pass the initiator

private:
    typedef xr_vector<actor*> actors_type;

private:
    actors_type m_initiators;
    Fvector m_open_vector;
    Fvector m_closed_vector;
    CPhysicObject& m_object;
    door_state m_state;
    door_state m_previous_state;
    door_state m_target_state;
    Fvector m_registered_position;
    bool m_locked;
}; // class door

} // namespace doors

#endif // #ifndef DOORS_DOOR_H_INCLUDED
