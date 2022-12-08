#include "stdafx.h"
#pragma hdrstop // huh?
#include "Threading/Lock.hpp"

using namespace std;

XRCORE_API smem_container* g_pSharedMemoryContainer = NULL;

smem_container::smem_container()
#ifdef CONFIG_PROFILE_LOCKS
    : lock(MUTEX_PROFILE_ID(smem_container))
#endif // CONFIG_PROFILE_LOCKS
{
}

smem_container::~smem_container()
{
    clean();
}

smem_value* smem_container::dock(u32 dwCRC, u32 dwLength, void* ptr)
{
    VERIFY(dwCRC && dwLength && ptr);

    ScopeLock scope(&lock);
    smem_value* result = 0;

    // search a place to insert
    u8 storage[4 * sizeof(u32)];
    smem_value* value = (smem_value*)storage;
    value->dwReference = 0;
    value->dwCRC = dwCRC;
    value->dwLength = dwLength;
    cdb::iterator it = std::lower_bound(container.begin(), container.end(), value, smem_search);
    cdb::iterator saved_place = it;
    if (container.end() != it)
    {
        // supposedly found
        for (;; ++it)
        {
            if (it == container.end())
                break;
            if ((*it)->dwCRC != dwCRC)
                break;
            if ((*it)->dwLength != dwLength)
                break;
            if (0 == memcmp((*it)->value, ptr, dwLength))
            {
                // really found
                result = *it;
                break;
            }
        }
    }

    // if not found - create new entry
    if (0 == result)
    {
        result = (smem_value*)xr_malloc(4 * sizeof(u32) + dwLength);
        result->dwReference = 0;
        result->dwCRC = dwCRC;
        result->dwLength = dwLength;
        CopyMemory(result->value, ptr, dwLength);
        container.insert(saved_place, result);
    }

    // exit
    return result;
}

void smem_container::clean()
{
    ScopeLock scope(&lock);
    cdb::iterator it = container.begin();
    cdb::iterator end = container.end();
    for (; it != end; ++it)
        if (0 == (*it)->dwReference)
            xr_free(*it);
    container.erase(remove(container.begin(), container.end(), (smem_value*)0), container.end());
    if (container.empty())
        container.clear();
}

void smem_container::dump()
{
    ScopeLock scope(&lock);
    cdb::iterator it = container.begin();
    cdb::iterator end = container.end();
    FILE* F = fopen("x:\\$smem_dump$.txt", "w");
    for (; it != end; ++it)
        fprintf(F, "%4u : crc[%6x], %u bytes\n", (*it)->dwReference, (*it)->dwCRC, (*it)->dwLength);
    fclose(F);
}

u32 smem_container::stat_economy()
{
    s64 counter = 0;
    {
        ScopeLock scope(&lock);
        cdb::iterator it = container.begin();
        cdb::iterator end = container.end();
        counter -= sizeof(*this);
        counter -= sizeof(cdb::allocator_type);
        const int node_size = 20; // XXX: refactor
        for (; it != end; ++it)
        {
            counter -= 16;
            counter -= node_size;
            counter += s64((s64((*it)->dwReference) - 1) * s64((*it)->dwLength));
        }
    }

    return u32(s64(counter) / s64(1024));
}
