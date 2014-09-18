#include "stdafx.h"
#pragma hdrstop

#include "xrMemory_align.h"
#include "xrMemory_pure.h"

#ifndef	__BORLANDC__

#ifndef DEBUG_MEMORY_MANAGER
#	define	debug_mode 0
#endif // DEBUG_MEMORY_MANAGER

#ifdef DEBUG_MEMORY_MANAGER
	XRCORE_API void*	g_globalCheckAddr = NULL;
#endif // DEBUG_MEMORY_MANAGER

#ifdef DEBUG_MEMORY_MANAGER
	extern void save_stack_trace	();
#endif // DEBUG_MEMORY_MANAGER

MEMPOOL		mem_pools			[mem_pools_count];

// MSVC
ICF	u8*		acc_header			(void* P)	{	u8*		_P		= (u8*)P;	return	_P-1;	}
ICF	u32		get_header			(void* P)	{	return	(u32)*acc_header(P);				}
ICF	u32		get_pool			(size_t size)
{
	u32		pid					= u32(size/mem_pools_ebase);
	if (pid>=mem_pools_count)	return mem_generic;
	else						return pid;
}

#ifdef PURE_ALLOC
bool	g_use_pure_alloc		= false;
#endif // PURE_ALLOC

void*	xrMemory::mem_alloc		(size_t size
#	ifdef DEBUG_MEMORY_NAME
								 , const char* _name
#	endif // DEBUG_MEMORY_NAME
								 )
{
	stat_calls++;

#ifdef PURE_ALLOC
	static bool g_use_pure_alloc_initialized = false;
	if (!g_use_pure_alloc_initialized) {
		g_use_pure_alloc_initialized	= true;
		g_use_pure_alloc				= 
#	ifdef XRCORE_STATIC
			true
#	else // XRCORE_STATIC
			!!strstr(GetCommandLine(),"-pure_alloc")
#	endif // XRCORE_STATIC
			;
	}

	if (g_use_pure_alloc) {
		void							*result = malloc(size);
#ifdef USE_MEMORY_MONITOR
		memory_monitor::monitor_alloc	(result,size,_name);
#endif // USE_MEMORY_MONITOR
		return							(result);
	}
#endif // PURE_ALLOC

#ifdef DEBUG_MEMORY_MANAGER
	if (mem_initialized)		debug_cs.Enter		();
#endif // DEBUG_MEMORY_MANAGER

	u32		_footer				=	debug_mode?4:0;
	void*	_ptr				=	0;

	//
	if (!mem_initialized /*|| debug_mode*/)		
	{
		// generic
		//	Igor: Reserve 1 byte for xrMemory header
		void*	_real			=	xr_aligned_offset_malloc	(1 + size + _footer, 16, 0x1);
		//void*	_real			=	xr_aligned_offset_malloc	(size + _footer, 16, 0x1);
		_ptr					=	(void*)(((u8*)_real)+1);
		*acc_header(_ptr)		=	mem_generic;
	} else {
#ifdef DEBUG_MEMORY_MANAGER
		save_stack_trace		();
#endif // DEBUG
		//	accelerated
		//	Igor: Reserve 1 byte for xrMemory header
		u32	pool				=	get_pool	(1+size+_footer);
		//u32	pool				=	get_pool	(size+_footer);
		if (mem_generic==pool)	
		{
			// generic
			//	Igor: Reserve 1 byte for xrMemory header
			void*	_real		=	xr_aligned_offset_malloc	(1 + size + _footer,16,0x1);
			//void*	_real		=	xr_aligned_offset_malloc	(size + _footer,16,0x1);
			_ptr				=	(void*)(((u8*)_real)+1);
			*acc_header(_ptr)	=	mem_generic;
		} else {
			// pooled
			//	Igor: Reserve 1 byte for xrMemory header
			//	Already reserved when getting pool id
			void*	_real		=	mem_pools[pool].create();
			_ptr				=	(void*)(((u8*)_real)+1);
			*acc_header(_ptr)	=	(u8)pool;
		}
	}

#ifdef DEBUG_MEMORY_MANAGER
	if		(debug_mode)		dbg_register		(_ptr,size,_name);
	if (mem_initialized)		debug_cs.Leave		();
	//if(g_globalCheckAddr==_ptr){
	//	__asm int 3;
	//}
	//if (_name && (0==strcmp(_name,"class ISpatial *")) && (size==376))
	//{
	//	__asm int 3;
	//}
#endif // DEBUG_MEMORY_MANAGER
#ifdef USE_MEMORY_MONITOR
	memory_monitor::monitor_alloc	(_ptr,size,_name);
#endif // USE_MEMORY_MONITOR
	return	_ptr;
}

void	xrMemory::mem_free		(void* P)
{
	stat_calls++;
#ifdef USE_MEMORY_MONITOR
	memory_monitor::monitor_free(P);
#endif // USE_MEMORY_MONITOR

#ifdef PURE_ALLOC
	if (g_use_pure_alloc) {
		free					(P);
		return;
	}
#endif // PURE_ALLOC

#ifdef DEBUG_MEMORY_MANAGER
	if(g_globalCheckAddr==P)
		__asm int 3;
#endif // DEBUG_MEMORY_MANAGER

#ifdef DEBUG_MEMORY_MANAGER
	if (mem_initialized)		debug_cs.Enter		();
#endif // DEBUG_MEMORY_MANAGER
	if		(debug_mode)		dbg_unregister	(P);
	u32	pool					= get_header	(P);
	void* _real					= (void*)(((u8*)P)-1);
	if (mem_generic==pool)		
	{
		// generic
		xr_aligned_free			(_real);
	} else {
		// pooled
		VERIFY2					(pool<mem_pools_count,"Memory corruption");
		mem_pools[pool].destroy	(_real);
	}
#ifdef DEBUG_MEMORY_MANAGER
	if (mem_initialized)		debug_cs.Leave	();
#endif // DEBUG_MEMORY_MANAGER
}

extern BOOL	g_bDbgFillMemory	;

void*	xrMemory::mem_realloc	(void* P, size_t size
#ifdef DEBUG_MEMORY_NAME
								 , const char* _name
#endif // DEBUG_MEMORY_NAME
								 )
{
	stat_calls++;
#ifdef PURE_ALLOC
	if (g_use_pure_alloc) {
		void							*result = realloc(P,size);
#	ifdef USE_MEMORY_MONITOR
		memory_monitor::monitor_free	(P);
		memory_monitor::monitor_alloc	(result,size,_name);
#	endif // USE_MEMORY_MONITOR
		return							(result);
	}
#endif // PURE_ALLOC
	if (0==P) {
		return mem_alloc	(size
#	ifdef DEBUG_MEMORY_NAME
		,_name
#	endif // DEBUG_MEMORY_NAME
		);
	}

#ifdef DEBUG_MEMORY_MANAGER
	if(g_globalCheckAddr==P)
		__asm int 3;
#endif // DEBUG_MEMORY_MANAGER

#ifdef DEBUG_MEMORY_MANAGER
	if (mem_initialized)		debug_cs.Enter		();
#endif // DEBUG_MEMORY_MANAGER
	u32		p_current			= get_header(P);
	//	Igor: Reserve 1 byte for xrMemory header
	u32		p_new				= get_pool	(1+size+(debug_mode?4:0));
	//u32		p_new				= get_pool	(size+(debug_mode?4:0));
	u32		p_mode				;

	if (mem_generic==p_current)	{
		if (p_new<p_current)		p_mode	= 2	;
		else						p_mode	= 0	;
	} else 							p_mode	= 1	;

	void*	_real				= (void*)(((u8*)P)-1);
	void*	_ptr				= NULL;
	if		(0==p_mode)
	{
		u32		_footer			=	debug_mode?4:0;
#ifdef DEBUG_MEMORY_MANAGER
		if		(debug_mode)	{
			g_bDbgFillMemory	= false;
			dbg_unregister		(P);
			g_bDbgFillMemory	= true;
		}
#endif // DEBUG_MEMORY_MANAGER
		//	Igor: Reserve 1 byte for xrMemory header
		void*	_real2			=	xr_aligned_offset_realloc	(_real,1+size+_footer,16,0x1);
		//void*	_real2			=	xr_aligned_offset_realloc	(_real,size+_footer,16,0x1);
		_ptr					= (void*)(((u8*)_real2)+1);
		*acc_header(_ptr)		= mem_generic;
#ifdef DEBUG_MEMORY_MANAGER
		if		(debug_mode)	dbg_register	(_ptr,size,_name);
#endif // DEBUG_MEMORY_MANAGER
#ifdef USE_MEMORY_MONITOR
		memory_monitor::monitor_free	(P);
		memory_monitor::monitor_alloc	(_ptr,size,_name);
#endif // USE_MEMORY_MONITOR
	} else if (1==p_mode)		{
		// pooled realloc
		R_ASSERT2				(p_current<mem_pools_count,"Memory corruption");
		u32		s_current		= mem_pools[p_current].get_element();
		u32		s_dest			= (u32)size;
		void*	p_old			= P;

		void*	p_new			= mem_alloc(size
#ifdef DEBUG_MEMORY_NAME
			,_name
#endif // DEBUG_MEMORY_NAME
		);
		//	Igor: Reserve 1 byte for xrMemory header
		//	Don't bother in this case?
		mem_copy				(p_new,p_old,_min(s_current-1,s_dest));
		//mem_copy				(p_new,p_old,_min(s_current,s_dest));
		mem_free				(p_old);
		_ptr					= p_new;
	} else if (2==p_mode)		{
		// relocate into another mmgr(pooled) from real
		void*	p_old			= P;
		void*	p_new			= mem_alloc(size
#	ifdef DEBUG_MEMORY_NAME
			,_name
#	endif // DEBUG_MEMORY_NAME
		);
		mem_copy				(p_new,p_old,(u32)size);
		mem_free				(p_old);
		_ptr					= p_new;
	}

#ifdef DEBUG_MEMORY_MANAGER
	if (mem_initialized)		debug_cs.Leave	();

	if(g_globalCheckAddr==_ptr)
		__asm int 3;
#endif // DEBUG_MEMORY_MANAGER

	return	_ptr;
}

#endif // __BORLANDC__