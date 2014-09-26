#ifndef KILLS_STORE_INCLUDED
#define KILLS_STORE_INCLUDED

#include "obsolete_queue.h"
#include "game_base_kill_type.h"
#include "../xrServerEntities/associative_vector.h"

namespace award_system
{

class kills_store
{
public:
				kills_store		();
				~kills_store	();
	
	void		clear			();

	struct kill
	{
		u32					m_kill_time;
		u16					m_weapon_id;
		KILL_TYPE			m_kill_type;
		SPECIAL_KILL_TYPE	m_spec_kill_type;
	}; //struct kill

	static unsigned int const max_kills_count = 10;
	typedef obsolete_queue<buffer_vector<kill>, max_kills_count>				kills_t;
	//key: (initiator, victim)
	typedef associative_vector<std::pair<shared_str, shared_str>, kills_t*>		kills_map_t;

	void		add_kill		(shared_str const & killer,
								 shared_str const & victim,
								 u16 weapon_id,
								 KILL_TYPE const kill_type,
								 SPECIAL_KILL_TYPE const spec_kill_type);

	template<typename Predicate>
	u32	 const fetch_kills		(Predicate & predicate,
								 buffer_vector<kill> & dest_kills);
private:
	kills_map_t				m_kills;
};//class kills_store

}//namespace award_system

#include "kills_store_inline.h"

#endif //#ifndef KILLS_STORE_INCLUDED