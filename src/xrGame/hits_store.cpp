#include "StdAfx.h"
#include "hits_store.h"
#include "Common/object_broker.h"

namespace award_system
{
hits_store::hits_store() {}
hits_store::~hits_store() { clear(); }
void hits_store::clear() { delete_data(m_bullet_hits); }
void hits_store::add_hit(
    shared_str const& hitter, shared_str const& victim, u16 weapon_id, u16 bone_id, float bullet_fly_dist)
{
    std::pair<shared_str, shared_str> search_key(hitter, victim);
    bullet_hits_map_t::iterator tmp_iter = m_bullet_hits.find(search_key);

    if (tmp_iter == m_bullet_hits.end())
    {
        bullet_hits_t* new_hits_queue = new bullet_hits_t();
        tmp_iter = m_bullet_hits.insert(std::make_pair(search_key, new_hits_queue)).first;
    }

    bullet_hit tmp_hit;
    tmp_hit.m_hit_time = Device.dwTimeGlobal;
    tmp_hit.m_dist = bullet_fly_dist;
    tmp_hit.m_weapon_id = weapon_id;
    tmp_hit.m_bone_id = bone_id;

    tmp_iter->second->push_obsolete(tmp_hit);
}

} // namespace award_system
