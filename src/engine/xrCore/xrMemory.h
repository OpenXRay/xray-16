#ifndef xrMemoryH
#define xrMemoryH
#pragma once

#include "memory_monitor.h"

#ifdef USE_MEMORY_MONITOR
#	define DEBUG_MEMORY_NAME
#endif // USE_MEMORY_MONITOR

#ifndef M_BORLAND
#	if 0//def DEBUG
#		define DEBUG_MEMORY_MANAGER
#	endif // DEBUG
#endif // M_BORLAND

#ifdef DEBUG_MEMORY_MANAGER
	XRCORE_API	extern BOOL	g_bMEMO;
#	ifndef DEBUG_MEMORY_NAME
#		define DEBUG_MEMORY_NAME
#	endif // DEBUG_MEMORY_NAME
	extern XRCORE_API	void dump_phase	();
#	define DUMP_PHASE	do {dump_phase();} while (0)
#else // DEBUG_MEMORY_MANAGER
#	define DUMP_PHASE	do {} while (0)
#endif // DEBUG_MEMORY_MANAGER

#include "xrMemory_pso.h"
#include "xrMemory_POOL.h"

class XRCORE_API		xrMemory
{
public:
	struct				mdbg {
		void*			_p;
		size_t 			_size;
		const char*		_name;
		u32				_dummy;
	};
public:
	xrMemory			();
	void				_initialize		(BOOL _debug_mode=FALSE);
	void				_destroy		();

#ifdef DEBUG_MEMORY_MANAGER
	BOOL				debug_mode;
	xrCriticalSection	debug_cs;
	std::vector<mdbg>	debug_info;
	u32					debug_info_update;
	u32					stat_strcmp		;
	u32					stat_strdock	;
#endif // DEBUG_MEMORY_MANAGER

	u32					stat_calls;
	s32					stat_counter;
public:
	void				dbg_register	(void* _p,	size_t _size, const char* _name);
	void				dbg_unregister	(void* _p);
	void				dbg_check		();

	u32					mem_usage		(u32* pBlocksUsed=NULL, u32* pBlocksFree=NULL);
	void				mem_compact		();
	void				mem_counter_set	(u32 _val)	{ stat_counter = _val;	}
	u32					mem_counter_get	()			{ return stat_counter;	}

#ifdef DEBUG_MEMORY_NAME
	void				mem_statistic	(LPCSTR fn);
	void*				mem_alloc		(size_t	size				, const char* _name);
	void*				mem_realloc		(void*	p, size_t size		, const char* _name);
#else // DEBUG_MEMORY_NAME
	void*				mem_alloc		(size_t	size				);
	void*				mem_realloc		(void*	p, size_t size		);
#endif // DEBUG_MEMORY_NAME
	void				mem_free		(void*	p					);

	pso_MemCopy*		mem_copy;
	pso_MemFill*		mem_fill;
	pso_MemFill32*		mem_fill32;
};

extern XRCORE_API	xrMemory	Memory;

#undef	ZeroMemory
#undef	CopyMemory
#undef	FillMemory
#define ZeroMemory(a,b)		Memory.mem_fill(a,0,b)
#define CopyMemory(a,b,c)	memcpy(a,b,c)			//. CopyMemory(a,b,c)
#define FillMemory(a,b,c)	Memory.mem_fill(a,c,b)

// delete
#ifdef __BORLANDC__
	#include "xrMemory_subst_borland.h"
#else
	#include "xrMemory_subst_msvc.h"
#endif

// generic "C"-like allocations/deallocations
#ifdef DEBUG_MEMORY_NAME
	#include "typeinfo.h"

	template <class T>
	IC T*		xr_alloc	(u32 count)				{	return  (T*)Memory.mem_alloc(count*sizeof(T),typeid(T).name());	}
	template <class T>
	IC void		xr_free		(T* &P)					{	if (P) { Memory.mem_free((void*)P); P=NULL;	};	}
	IC void*	xr_malloc	(size_t size)			{	return	Memory.mem_alloc(size,"xr_malloc");				}
	IC void*	xr_realloc	(void* P, size_t size)	{	return Memory.mem_realloc(P,size,"xr_realloc");			}
#else // DEBUG_MEMORY_NAME
	template <class T>
	IC T*		xr_alloc	(u32 count)				{	return  (T*)Memory.mem_alloc(count*sizeof(T));	}
	template <class T>
	IC void		xr_free		(T* &P)					{	if (P) { Memory.mem_free((void*)P); P=NULL;	};	}
	IC void*	xr_malloc	(size_t size)			{	return	Memory.mem_alloc(size);					}
	IC void*	xr_realloc	(void* P, size_t size)	{	return Memory.mem_realloc(P,size);				}
#endif // DEBUG_MEMORY_NAME

XRCORE_API	char* 	xr_strdup	(const char* string);

#ifdef DEBUG_MEMORY_NAME
// Global new/delete override
#	if !(defined(__BORLANDC__) || defined(NO_XRNEW))
	IC void*	operator new		(size_t size)		{	return Memory.mem_alloc(size?size:1, "C++ NEW");	}
	IC void		operator delete		(void *p)			{	xr_free(p);											}
	IC void*	operator new[]		(size_t size)		{	return Memory.mem_alloc(size?size:1, "C++ NEW");	}
	IC void		operator delete[]	(void* p)			{	xr_free(p);											}
#	endif
#else // DEBUG_MEMORY_NAME
#	if !(defined(__BORLANDC__) || defined(NO_XRNEW))
	IC void*	operator new		(size_t size)		{	return Memory.mem_alloc(size?size:1);				}
	IC void		operator delete		(void *p)			{	xr_free(p);											}
	IC void*	operator new[]		(size_t size)		{	return Memory.mem_alloc(size?size:1);				}
	IC void		operator delete[]	(void* p)			{	xr_free(p);											}
#	endif
#endif // DEBUG_MEMORY_MANAGER


// POOL-ing
const		u32			mem_pools_count			=	54;
const		u32			mem_pools_ebase			=	16;
const		u32			mem_generic				=	mem_pools_count+1;
extern		MEMPOOL		mem_pools				[mem_pools_count];
extern		BOOL		mem_initialized;

XRCORE_API void vminfo			(size_t *_free, size_t *reserved, size_t *committed);
XRCORE_API void log_vminfo		();
XRCORE_API u32	mem_usage_impl	(HANDLE heap_handle, u32* pBlocksUsed, u32* pBlocksFree);

#endif // xrMemoryH