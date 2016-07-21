#pragma once
#ifndef xrMemory_POOLh
#define xrMemory_POOLh
#include "_types.h"
#include "xrCommon/inlining_macros.h"

class xrMemory;
class Lock;

class MEMPOOL
{
#ifdef DEBUG_MEMORY_MANAGER
    friend class xrMemory;
#endif // DEBUG_MEMORY_MANAGER

public:
    MEMPOOL();
    ~MEMPOOL();

    void _initialize(u32 _element, u32 _sector, u32 _header);

    ICF u32 get_block_count() { return block_count; }
    ICF u32 get_element() { return s_element; }
    void* create();
    void destroy(void*& P);

private:
    // noncopyable
    MEMPOOL(const MEMPOOL&) = delete;
    void operator=(const MEMPOOL&) = delete;

    ICF void** access(void* P) { return (void**)((void*)(P)); }
    void block_create();

    Lock* pcs;
    u32 s_sector; // large-memory sector size
    u32 s_element; // element size, for example 32
    u32 s_count; // element count = [s_sector/s_element]
    u32 s_offset; // header size
    u32 block_count; // block count
    u8* list;
};

#endif // include guard
