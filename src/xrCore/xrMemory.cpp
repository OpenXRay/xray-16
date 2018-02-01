#include "stdafx.h"
#pragma hdrstop

#include "xrsharedmem.h"
#include "xrCore/_std_extensions.h"

xrMemory Memory;
// Also used in src\xrCore\xrDebug.cpp to prevent use of g_pStringContainer before it initialized
bool shared_str_initialized = false;

// fake fix of memory corruptions in multiplayer game :(
// XXX nitrocaster: to be removed
XRCORE_API bool g_allow_heap_min = true;

xrMemory::xrMemory()
{
}

void xrMemory::_initialize()
{
    stat_calls = 0;

    g_pStringContainer = new str_container();
    shared_str_initialized = true;
    g_pSharedMemoryContainer = new smem_container();
}

void xrMemory::_destroy()
{
    xr_delete(g_pSharedMemoryContainer);
    xr_delete(g_pStringContainer);
}

XRCORE_API void vminfo(size_t* _free, size_t* reserved, size_t* committed)
{
    MEMORY_BASIC_INFORMATION memory_info;
    memory_info.BaseAddress = 0;
    *_free = *reserved = *committed = 0;
    while (VirtualQuery(memory_info.BaseAddress, &memory_info, sizeof(memory_info)))
    {
        switch (memory_info.State)
        {
        case MEM_FREE: *_free += memory_info.RegionSize; break;
        case MEM_RESERVE: *reserved += memory_info.RegionSize; break;
        case MEM_COMMIT: *committed += memory_info.RegionSize; break;
        }
        memory_info.BaseAddress = (char*)memory_info.BaseAddress + memory_info.RegionSize;
    }
}

XRCORE_API void log_vminfo()
{
    size_t w_free, w_reserved, w_committed;
    vminfo(&w_free, &w_reserved, &w_committed);
    Msg("* [win32]: free[%d K], reserved[%d K], committed[%d K]", w_free / 1024, w_reserved / 1024, w_committed / 1024);
}

size_t xrMemory::mem_usage()
{
    _HEAPINFO hinfo = {};
    int status;
    size_t bytesUsed = 0;
    while ((status = _heapwalk(&hinfo)) == _HEAPOK)
    {
        if (hinfo._useflag == _USEDENTRY)
            bytesUsed += hinfo._size;
    }
    switch (status)
    {
    case _HEAPEMPTY: break;
    case _HEAPEND: break;
    case _HEAPBADPTR: FATAL("bad pointer to heap"); break;
    case _HEAPBADBEGIN: FATAL("bad start of heap"); break;
    case _HEAPBADNODE: FATAL("bad node in heap"); break;
    }
    return bytesUsed;
}

void xrMemory::mem_compact()
{
    RegFlushKey(HKEY_CLASSES_ROOT);
    RegFlushKey(HKEY_CURRENT_USER);
    if (g_allow_heap_min)
        _heapmin();
    HeapCompact(GetProcessHeap(), 0);
    if (g_pStringContainer)
        g_pStringContainer->clean();
    if (g_pSharedMemoryContainer)
        g_pSharedMemoryContainer->clean();
    if (strstr(Core.Params, "-swap_on_compact"))
        SetProcessWorkingSetSize(GetCurrentProcess(), size_t(-1), size_t(-1));
}

void* xrMemory::mem_alloc(size_t size)
{
    stat_calls++;
    return malloc(size);
}

void xrMemory::mem_free(void* P)
{
    stat_calls++;
    free(P);
}

void* xrMemory::mem_realloc(void* P, const size_t size)
{
    stat_calls++;
    return realloc(P, size);
}

// xr_strdup
pstr xr_strdup(pcstr string)
{
    VERIFY(string);
    size_t len = xr_strlen(string) + 1;
    char* memory = (char*)Memory.mem_alloc(len);
    CopyMemory(memory, string, len);
    return memory;
}
