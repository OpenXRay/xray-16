#include "stdafx.h"
#pragma hdrstop // Huh?

#include "xrMemory_POOL.h"
#include "xrMemory_align.h"
#include "Threading/Lock.hpp"


MEMPOOL::MEMPOOL() :
#ifdef CONFIG_PROFILE_LOCKS
	pcs(new Lock(MUTEX_PROFILE_ID(memory_pool))
#else
	pcs(new Lock)
#endif
{
}

MEMPOOL::~MEMPOOL()
{
	delete pcs;
}

void* MEMPOOL::create()
{
	pcs->Enter();
	if (0 == list) block_create();

	void* E = list;
	list = (u8*)*access(list);
	pcs->Leave();
	return E;
}
void MEMPOOL::destroy(void*& P)
{
	pcs->Enter();
	*access(P) = list;
	list = (u8*)P;
	pcs->Leave();
}

void MEMPOOL::block_create()
{
    // Allocate
    R_ASSERT(0 == list);
    list = (u8*)xr_aligned_offset_malloc(s_sector, 16, s_offset);

    // Partition
    for (u32 it = 0; it < (s_count - 1); it++)
    {
        u8* E = list + it * s_element;
        *access(E) = E + s_element;
    }
    *access(list + (s_count - 1) * s_element) = NULL;
    block_count++;
}

void MEMPOOL::_initialize(u32 _element, u32 _sector, u32 _header)
{
    R_ASSERT(_element < _sector / 2);
    s_sector = _sector;
    s_element = _element;
    s_count = s_sector / s_element;
    s_offset = _header;
    list = NULL;
    block_count = 0;
}
