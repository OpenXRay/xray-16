#include "stdafx.h"
#pragma hdrstop

#include ".\soundrender_cache.h"

CSoundRender_Cache::CSoundRender_Cache	()
{
	data		= NULL;
	c_storage	= NULL;
	c_begin		= NULL;
	c_end		= NULL;
	_total		= 0;
	_line		= 0;
	_count		= 0;
}

CSoundRender_Cache::~CSoundRender_Cache	()
{
}


void	CSoundRender_Cache::move2top	(cache_line* line)
{
	VERIFY						(line);
	if (line==c_begin)			return;			// already at top

	// track end
	if (line==c_end)			c_end = c_end->prev;

	// cut 
	cache_line*		prev		= line->prev;
	cache_line*		next		= line->next;
	if (prev)		prev->next	= next;
	if (next)		next->prev	= prev;

	// register at top
	line->prev					= NULL;
	line->next					= c_begin;

	// track begin
	c_begin->prev				= line;
	c_begin						= line;

	// internal verify
	VERIFY						(c_begin->prev	== NULL);
	VERIFY						(c_end->next	== NULL);
}

BOOL	CSoundRender_Cache::request		(cache_cat& cat, u32 id)
{
	// 1. check if cached version available
	id				%= cat.size;
//.	R_ASSERT		(id<cat.size);
	u16&	cptr	= cat.table[id];
	if (CAT_FREE != cptr)	{
		// cache line exists - change it's priority and return
		_stat_hit		++;
		cache_line*	L	=	c_storage + cptr;
		move2top		(L);
		return			FALSE;
	}

	// 2. purge oldest item + move it to top
	_stat_miss		++;
	move2top	(c_end);
	if (c_begin->loopback)	{
		*c_begin->loopback		= CAT_FREE;
		c_begin->loopback		= NULL;
	}

	// 3. associate
	cptr				= c_begin->id;
	c_begin->loopback	= &cptr;

	// 4. fill with data
	return			TRUE;
}

void	CSoundRender_Cache::initialize	(u32 _total_kb_approx, u32 bytes_per_line)
{
	// use twice the requisted memory (to avoid bad configs)
	_total_kb_approx	*=	2;

	// calc
	_line		= bytes_per_line;
	_count		= ((_total_kb_approx*1024)/bytes_per_line + 1);
	_total		= _count*_line;
	R_ASSERT	(_count<CAT_FREE);
	Msg			("* sound : cache: %d kb, %d lines, %d bpl",_total/1024,_count,_line);

	// alloc structs
	data		= xr_alloc<u8>			(_total);
	c_storage	= xr_alloc<cache_line>	(_count);
	
	// format
	format		();
}

void	CSoundRender_Cache::disconnect	()
{
	// disconnect from CATs
	for (u32 it=0; it<_count; it++)
	{
		cache_line*		L	= c_storage+it;
		if (L->loopback)	{
			*L->loopback		= CAT_FREE;
			L->loopback			= NULL;
		}
	}
}

void	CSoundRender_Cache::format		()
{
	// format structs
	for (u32 it=0; it<_count; it++)
	{
		cache_line*		L	= c_storage+it;
		L->prev				= (0==it)				? NULL : c_storage+it-1;
		L->next				= ((_count-1) == it)	? NULL : c_storage+it+1;
		L->data				= data + it*_line;
		L->loopback			= NULL;
		L->id				= u16	(it);
	}

	// start-end
	c_begin		= c_storage + 0;
	c_end		= c_storage + _count - 1;
}

void	CSoundRender_Cache::purge		()
{
	disconnect	();		// disconnect from CATs
	format		();		// format
}

void	CSoundRender_Cache::destroy		()
{
	disconnect	();
	xr_free		(data);
	xr_free		(c_storage);
	c_begin		= NULL;
	c_end		= NULL;
	_total		= 0;
	_line		= 0;
	_count		= 0;
}

void	CSoundRender_Cache::cat_create	(cache_cat& cat, u32 bytes)
{
	cat.size			=	bytes / _line;
	if	(bytes%_line)	cat.size += 1;
	u32 allocsize		=	(cat.size&1)?cat.size+1:cat.size;
	cat.table			=	xr_alloc<u16>(allocsize);
	Memory.mem_fill32	(cat.table,0xffffffff,allocsize/2);
}

void	CSoundRender_Cache::cat_destroy	(cache_cat& cat)
{
	xr_free				(cat.table);
	cat.size			= 0;
}
