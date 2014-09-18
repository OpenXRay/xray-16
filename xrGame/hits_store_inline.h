namespace award_system
{

template<typename Predicate>
u32 const hits_store::fetch_hits(Predicate & predicate,
								 buffer_vector<bullet_hit> & dest_hits) const
{
	u32	ret_count = 0;
	for (bullet_hits_map_t::const_iterator i = m_bullet_hits.begin(),
		ie = m_bullet_hits.end(); i != ie; ++i)
	{
		for (bullet_hits_t::const_iterator hi = i->second->begin(),
			hie = i->second->end(); hi != hie; ++hi)
		{
			if (predicate(i->first.first, i->first.second, *hi))
			{
				++ret_count;
				if (dest_hits.capacity() == dest_hits.size())
					continue;

				dest_hits.push_back(*hi);
			}
		}
	}
	return ret_count;
}

}//namespace award_system