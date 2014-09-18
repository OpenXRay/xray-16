////////////////////////////////////////////////////////////////////////////
//	Module 		: random32.cpp
//	Created 	: 09.03.2004
//  Modified 	: 09.03.2004
//	Author		: Dmitriy Iassenev
//	Description : 32-bit peudo random number generator
////////////////////////////////////////////////////////////////////////////

#pragma once

class CRandom32 {
private:
	u32			m_seed;

public:
	IC	u32		seed	()
	{
		return	(m_seed);
	}

	IC	void	seed	(u32 seed)
	{
		m_seed	= seed;
	}

	IC	u32		random	(u32 range)
	{
		m_seed	= 0x08088405*m_seed + 1;
		return	(u32(u64(m_seed)*u64(range) >> 32));
	}
};

extern CRandom32 Random32;