#pragma once
#include "xrDebug_macros.h"

template <class T, size_t granularity>
class poolSS
{
    T* list;
    xr_vector<T*> blocks;

    T** access(T* P) { return (T**)LPVOID(P); }

    void block_create()
    {
        constexpr size_t count = granularity - 1; // minus one. Correct. See partition logic below.

        // Allocate
        VERIFY(nullptr == list);
        list = xr_alloc<T>(granularity);
        blocks.push_back(list);

        // Partition
        for (size_t it = 0; it < count; it++)
        {
            T* E = list + it;
            *access(E) = E + 1;
        }
        *access(list + count) = nullptr;
    }

public:
    poolSS() : list(nullptr) {}
    ~poolSS()
    {
        for (auto& block : blocks)
            xr_free(block);
    }
    T* create()
    {
        if (0 == list)
            block_create();

        T* E = list;
        list = *access(list);
        return new (E) T();
    }
    void destroy(T*& P)
    {
        P->~T();
        *access(P) = list;
        list = P;
        P = nullptr;
    }
    void clear()
    {
        list = nullptr;
        for (auto& block : blocks)
            xr_free(block);
        blocks.clear();
    }
};
