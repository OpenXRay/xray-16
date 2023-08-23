#include "stdafx.h"
#pragma hdrstop // huh?
#include "Threading/Lock.hpp"

using namespace std;

XRCORE_API smem_container* g_pSharedMemoryContainer = nullptr;

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
    smem_value* result = nullptr;

    // search a place to insert
    u8 storage[sizeof(smem_value)];
    smem_value* value = (smem_value*)storage;
    value->dwReference = 0;
    value->dwCRC = dwCRC;
    value->dwLength = dwLength;
    auto it = std::lower_bound(container.begin(), container.end(), value, smem_search);
    const auto saved_place = it;
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
    if (nullptr == result)
    {
        result = (smem_value*)xr_malloc(sizeof(smem_value) + dwLength);
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
    for (auto& v : container)
        if (0 == v->dwReference)
            xr_free(v);
    container.erase(remove(container.begin(), container.end(), (smem_value*)nullptr), container.end());
    if (container.empty())
        container.clear();
}

void smem_container::dump()
{
    ScopeLock scope(&lock);
    FILE* F = fopen("x:\\$smem_dump$.txt", "w");
    for (auto& v : container)
        fprintf(F, "%4u : crc[%6x], %u bytes\n", v->dwReference, v->dwCRC, v->dwLength);
    fclose(F);
}

size_t smem_container::stat_economy() const
{
    ptrdiff_t counter = 0;
    {
        ScopeLock scope(&lock);
        counter -= sizeof(*this);
        counter -= sizeof(cdb::allocator_type);
        constexpr ptrdiff_t node_size = 20; // XXX: refactor
        for (auto& v : container)
        {
            counter -= 16;
            counter -= node_size;
            counter += ptrdiff_t((ptrdiff_t(v->dwReference) - 1) * ptrdiff_t(v->dwLength));
        }
    }

    return size_t(counter / ptrdiff_t(1024));
}
