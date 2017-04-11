#include "stdafx.h"
#include "xrMemory_align.h"
#include "xrMemory_pure.h"

#ifndef __BORLANDC__

#ifndef DEBUG_MEMORY_MANAGER
#define debug_mode 0
#endif

#ifdef DEBUG_MEMORY_MANAGER
XRCORE_API void* g_globalCheckAddr = nullptr;
extern void save_stack_trace();
#endif

MEMPOOL mem_pools[mem_pools_count];

// MSVC
ICF u8* acc_header(void* P) { return (u8*)P - 1; }
ICF u32 get_header(void* P) { return (u32)*acc_header(P); }
ICF u32 get_pool(size_t size)
{
    u32 pid = u32(size / mem_pools_ebase);
    if (pid >= mem_pools_count)
        return mem_generic;
    return pid;
}

static bool g_use_pure_alloc = false;

#ifdef DEBUG_MEMORY_NAME
void* xrMemory::mem_alloc(size_t size, const char* _name)
#else
void* xrMemory::mem_alloc(size_t size)
#endif
{
    stat_calls++;
    static bool g_use_pure_alloc_initialized = false;
    if (!g_use_pure_alloc_initialized)
    {
        g_use_pure_alloc_initialized = true;
        g_use_pure_alloc = Core.PluginMode || strstr(GetCommandLine(), "-pure_alloc");
    }
    if (g_use_pure_alloc)
    {
        void* result = malloc(size);
#ifdef USE_MEMORY_MONITOR
        memory_monitor::monitor_alloc(result, size, _name);
#endif
        return result;
    }
#ifdef DEBUG_MEMORY_MANAGER
    if (mem_initialized)
        debug_cs.Enter();
#endif
    u32 _footer = debug_mode ? 4 : 0;
    void* _ptr = 0;
    if (!mem_initialized)
    {
        // generic
        void* _real = xr_aligned_offset_malloc(1 + size + _footer, 16, 0x1);
        _ptr = (void*)(((u8*)_real) + 1);
        *acc_header(_ptr) = mem_generic;
    }
    else
    {
#ifdef DEBUG_MEMORY_MANAGER
        save_stack_trace();
#endif
        // accelerated
        u32 pool = get_pool(1 + size + _footer);
        if (mem_generic == pool)
        {
            // generic
            void* _real = xr_aligned_offset_malloc(1 + size + _footer, 16, 0x1);
            _ptr = (void*)(((u8*)_real) + 1);
            *acc_header(_ptr) = mem_generic;
        }
        else
        {
            // pooled
            void* _real = mem_pools[pool].create();
            _ptr = (void*)(((u8*)_real) + 1);
            *acc_header(_ptr) = (u8)pool;
        }
    }

#ifdef DEBUG_MEMORY_MANAGER
    if (debug_mode)
        dbg_register(_ptr, size, _name);
    if (mem_initialized)
        debug_cs.Leave();
#endif
#ifdef USE_MEMORY_MONITOR
    memory_monitor::monitor_alloc(_ptr, size, _name);
#endif
    return _ptr;
}

void xrMemory::mem_free(void* P)
{
    stat_calls++;
#ifdef USE_MEMORY_MONITOR
    memory_monitor::monitor_free(P);
#endif
    if (g_use_pure_alloc)
    {
        free(P);
        return;
    }
#ifdef DEBUG_MEMORY_MANAGER
    if (g_globalCheckAddr == P)
        DEBUG_BREAK;
    if (mem_initialized)
        debug_cs.Enter();
#endif
    if (debug_mode)
        dbg_unregister(P);
    u32 pool = get_header(P);
    void* _real = (void*)((u8*)P - 1);
    if (mem_generic == pool)
    {
        // generic
        xr_aligned_free(_real);
    }
    else
    {
        // pooled
        VERIFY2(pool < mem_pools_count, "Memory corruption");
        mem_pools[pool].destroy(_real);
    }
#ifdef DEBUG_MEMORY_MANAGER
    if (mem_initialized)
        debug_cs.Leave();
#endif
}

extern BOOL g_bDbgFillMemory;

#ifdef DEBUG_MEMORY_NAME
void* xrMemory::mem_realloc(void* P, size_t size, const char* _name)
#else
void* xrMemory::mem_realloc(void* P, size_t size)
#endif
{
    stat_calls++;
    if (g_use_pure_alloc)
    {
        void* result = realloc(P, size);
#ifdef USE_MEMORY_MONITOR
        memory_monitor::monitor_free(P);
        memory_monitor::monitor_alloc(result, size, _name);
#endif
        return result;
    }
    if (!P)
    {
#ifdef DEBUG_MEMORY_NAME
        return mem_alloc(size, _name);
#else
        return mem_alloc(size);
#endif
    }
#ifdef DEBUG_MEMORY_MANAGER
    if (g_globalCheckAddr == P)
        DEBUG_BREAK;
    if (mem_initialized)
        debug_cs.Enter();
#endif
    u32 p_current = get_header(P);
    u32 newPool = get_pool(1 + size + (debug_mode ? 4 : 0));
    u32 p_mode;
    if (mem_generic == p_current)
    {
        if (newPool < p_current)
            p_mode = 2;
        else
            p_mode = 0;
    }
    else
        p_mode = 1;
    void* _real = (void*)((u8*)P - 1);
    void* _ptr = NULL;
    if (!p_mode)
    {
        u32 _footer = debug_mode ? 4 : 0;
#ifdef DEBUG_MEMORY_MANAGER
        if (debug_mode)
        {
            g_bDbgFillMemory = false;
            dbg_unregister(P);
            g_bDbgFillMemory = true;
        }
#endif
        void* _real2 = xr_aligned_offset_realloc(_real, 1 + size + _footer, 16, 0x1);
        _ptr = (void*)((u8*)_real2 + 1);
        *acc_header(_ptr) = mem_generic;
#ifdef DEBUG_MEMORY_MANAGER
        if (debug_mode)
            dbg_register(_ptr, size, _name);
#endif
#ifdef USE_MEMORY_MONITOR
        memory_monitor::monitor_free(P);
        memory_monitor::monitor_alloc(_ptr, size, _name);
#endif
    }
    else if (p_mode == 1)
    {
        // pooled realloc
        R_ASSERT2(p_current < mem_pools_count, "Memory corruption");
        u32 s_current = mem_pools[p_current].get_element();
        u32 s_dest = (u32)size;
        void* p_old = P;
#ifdef DEBUG_MEMORY_NAME
        void* p_new = mem_alloc(size, _name);
#else
        void* p_new = mem_alloc(size);
#endif
        // Igor: Reserve 1 byte for xrMemory header
        // Don't bother in this case?
        memcpy(p_new, p_old, std::min(s_current - 1, s_dest));
        mem_free(p_old);
        _ptr = p_new;
    }
    else if (p_mode == 2)
    {
        // relocate into another mmgr(pooled) from real
        void* p_old = P;
#ifdef DEBUG_MEMORY_NAME
        void* p_new = mem_alloc(size, _name);
#else
        void* p_new = mem_alloc(size);
#endif
        memcpy(p_new, p_old, (u32)size);
        mem_free(p_old);
        _ptr = p_new;
    }
#ifdef DEBUG_MEMORY_MANAGER
    if (mem_initialized)
        debug_cs.Leave();
    if (g_globalCheckAddr == _ptr)
        DEBUG_BREAK;
#endif
    return _ptr;
}

#endif // __BORLANDC__
