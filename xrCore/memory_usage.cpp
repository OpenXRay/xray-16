#include "stdafx.h"
#include <malloc.h>
#include <errno.h>

XRCORE_API void vminfo (size_t *_free, size_t *reserved, size_t *committed) {
	MEMORY_BASIC_INFORMATION memory_info;
	memory_info.BaseAddress = 0;
	*_free = *reserved = *committed = 0;
	while (VirtualQuery (memory_info.BaseAddress, &memory_info, sizeof (memory_info))) {
		switch (memory_info.State) {
		case MEM_FREE:
			*_free		+= memory_info.RegionSize;
			break;
		case MEM_RESERVE:
			*reserved	+= memory_info.RegionSize;
			break;
		case MEM_COMMIT:
			*committed += memory_info.RegionSize;
			break;
		}
		memory_info.BaseAddress = (char *) memory_info.BaseAddress + memory_info.RegionSize;
	}
}

XRCORE_API void log_vminfo	()
{
	size_t  w_free, w_reserved, w_committed;
	vminfo	(&w_free, &w_reserved, &w_committed);
	Msg		(
		"* [win32]: free[%d K], reserved[%d K], committed[%d K]",
		w_free/1024,
		w_reserved/1024,
		w_committed/1024
	);
}

int heap_walk (
	    HANDLE heap_handle,
        struct _heapinfo *_entry
        )
{
        PROCESS_HEAP_ENTRY Entry;
        DWORD errval;
        int errflag;
        int retval = _HEAPOK;

        Entry.wFlags = 0;
        Entry.iRegionIndex = 0;
		Entry.cbData = 0;
        if ( (Entry.lpData = _entry->_pentry) == NULL ) {
            if ( !HeapWalk( heap_handle, &Entry ) ) {
                if ( GetLastError() == ERROR_CALL_NOT_IMPLEMENTED ) {
                    _doserrno = ERROR_CALL_NOT_IMPLEMENTED;
                    errno = ENOSYS;
                    return _HEAPEND;
                }
                return _HEAPBADBEGIN;
            }
        }
        else {
            if ( _entry->_useflag == _USEDENTRY ) {
                if ( !HeapValidate( heap_handle, 0, _entry->_pentry ) )
                    return _HEAPBADNODE;
                Entry.wFlags = PROCESS_HEAP_ENTRY_BUSY;
            }
nextBlock:
            /*
             * Guard the HeapWalk call in case we were passed a bad pointer
             * to an allegedly free block.
             */
            __try {
                errflag = 0;
                if ( !HeapWalk( heap_handle, &Entry ) )
                    errflag = 1;
            }
            __except( EXCEPTION_EXECUTE_HANDLER ) {
                errflag = 2;
            }

            /*
             * Check errflag to see how HeapWalk fared...
             */
            if ( errflag == 1 ) {
                /*
                 * HeapWalk returned an error.
                 */
                if ( (errval = GetLastError()) == ERROR_NO_MORE_ITEMS ) {
                    return _HEAPEND;
                }
                else if ( errval == ERROR_CALL_NOT_IMPLEMENTED ) {
                    _doserrno = errval;
                    errno = ENOSYS;
                    return _HEAPEND;
                }
                return _HEAPBADNODE;
            }
            else if ( errflag == 2 ) {
                /*
                 * Exception occurred during the HeapWalk!
                 */
                return _HEAPBADNODE;
            }
        }

        if ( Entry.wFlags & (PROCESS_HEAP_REGION |
             PROCESS_HEAP_UNCOMMITTED_RANGE) )
        {
            goto nextBlock;
        }

        _entry->_pentry = (int*)Entry.lpData;
        _entry->_size = Entry.cbData;
        if ( Entry.wFlags & PROCESS_HEAP_ENTRY_BUSY ) {
            _entry->_useflag = _USEDENTRY;
        }
        else {
            _entry->_useflag = _FREEENTRY;
        }

        return( retval );
}

u32	mem_usage_impl	(HANDLE heap_handle, u32* pBlocksUsed, u32* pBlocksFree)
{
	static bool no_memory_usage = !!strstr( GetCommandLine(), "-no_memory_usage");
	if ( no_memory_usage )
		return		0;

	_HEAPINFO		hinfo;
	int				heapstatus;
	hinfo._pentry	= NULL;
	size_t	total	= 0;
	u32	blocks_free	= 0;
	u32	blocks_used	= 0;
	while( ( heapstatus = heap_walk( heap_handle, &hinfo ) ) == _HEAPOK )
	{ 
		if (hinfo._useflag == _USEDENTRY)	{
			total		+= hinfo._size;
			blocks_used	+= 1;
		} else {
			blocks_free	+= 1;
		}
	}
	if (pBlocksFree)	*pBlocksFree= 1024*(u32)blocks_free;
	if (pBlocksUsed)	*pBlocksUsed= 1024*(u32)blocks_used;

	switch( heapstatus )
	{
	case _HEAPEMPTY:
		break;
	case _HEAPEND:
		break;
	case _HEAPBADPTR:
#ifndef MASTER_GOLD
		FATAL			("bad pointer to heap");
#else // #ifndef MASTER_GOLD
		Msg				("! bad pointer to heap");
#endif // #ifndef MASTER_GOLD
		break;
	case _HEAPBADBEGIN:
#ifndef MASTER_GOLD
		FATAL			("bad start of heap");
#else // #ifndef MASTER_GOLD
		Msg				("! bad start of heap");
#endif // #ifndef MASTER_GOLD
		break;
	case _HEAPBADNODE:
#ifndef MASTER_GOLD
		FATAL			("bad node in heap");
#else // #ifndef MASTER_GOLD
		Msg				("! bad node in heap");
#endif // #ifndef MASTER_GOLD
		break;
	}
	return (u32) total;
}

u32		xrMemory::mem_usage		(u32* pBlocksUsed, u32* pBlocksFree)
{
	return				(mem_usage_impl((HANDLE)_get_heap_handle(),pBlocksUsed,pBlocksFree));
}
