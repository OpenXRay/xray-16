#ifndef HITS_STORE_INCLUDED
#define HITS_STORE_INCLUDED

#include "obsolete_queue.h"
#include "../xrServerEntities/associative_vector.h"

namespace award_system
{

class hits_store
{
public:
				hits_store		();
				~hits_store		();

	void		clear			();

	struct bullet_hit
	{
		u32		m_hit_time;
		float	m_dist;
		u16		m_weapon_id;
		u16		m_bone_id;
	}; //struct bullet_hit
	
	static unsigned int const max_hits_count	= 10;
	typedef obsolete_queue<buffer_vector<bullet_hit>, max_hits_count>				bullet_hits_t;
	//key: (initiator, victim)
	typedef associative_vector<std::pair<shared_str, shared_str>, bullet_hits_t*>	bullet_hits_map_t;

	void		add_hit			(shared_str const & hitter,
								 shared_str const & victim,
								 u16 weapon_id,
								 u16 bone_id,
								 float bullet_fly_dist);

	template<typename Predicate>
	u32	const	fetch_hits		(Predicate & predicate,
								 buffer_vector<bullet_hit> & dest_hits) const;
private:
	bullet_hits_map_t	m_bullet_hits;
};//class hits_store

} //namespace award_system

#include "hits_store_inline.h"

#endif //#ifndef HITS_STORE_INCLUDED