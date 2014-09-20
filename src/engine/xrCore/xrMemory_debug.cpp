#include "StdAfx.h"
#pragma hdrstop

#ifndef DEBUG_MEMORY_MANAGER
void	xrMemory::dbg_register		(void* _p, size_t _size, const char* _name)	{ }
void	xrMemory::dbg_unregister	(void* _p)									{ }
void	xrMemory::dbg_check			()											{ }

#else // DEBUG_MEMORY_MANAGER
#	if 0
#		define DEBUG_MEMORY_LEAK		
#		define MEMORY_LEAK_DESCRIPTION	"C++ NEW"
//#		define MEMORY_LEAK_DESCRIPTION	"class luabind::functor<bool>"
#		define MEMORY_LEAK_SIZE			12
#	endif

#include <malloc.h>

bool	pred_mdbg	(const xrMemory::mdbg& A)	{
	return (0==A._p && 0==A._size);
}
extern	u32		get_header		(void*	P);
extern	u32		get_pool		(size_t size);
BOOL	g_bDbgFillMemory		= true;

void	dbg_header			(xrMemory::mdbg& dbg, bool _debug)
{
	//. check header
	u32 t1 = get_header	(dbg._p);
	u32 t2 = get_pool	(1+dbg._size+(_debug?4:0));
	R_ASSERT2			(t1==t2,"CorePanic: Memory block header corrupted");
}

void	xrMemory::dbg_register		(void* _p, size_t _size, const char* _name)
{
#ifdef DEBUG_MEMORY_LEAK
	if ((_size == MEMORY_LEAK_SIZE) && _name &&!xr_strcmp(MEMORY_LEAK_DESCRIPTION,_name)) {
		static int			i = 0;
		string2048			temp;
		xr_sprintf			(temp,sizeof(temp),"____[%s][%d] : 0x%8x [REGISTER][%d]\n",_name,_size,(u32)((size_t)_p),i++);
		OutputDebugString	(temp);
	}
#endif

	VERIFY					(debug_mode);
	debug_cs.Enter			();
	debug_mode				= FALSE;

	// register + mark
	mdbg	dbg				=  { _p,_size,_name, 0 };
	dbg_header				(dbg,true);
	debug_info.push_back	(dbg);
	u8*			_ptr		= (u8*)	_p;
	u32*		_shred		= (u32*)(_ptr + _size);
	*_shred					= u32	(-1);
	dbg_header				(dbg,true);

	debug_mode				= TRUE;
	debug_cs.Leave			();
}
void	xrMemory::dbg_unregister	(void* _p)
{
	VERIFY					(debug_mode);
	debug_cs.Enter			();
	debug_mode				= FALSE;

	// search entry
	u32	_found				= u32(-1);

	if (!debug_info.empty())
	{
		for (int it=int(debug_info.size()-1); it>=0; it--)
			if (debug_info[it]._p==_p)	{ _found=it; break; }
	}

	// unregister entry
	if (u32(-1)==_found)	{ 
		FATAL					("Memory allocation error: double free() ?"); 
	} else	{
#ifdef DEBUG_MEMORY_LEAK
		if ((debug_info[_found]._size == MEMORY_LEAK_SIZE) && debug_info[_found]._name && !xr_strcmp(MEMORY_LEAK_DESCRIPTION,debug_info[_found]._name)) {
			string2048			temp;
			xr_sprintf			(temp,sizeof(temp),"____[%s][%d] : 0x%8x [UNREGISTER]\n",debug_info[_found]._name,debug_info[_found]._size,(u32)((size_t)_p));
			OutputDebugString	(temp);
		}
#endif

		u8*			_ptr	= (u8*)	debug_info[_found]._p;
		u32*		_shred	= (u32*)(_ptr + debug_info[_found]._size);
		R_ASSERT2			(u32(-1)==*_shred, "Memory overrun error");

		// fill free memory with random data
		if (g_bDbgFillMemory)
			memset			(debug_info[_found]._p,'C',debug_info[_found]._size);

		// clear record
		std::swap			(debug_info[_found],debug_info.back());
		debug_info.pop_back	();
		debug_info_update	++;
	}

	// perform cleanup
	if (debug_info_update>1024*100)
	{
		debug_info_update	=	0;
		debug_info.erase	(std::remove_if(debug_info.begin(),debug_info.end(),pred_mdbg),debug_info.end());
		dbg_check			();
	}

	debug_mode				= TRUE;
	debug_cs.Leave			();
}

void	xrMemory::dbg_check		()
{
	if (!debug_mode)		return;

	// Check RO strings
	if (g_pStringContainer) g_pStringContainer->verify	();

	// Check overrun
	debug_cs.Enter			();
	debug_mode				= FALSE;
	for (int it=0; it<int(debug_info.size()); it++)
	{
		if (0==debug_info[it]._p)	
			continue;

		// check header
		dbg_header			(debug_info[it],true);

		// check footer
		u8*			_ptr	= (u8*)	debug_info[it]._p;
		u32*		_shred	= (u32*)(_ptr + debug_info[it]._size);
		R_ASSERT2			(u32(-1)==*_shred, "CorePanic: Memory overrun error");
	}

	// crt-check
	R_ASSERT2(_HEAPOK==_heapchk(),					"CorePanic: CRT heap corruption");
	R_ASSERT2(HeapValidate(GetProcessHeap(),0,0),	"CorePanic: Win32 heap corruption");

	// leave
	debug_mode				= TRUE;
	debug_cs.Leave			();
}

XRCORE_API void	dbg_dump_leaks_prepare	()
{
	Memory.mem_compact		()	;

	Memory.debug_cs.Enter	()	;
	Memory.debug_mode		= FALSE;

	for (u32 it=0; it<Memory.debug_info.size(); it++)
	{
		if (0==Memory.debug_info[it]._p)		continue	;
		if (0==Memory.debug_info[it]._name)		continue	;
		Memory.debug_info[it]._name			=	xr_strdup	(Memory.debug_info[it]._name);
	}

	// leave
	Memory.debug_mode		= TRUE;
	Memory.debug_cs.Leave	();
}

XRCORE_API void	dbg_dump_leaks			()
{
	Memory.mem_statistic	("x:\\$memory_leak$.dump");
}

XRCORE_API void	dbg_dump_str_leaks			()
{
	g_pStringContainer->dump();
}
#endif // DEBUG_MEMORY_MANAGER