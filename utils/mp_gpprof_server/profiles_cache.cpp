#include "profiles_cache.h"
#include <algorithm>
#include <assert.h>


extern unsigned int cache_expire_time;

profiles_cache::profiles_cache(unsigned int max_memory)
{
	m_items_allowed = max_memory / sizeof(cache_item);
	assert(m_items_allowed);
	m_cache.reserve(m_items_allowed);
}

profiles_cache::~profiles_cache()
{
}

bool profiles_cache::search(user_name_t const & profname,
							gamespy_profile::profile_data & dest_data)
{
	cache_item	search_item;
	strcpy(search_item.m_user_name, profname);
	cache_t::iterator tmp_iter = std::lower_bound(
		m_cache.begin(),
		m_cache.end(),
		search_item,
		cache_item_less_p());
	if (tmp_iter != m_cache.end())
	{
		if (strcmp(tmp_iter->m_user_name, profname))
			return false;

		if ((xray::get_clock_ms() - tmp_iter->m_creation_time) >= cache_expire_time)
		{
			m_cache.erase(tmp_iter);
			return false;
		}
		
		dest_data = tmp_iter->m_profile_data;
		++tmp_iter->m_hits_count;
		return true;
	}
	return false;
}

profiles_cache::cache_item::cache_item()
{
	strcpy(m_user_name, "");
	m_creation_time = 0;
	m_hits_count	= 0;
}

profiles_cache::cache_item::cache_item(cache_item const & copy)
{
	strcpy(m_user_name, copy.m_user_name);
	m_profile_data	= copy.m_profile_data;
	m_creation_time = copy.m_creation_time;
	m_hits_count	= copy.m_hits_count;
}

bool const profiles_cache::cache_item_less_p::operator()(
	cache_item const & left,
	cache_item const & right) const
{
	return strcmp(left.m_user_name, right.m_user_name) < 0;
};

bool profiles_cache::add(char const * profname,
						 gamespy_profile::profile_data const & src_data)
{
	if (!get_free_space())
	{
		return false;
	}
	cache_item	search_item;
	strcpy(search_item.m_user_name, profname);
	cache_t::iterator tmp_iter = std::lower_bound(
		m_cache.begin(),
		m_cache.end(),
		search_item,
		cache_item_less_p());
	if (tmp_iter != m_cache.end())
	{
		if (!strcmp(tmp_iter->m_user_name, profname))
		{
			tmp_iter->m_profile_data	= src_data;
			tmp_iter->m_creation_time	= xray::get_clock_ms();
			return true;
		}
	}

	cache_item	new_item;
	strcpy(new_item.m_user_name, profname);
	new_item.m_profile_data		= src_data;
	new_item.m_creation_time	= xray::get_clock_ms();
	m_cache.push_back(new_item);
	return true;
}

void profiles_cache::sort()
{
	std::sort(m_cache.begin(), m_cache.end(), cache_item_less_p());
}

profiles_cache::cache_t::size_type profiles_cache::get_free_space()
{
	return m_cache.capacity() - m_cache.size();
}

bool const profiles_cache::cache_item_expired_p::operator()(
	cache_item const & left) const
{
	if (!left.m_hits_count)
		return true;
	if ((m_current_time - left.m_creation_time) >= m_expire_time)
		return true;
	return false;	
}

void profiles_cache::clear_expired()
{
	cache_item_expired_p tmp_predicate;
	tmp_predicate.m_current_time	= xray::get_clock_ms();
	tmp_predicate.m_expire_time		= cache_expire_time;
	m_cache.erase(
		std::remove_if(
			m_cache.begin(),
			m_cache.end(),
			tmp_predicate),
		m_cache.end());
}