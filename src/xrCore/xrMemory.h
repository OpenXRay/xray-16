#pragma once

#include "Memory/memory_allocator_options.h"

#ifndef M_BORLAND
#if 0 // def DEBUG
#define DEBUG_MEMORY_MANAGER
#endif // DEBUG
#endif // M_BORLAND

#ifdef DEBUG_MEMORY_MANAGER
XRCORE_API extern BOOL g_bMEMO;
#ifndef DEBUG_MEMORY_NAME
#define DEBUG_MEMORY_NAME
#endif // DEBUG_MEMORY_NAME
extern XRCORE_API void dump_phase();
#define DUMP_PHASE    \
    do                \
    {                 \
        dump_phase(); \
    } while (0)
#else // DEBUG_MEMORY_MANAGER
#define DUMP_PHASE \
    do             \
    {              \
    } while (0)
#endif // DEBUG_MEMORY_MANAGER

#include "xrMemory_POOL.h"

class XRCORE_API xrMemory
{
public:
    struct mdbg
    {
        void* _p;
        size_t _size;
        const char* _name;
        u32 _dummy;
    };

public:
    xrMemory();
    void _initialize(bool _debug_mode = false);
    void _destroy();

#ifdef DEBUG_MEMORY_MANAGER
    BOOL debug_mode;
    Lock debug_cs;
    std::vector<mdbg> debug_info;
    u32 debug_info_update;
    u32 stat_strcmp;
    u32 stat_strdock;
#endif // DEBUG_MEMORY_MANAGER

    u32 stat_calls;
    s32 stat_counter;

public:
    void dbg_register(void* _p, size_t _size, const char* _name);
    void dbg_unregister(void* _p);
    void dbg_check();

    size_t mem_usage();
    void mem_compact();
    void mem_counter_set(u32 _val) { stat_counter = _val; }
    u32 mem_counter_get() { return stat_counter; }
#ifdef DEBUG_MEMORY_NAME
    void mem_statistic(const char* fn);
#endif // DEBUG_MEMORY_NAME
    void* mem_alloc(size_t size);
    void* mem_realloc(void* p, const size_t size);
    void mem_free(void* p);
};

extern XRCORE_API xrMemory Memory;

#undef ZeroMemory
#undef CopyMemory
#undef FillMemory
#define ZeroMemory(a, b) memset(a, 0, b)
#define CopyMemory(a, b, c) memcpy(a, b, c)
#define FillMemory(a, b, c) memset(a, c, b)

// delete
#ifdef __BORLANDC__
#include "xrMemory_subst_borland.h"
#else
#include "xrMemory_subst_msvc.h"
#endif

// generic "C"-like allocations/deallocations
template <class T>
T* xr_alloc(const size_t count)
{ return (T*)Memory.mem_alloc(count * sizeof(T)); }


template <class T>
void xr_free(T*& P) noexcept
{
    if (P)
    {
        Memory.mem_free((void*)P);
        P = nullptr;
    }
}
inline void* xr_malloc(const size_t size) { return Memory.mem_alloc(size); }
inline void* xr_realloc(void* P, const size_t size) { return Memory.mem_realloc(P, size); }

XRCORE_API pstr xr_strdup(pcstr string);

// Global new/delete override
#ifndef NO_XRNEW
#if !defined(BUILDING_XRMISC_LIB) && defined(_MSC_VER)
#pragma comment(lib, "xrMisc") // Attempt to force the TU to include our version.
#endif
// XXX: Implementations of operator new/delete are in xrMisc/xrMemory.cpp, since they need
// to be in a static link library.
void* operator new(const size_t size);
void operator delete(void* p) noexcept;
void* operator new[](const size_t size);
void operator delete[](void* p) noexcept;
#endif

// POOL-ing
const u32 mem_pools_count = 54;
const u32 mem_pools_ebase = 16;
const u32 mem_generic = mem_pools_count + 1;
extern MEMPOOL mem_pools[mem_pools_count];
extern bool mem_initialized;

XRCORE_API void log_vminfo();
