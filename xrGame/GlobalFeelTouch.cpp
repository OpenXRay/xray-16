#include "stdafx.h"
#include "GlobalFeelTouch.hpp"
#include <functional>

GlobalFeelTouch::GlobalFeelTouch()
{
}

GlobalFeelTouch::~GlobalFeelTouch()
{
}

struct delete_predicate_by_time : public std::binary_function<Feel::Touch::DenyTouch, DWORD, bool>
{
	bool operator () (Feel::Touch::DenyTouch const & left, DWORD const expire_time) const
	{
		if (left.Expire <= expire_time)
			return true;
		return false;
	};
};
struct objects_ptrs_equal : public std::binary_function<Feel::Touch::DenyTouch, CObject const *, bool>
{
	bool operator() (Feel::Touch::DenyTouch const & left, CObject const * const right) const
	{
		if (left.O == right)
			return true;
		return false;
	}
};

void GlobalFeelTouch::feel_touch_update(Fvector& P, float R)
{
	//we ignore P and R arguments, we need just delete evaled denied objects...
	xr_vector<Feel::Touch::DenyTouch>::iterator new_end = 
		std::remove_if(feel_touch_disable.begin(), feel_touch_disable.end(), 
			std::bind2nd(delete_predicate_by_time(), Device.dwTimeGlobal));
	feel_touch_disable.erase(new_end, feel_touch_disable.end());
}

bool GlobalFeelTouch::is_object_denied(CObject const * O)
{
	/*Fvector temp_vector;
	feel_touch_update(temp_vector, 0.f);*/
	if (std::find_if(feel_touch_disable.begin(), feel_touch_disable.end(),
		std::bind2nd(objects_ptrs_equal(), O)) == feel_touch_disable.end())
	{
		return false;
	}
	return true;
}