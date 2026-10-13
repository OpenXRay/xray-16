#pragma once

#include "xrCore/xrMemory.h"
#include "xrCommon/xr_vector.h"

#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

namespace xray::render::framegraph {

class FrameArena {
public:
    struct Stats {
        u32 lastUsed = 0;
        u32 peak = 0;
        u32 capacity = 0;
        u32 lastFallbackAllocs = 0;
    };

    explicit FrameArena(u32 capacity = 256u * 1024u)
        : m_capacity(capacity)
    {
        m_begin = static_cast<u8*>(xr_malloc(m_capacity));
        m_cur = m_begin;
        m_stats.capacity = m_capacity;
    }

    FrameArena(const FrameArena&) = delete;
    FrameArena& operator=(const FrameArena&) = delete;

    ~FrameArena()
    {
        Reset();
        xr_free(m_begin);
    }

    void* Alloc(size_t size, size_t align)
    {
        u8* aligned = reinterpret_cast<u8*>(
            (reinterpret_cast<uintptr_t>(m_cur) + (align - 1)) & ~(uintptr_t(align) - 1));
        if (aligned + size <= m_begin + m_capacity)
        {
            m_cur = aligned + size;
            return aligned;
        }
        return AllocFallback(size, align);
    }

    template <typename T, typename... Args>
    T* Make(Args&&... args)
    {
        static_assert(sizeof(T) <= 8192, "frame arena objects must stay small");
        void* p = Alloc(sizeof(T), alignof(T));
        T* obj = new (p) T(std::forward<Args>(args)...);
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            auto* node = static_cast<DtorNode*>(Alloc(sizeof(DtorNode), alignof(DtorNode)));
            node->obj = obj;
            node->fn = [](void* o) { static_cast<T*>(o)->~T(); };
            node->next = m_dtors;
            m_dtors = node;
        }
        return obj;
    }

    void Reset()
    {
        for (DtorNode* n = m_dtors; n; n = n->next)
            n->fn(n->obj);
        m_dtors = nullptr;

        for (u8* raw : m_fallbackBlocks)
            xr_free(raw);
        m_fallbackBlocks.clear();

        m_stats.lastUsed = static_cast<u32>(m_cur - m_begin) + m_fallbackBytes;
        if (m_stats.lastUsed > m_stats.peak)
            m_stats.peak = m_stats.lastUsed;
        m_stats.lastFallbackAllocs = m_fallbackAllocs;

        if (m_fallbackAllocs && m_stats.peak > m_capacity)
        {
            xr_free(m_begin);
            m_capacity = m_stats.peak + m_stats.peak / 2;
            m_begin = static_cast<u8*>(xr_malloc(m_capacity));
        }
        m_stats.capacity = m_capacity;
        m_cur = m_begin;
        m_fallbackAllocs = 0;
        m_fallbackBytes = 0;
    }

    const Stats& GetStats() const { return m_stats; }

private:
    struct DtorNode {
        void (*fn)(void*);
        void* obj;
        DtorNode* next;
    };

    void* AllocFallback(size_t size, size_t align)
    {
        ++m_fallbackAllocs;
        m_fallbackBytes += static_cast<u32>(size);
        u8* raw = static_cast<u8*>(xr_malloc(size + align));
        m_fallbackBlocks.push_back(raw);
        return reinterpret_cast<u8*>(
            (reinterpret_cast<uintptr_t>(raw) + (align - 1)) & ~(uintptr_t(align) - 1));
    }

    u8* m_begin = nullptr;
    u8* m_cur = nullptr;
    u32 m_capacity = 0;
    DtorNode* m_dtors = nullptr;
    xr_vector<u8*> m_fallbackBlocks;
    u32 m_fallbackAllocs = 0;
    u32 m_fallbackBytes = 0;
    Stats m_stats;
};

}
