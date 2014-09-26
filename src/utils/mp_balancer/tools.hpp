#ifndef TOOLS_HPP
#define TOOLS_HPP

inline u32 get_string_collection		(shared_str const & src, xr_vector<shared_str> & dest_collection)
{
	u32 cnt			= _GetItemCount(src.c_str());
	string1024		_one;
	
	for(u32 c=0; c<cnt; ++c)
	{
		_GetItem					(src.c_str(), c, _one);
		dest_collection.push_back	(_one);
	}
	return cnt;
}


inline void get_string_from_collection	(xr_vector<shared_str> const & src_collection, xr_string & dest_string)
{
	xr_vector<shared_str>::const_iterator ie = src_collection.end();
	for (xr_vector<shared_str>::const_iterator i = src_collection.begin(); i != ie; ++i)
	{
		dest_string.append(i->c_str());
		if ((i + 1) != ie)
			dest_string.append(",");
	}
}


#endif //#ifndef TOOLS_HPP