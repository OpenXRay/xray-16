#include "StdAfx.h"
#include "kills_store.h"
#include "Common/object_broker.h"

namespace award_system
{
kills_store::kills_store() {}
kills_store::~kills_store() {}
void kills_store::clear() { delete_data(m_kills); }
void kills_store::add_kill(shared_str const& killer, shared_str const& victim, u16 weapon_id, KILL_TYPE const kill_type,
    SPECIAL_KILL_TYPE const spec_kill_type)
{
    std::pair<shared_str, shared_str> search_key(killer, victim);
    kills_map_t::iterator tmp_iter = m_kills.find(search_key);
    if (tmp_iter == m_kills.end())
    {
        kills_t* new_kills = new kills_t();
        tmp_iter = m_kills.insert(std::make_pair(search_key, new_kills)).first;
    }

    kill new_kill;
    new_kill.m_kill_time = Device.dwTimeGlobal;
    new_kill.m_weapon_id = weapon_id;
    new_kill.m_kill_type = kill_type;
    new_kill.m_spec_kill_type = spec_kill_type;

    tmp_iter->second->push_obsolete(new_kill);
}

} // namespace award_system
