#pragma once

#include "alife_space.h"

class xrTime{
	ALife::_TIME_ID		m_time;
public:
	xrTime():m_time(0){}
	xrTime(const xrTime& other):m_time(other.m_time){}
	xrTime(ALife::_TIME_ID t):m_time(t){}

	bool	operator <		(const xrTime& other)	const			{ return m_time < other.m_time;			}
	bool	operator >		(const xrTime& other)	const			{ return m_time > other.m_time;			}
	bool	operator >=		(const xrTime& other)	const			{ return m_time >= other.m_time;		}
	bool	operator <=		(const xrTime& other)	const			{ return m_time <= other.m_time;		}
	bool	operator ==		(const xrTime& other)	const			{ return m_time == other.m_time;		}
	xrTime	operator +		(const xrTime& other)					{ return xrTime(m_time+other.m_time);	}
	xrTime	operator -		(const xrTime& other)					{ return xrTime(m_time-other.m_time);	}
	
	float	diffSec			(const xrTime& other);
	void	add				(const xrTime& other);
	void	sub				(const xrTime& other);

	void	add_script		(xrTime* other){add(*other);};
	void	sub_script		(xrTime* other){sub(*other);};
	float	diffSec_script	(xrTime* other){return diffSec(*other);};

	void	setHMS			(int h, int m, int s);
	void	setHMSms		(int h, int m, int s, int ms);
	void	set				(int y, int mo, int d, int h, int mi, int s, int ms);
	void	get				(u32 &y, u32 &mo, u32 &d, u32 &h, u32 &mi, u32 &s, u32 &ms);

	LPCSTR	dateToString	(int mode);
	LPCSTR	timeToString	(int mode);
};


extern u32 get_time();
extern xrTime get_time_struct();