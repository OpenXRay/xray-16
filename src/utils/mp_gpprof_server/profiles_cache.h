#ifndef PROFILES_CACHE_INCLUDED
#define PROFILES_CACHE_INCLUDED

#include <vector>
#include "profile_data_types.h"
#include "profile_request.h"
#include "threads.h"



class profiles_cache
{
public:
			profiles_cache	(unsigned int max_memory);
			~profiles_cache	();
	bool	search			(user_name_t const & profname,
							 gamespy_profile::profile_data & dest_data);
	bool	add				(char const * profname,
							 gamespy_profile::profile_data const & src_data);
	void	clear_expired	();
	void	sort			();
private:
	profiles_cache			() {};
	struct cache_item
	{
		cache_item						();
		cache_item						(cache_item const & copy);
		user_name_t						m_user_name;
		gamespy_profile::profile_data	m_profile_data;
		unsigned int					m_creation_time;
		unsigned int					m_hits_count;
	};
	struct cache_item_less_p
	{
		bool const operator()	(cache_item const & left,
								 cache_item const & right) const;
	};//struct cache_item_less_predicate
	struct cache_item_expired_p
	{
		cache_item_expired_p	() {};
		cache_item_expired_p	(cache_item_expired_p const & copy) :
			m_current_time(copy.m_current_time),
			m_expire_time(copy.m_expire_time)
		{
		}
		unsigned int			m_current_time;
		unsigned int			m_expire_time;
		bool const operator()	(cache_item const & left) const;
	};
	typedef std::vector<cache_item>		cache_t;
	
	cache_t					m_cache;
	cache_t::size_type		m_items_allowed;

	cache_t::size_type		get_free_space	();
};//class profiles_cache

#endif //#ifndef PROFILES_CACHE_INCLUDED