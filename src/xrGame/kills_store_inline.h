namespace award_system
{

template<typename Predicate>
u32	 const kills_store::fetch_kills(Predicate & predicate,
									buffer_vector<kill> & dest_kills)
{
	u32 ret_count = 0;
	for (kills_map_t::const_iterator i = m_kills.begin(),
		ie = m_kills.end(); i != ie; ++i)
	{
		for (kills_t::const_iterator ki = i->second->begin(),
			kie = i->second->end(); ki != kie; ++ki)
		{
			if (predicate(i->first.first, i->first.second, *ki))
			{
				++ret_count;
				if (dest_kills.capacity() == dest_kills.size())
					continue;
				
				dest_kills.push_back(*ki);
			}
		}
	}
	return ret_count;
}


}//namespace award_system