#include "stdafx.h"
#include "base_color.h"

bool	base_color::				similar	( const base_color &c, float eps /*=EPS*/ ) const
{
	bool ret = 
		r._value == c.r._value  &&
		g._value == c.g._value  &&	 
		b._value == c.b._value  &&
		h._value == c.h._value  &&
		s._value == c.s._value  &&
		t._value == c.t._value;
	if(!ret)
	{
		Msg( "base color diff : dr = %d , dg = %d, db = %d, dh = %d, ds = %d, dt = %d ", 
			r._value - c.r._value,g._value - c.g._value,b._value - c.b._value,h._value - c.h._value,
			s._value - c.s._value , t._value - c.t._value 
		);
		Msg( "base color  : r = %d,%d , g = %d,%d, b = %d,%d, h = %d,%d, s = %d,%d, t = %d,%d ", 
			r._value ,c.r._value,g._value ,c.g._value,b._value ,c.b._value,h._value ,c.h._value,
			s._value ,c.s._value , t._value ,c.t._value 
		);
	}
	return ret;
}