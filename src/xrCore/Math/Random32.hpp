////////////////////////////////////////////////////////////////////////////
//	Module 		: random32.cpp
//	Created 	: 09.03.2004
//  Modified 	: 09.03.2004
//	Author		: Dmitriy Iassenev
//	Description : 32-bit peudo random number generator
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/xrCore.h"

class CRandom32
{
private:
    u32 m_seed;

public:
    inline u32 seed() const { return m_seed; }
    inline void seed(u32 seed) { m_seed = seed; }
    inline u32 random(u32 range)
    {
        m_seed = 0x08088405U * m_seed + 1U;
        return u32((u64(m_seed) * u64(range)) >> 32U);
    }
};
