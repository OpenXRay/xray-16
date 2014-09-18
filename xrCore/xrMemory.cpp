#include "stdafx.h"
#pragma hdrstop

#include	"xrsharedmem.h"
#include	"xrMemory_pure.h"

#include	<malloc.h>

xrMemory	Memory;
BOOL		mem_initialized	= FALSE;
bool		shared_str_initialized	= false;

//fake fix of memory corruptions in multiplayer game :(
XRCORE_API	bool	g_allow_heap_min = true;

// Processor specific implementations
extern		pso_MemCopy		xrMemCopy_MMX;
extern		pso_MemCopy		xrMemCopy_x86;
extern		pso_MemFill		xrMemFill_x86;
extern		pso_MemFill32	xrMemFill32_MMX;
extern		pso_MemFill32	xrMemFill32_x86;

#ifdef DEBUG_MEMORY_MANAGER
XRCORE_API void dump_phase		()
{
	if (!Memory.debug_mode)
		return;

	static int					phase_counter = 0;

	string256					temp;
	xr_sprintf					(temp,sizeof(temp),"x:\\$phase$%d.dump",++phase_counter);
	Memory.mem_statistic		(temp);
}
#endif // DEBUG_MEMORY_MANAGER

xrMemory::xrMemory()
#ifdef DEBUG_MEMORY_MANAGER
#	ifdef PROFILE_CRITICAL_SECTIONS
		:debug_cs(MUTEX_PROFILE_ID(xrMemory))
#	endif // PROFILE_CRITICAL_SECTIONS
#endif // DEBUG_MEMORY_MANAGER
{
#ifdef DEBUG_MEMORY_MANAGER

	debug_mode	= FALSE;

#endif // DEBUG_MEMORY_MANAGER
	mem_copy	= xrMemCopy_x86;
	mem_fill	= xrMemFill_x86;
	mem_fill32	= xrMemFill32_x86;
}

#ifdef DEBUG_MEMORY_MANAGER
	BOOL	g_bMEMO		= FALSE;
#endif // DEBUG_MEMORY_MANAGER

void	xrMemory::_initialize	(BOOL bDebug)
{
#ifdef DEBUG_MEMORY_MANAGER
	debug_mode				= bDebug;
	debug_info_update		= 0;
#endif // DEBUG_MEMORY_MANAGER

	stat_calls				= 0;
	stat_counter			= 0;

	u32	features		= CPU::ID.feature;
	if (features & _CPU_FEATURE_MMX)
	{
		mem_copy	= xrMemCopy_MMX;
		mem_fill	= xrMemFill_x86;
		mem_fill32	= xrMemFill32_MMX;
	} else {
		mem_copy	= xrMemCopy_x86;
		mem_fill	= xrMemFill_x86;
		mem_fill32	= xrMemFill32_x86;
	}

#ifndef M_BORLAND
	if (!strstr(Core.Params,"-pure_alloc")) {
		// initialize POOLs
		u32	element		= mem_pools_ebase;
		u32 sector		= mem_pools_ebase*1024;
		for (u32 pid=0; pid<mem_pools_count; pid++)
		{
			mem_pools[pid]._initialize(element,sector,0x1);
			element		+=	mem_pools_ebase;
		}
	}
#endif // M_BORLAND

#ifdef DEBUG_MEMORY_MANAGER
	if (0==strstr(Core.Params,"-memo"))	mem_initialized				= TRUE;
	else								g_bMEMO						= TRUE;
#else // DEBUG_MEMORY_MANAGER
	mem_initialized				= TRUE;
#endif // DEBUG_MEMORY_MANAGER

//	DUMP_PHASE;
	g_pStringContainer			= xr_new<str_container>		();
	shared_str_initialized		= true;
//	DUMP_PHASE;
	g_pSharedMemoryContainer	= xr_new<smem_container>	();
//	DUMP_PHASE;
}

#ifdef DEBUG_MEMORY_MANAGER
	extern void dbg_dump_leaks();
	extern void dbg_dump_str_leaks();
#endif // DEBUG_MEMORY_MANAGER

void	xrMemory::_destroy()
{
#ifdef DEBUG_MEMORY_MANAGER
	mem_alloc_gather_stats		(false);
	mem_alloc_show_stats		();
	mem_alloc_clear_stats		();
#endif // DEBUG

#ifdef DEBUG_MEMORY_MANAGER
	if (debug_mode)				dbg_dump_str_leaks	();
#endif // DEBUG_MEMORY_MANAGER

	xr_delete					(g_pSharedMemoryContainer);
	xr_delete					(g_pStringContainer);

#ifndef M_BORLAND
#	ifdef DEBUG_MEMORY_MANAGER
		if (debug_mode)				dbg_dump_leaks	();
#	endif // DEBUG_MEMORY_MANAGER
#endif // M_BORLAND

	mem_initialized				= FALSE;
#ifdef DEBUG_MEMORY_MANAGER
	debug_mode					= FALSE;
#endif // DEBUG_MEMORY_MANAGER
}

void	xrMemory::mem_compact	()
{
	RegFlushKey						( HKEY_CLASSES_ROOT );
	RegFlushKey						( HKEY_CURRENT_USER );
	if (g_allow_heap_min)
		_heapmin					( );
	HeapCompact					(GetProcessHeap(),0);
	if (g_pStringContainer)			g_pStringContainer->clean		();
	if (g_pSharedMemoryContainer)	g_pSharedMemoryContainer->clean	();
	if (strstr(Core.Params,"-swap_on_compact"))
		SetProcessWorkingSetSize	(GetCurrentProcess(),size_t(-1),size_t(-1));
}

#ifdef DEBUG_MEMORY_MANAGER
ICF	u8*		acc_header			(void* P)	{	u8*		_P		= (u8*)P;	return	_P-1;	}
ICF	u32		get_header			(void* P)	{	return	(u32)*acc_header(P);				}
void	xrMemory::mem_statistic	(LPCSTR fn)
{
	if (!debug_mode)	return	;
	mem_compact				()	;

	debug_cs.Enter			()	;
	debug_mode				= FALSE;

	FILE*		Fa			= fopen		(fn,"w");
	fprintf					(Fa,"$BEGIN CHUNK #0\n");
	fprintf					(Fa,"POOL: %d %dKb\n",mem_pools_count,mem_pools_ebase);

	fprintf					(Fa,"$BEGIN CHUNK #1\n");
	for (u32 k=0; k<mem_pools_count; ++k)
		fprintf				(Fa,"%2d: %d %db\n",k,mem_pools[k].get_block_count(),(k+1)*16);
	
	fprintf					(Fa,"$BEGIN CHUNK #2\n");
	for (u32 it=0; it<debug_info.size(); it++)
	{
		if (0==debug_info[it]._p)	continue	;

		u32 p_current		= get_header(debug_info[it]._p);
		int pool_id			= (mem_generic==p_current)?-1:p_current;

		fprintf				(Fa,"0x%08X[%2d]: %8d %s\n",*(u32*)(&debug_info[it]._p),pool_id,debug_info[it]._size,debug_info[it]._name);
	}

	{
		for (u32 k=0; k<mem_pools_count; ++k) {
			MEMPOOL			&pool = mem_pools[k];
			u8				*list = pool.list;
			while (list) {
				pool.cs.Enter	();
				u32				temp = *(u32*)(&list);
				if (!temp)
					break;
				fprintf			(Fa,"0x%08X[%2d]: %8d mempool\n",temp,k,pool.s_element);
				list			= (u8*)*pool.access(list);
				pool.cs.Leave	();
			}
		}
	}

	/*
	fprintf					(Fa,"$BEGIN CHUNK #3\n");
	for (u32 it=0; it<debug_info.size(); it++)
	{
		if (0==debug_info[it]._p)	continue	;
		try{
			if (0==strcmp(debug_info[it]._name,"storage: sstring"))
				fprintf		(Fa,"0x%08X: %8d %s %s\n",*(u32*)(&debug_info[it]._p),debug_info[it]._size,debug_info[it]._name,((str_value*)(*(u32*)(&debug_info[it]._p)))->value);
		}catch(...){
		}
	}
	*/

	fclose		(Fa)		;

	// leave
	debug_mode				= TRUE;
	debug_cs.Leave			();

	/*
	mem_compact				();
	LPCSTR					fn	= "$memstat$.tmp";
	xr_map<u32,u32>			stats;

	if (g_pStringContainer)			Msg	("memstat: shared_str: economy: %d bytes",g_pStringContainer->stat_economy());
	if (g_pSharedMemoryContainer)	Msg	("memstat: shared_mem: economy: %d bytes",g_pSharedMemoryContainer->stat_economy());

	// Dump memory stats into file to avoid reallocation while traversing
	{
		IWriter*	F		= FS.w_open(fn);
		F->w_u32			(0);
		_HEAPINFO			hinfo;
		int					heapstatus;
		hinfo._pentry		= NULL;
		while( ( heapstatus = _heapwalk( &hinfo ) ) == _HEAPOK )
			if (hinfo._useflag == _USEDENTRY)	F->w_u32	(u32(hinfo._size));
		FS.w_close			(F);
	}

	// Read back and perform sorting
	{
		IReader*	F		= FS.r_open	(fn);
		u32 size			= F->r_u32	();
		while (!F->eof())
		{
			size						= F->r_u32	();
			xr_map<u32,u32>::iterator I	= stats.find(size);
			if (I!=stats.end())			I->second += 1;
			else						stats.insert(mk_pair(size,1));
		}
		FS.r_close			(F);
		FS.file_delete		(fn);
	}

	// Output to log
	{
		xr_map<u32,u32>::iterator I		= stats.begin();
		xr_map<u32,u32>::iterator E		= stats.end();
		for (; I!=E; I++)	Msg			("%8d : %-4d [%d]",I->first,I->second,I->first*I->second);
	}
	*/
}
#endif // DEBUG_MEMORY_MANAGER

// xr_strdup
char*			xr_strdup		(const char* string)
{	
	VERIFY	(string);
	u32		len			= u32(xr_strlen(string))+1	;
	char *	memory		= (char*)	Memory.mem_alloc( len
#ifdef DEBUG_MEMORY_NAME
		, "strdup"
#endif // DEBUG_MEMORY_NAME
	);
	CopyMemory		(memory,string,len);
	return	memory;
}

XRCORE_API		BOOL			is_stack_ptr		( void* _ptr)
{
	int			local_value		= 0;
	void*		ptr_refsound	= _ptr;
	void*		ptr_local		= &local_value;
	ptrdiff_t	difference		= (ptrdiff_t)_abs(s64(ptrdiff_t(ptr_local) - ptrdiff_t(ptr_refsound)));
	return		(difference < (512*1024));
}
