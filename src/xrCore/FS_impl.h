#ifndef FS_IMPL_H_INCLUDED
#define FS_IMPL_H_INCLUDED

// 1: default
// 1.5: check next chunk first heuristics
// 2: vector population heuristics
// 3: dynamic map population heuristics

#define FIND_CHUNK_HEU
//#define FIND_CHUNK_STD
//#define FIND_CHUNK_VEC
//#define FIND_CHUNK_MAP

// Uncomment to log time of find_chunk (search
//#define FIND_CHUNK_BENCHMARK_ENABLE

#ifdef FIND_CHUNK_BENCHMARK_ENABLE
struct find_chunk_counter
{
    CTimer timer{};

    std::atomic<u32> calls{};
    std::atomic<float> seconds{};

    find_chunk_counter() = default;

    void flush() const
    {
        Msg("find_chunk: [calls: %u] [seconds: %f]", calls.load(), seconds.load());
    }
};

extern XRCORE_API find_chunk_counter g_find_chunk_counter;

struct find_chunk_auto_timer_t
{
    find_chunk_auto_timer_t()
    {
        ++g_find_chunk_counter.calls;
        g_find_chunk_counter.timer.Start();
    }

    ~find_chunk_auto_timer_t()
    {
        // XXX: replace with operator+= after C++20
        const float old = g_find_chunk_counter.seconds.load();
        g_find_chunk_counter.seconds = old + g_find_chunk_counter.timer.GetElapsed_sec();
    }
};

#define FIND_CHUNK_AUTO_TIMER find_chunk_auto_timer_t __find_chunk_auto_timer
#define FIND_CHUNK_COUNTER_FLUSH() g_find_chunk_counter.flush()
#else
#define FIND_CHUNK_AUTO_TIMER
#define FIND_CHUNK_COUNTER_FLUSH()
#endif // FIND_CHUNK_BENCHMARK_ENABLE

#ifdef FIND_CHUNK_STD

struct IReaderBase_Test
{
};

template <typename T>
IC size_t IReaderBase<T>::find_chunk(u32 ID, bool* bCompressed)
{
    FIND_CHUNK_AUTO_TIMER;

    u32 dwType;
    size_t dwSize = 0;

    rewind();
    while (!eof())
    {
        dwType = r_u32();
        dwSize = r_u32();
        if ((dwType & (~CFS_CompressMark)) == ID)
        {
            VERIFY(impl().tell() + dwSize <= impl().length());
            if (bCompressed)
                *bCompressed = dwType & CFS_CompressMark;
            return dwSize;
        }
        else
            impl().advance(dwSize);
    }

    return 0;
}

#endif // #ifdef FIND_CHUNK_STD

#ifdef FIND_CHUNK_HEU

struct IReaderBase_Test
{
};

#pragma warning(push)
#pragma warning(disable : 4701)
template <typename T>
IC size_t IReaderBase<T>::find_chunk(u32 ID, bool* bCompressed)
{
    FIND_CHUNK_AUTO_TIMER;

    u32 dwType;
    size_t dwSize = 0;

    bool success = false;

    if (m_last_pos != 0)
    {
        impl().seek(m_last_pos);
        dwType = r_u32();
        dwSize = r_u32();

        if ((dwType & (~CFS_CompressMark)) == ID)
        {
            success = true;
        }
    }

    if (!success)
    {
        rewind();
        while (!eof())
        {
            dwType = r_u32();
            dwSize = r_u32();
            if ((dwType & (~CFS_CompressMark)) == ID)
            {
                success = true;
                break;
            }
            else
            {
                impl().advance(dwSize);
            }
        }

        if (!success)
        {
            m_last_pos = 0;
            return 0;
        }
    }

    VERIFY(impl().tell() + dwSize <= impl().length());
    if (bCompressed)
        *bCompressed = dwType & CFS_CompressMark;

    const size_t dwPos = impl().tell();
    if (dwPos + dwSize < impl().length())
    {
        m_last_pos = dwPos + dwSize;
    }
    else
    {
        m_last_pos = 0;
    }

    return dwSize;
}

#pragma warning(pop)

#endif // #ifdef FIND_CHUNK_HEU

#ifdef FIND_CHUNK_VEC

#include "xrServerEntities/associative_vector.h"

struct IReaderBase_Test
{
    typedef associative_vector<u32, size_t> id2pos_container;
    id2pos_container id2pos;
};

template <typename T>
IC size_t IReaderBase<T>::find_chunk(u32 ID, bool* bCompressed)
{
    FIND_CHUNK_AUTO_TIMER;

    u32 dwType;
    size_t dwSize = 0;

    if (!m_test)
    {
        m_test = xr_new<IReaderBase_Test>();

        rewind();
        int num_chunks = 0;
        while (!eof())
        {
            r_u32();
            impl().advance(r_u32());
            ++num_chunks;
        }

        ((std::vector<std::pair<u32, size_t>>*)&m_test->id2pos)->reserve(num_chunks);

        rewind();
        while (!eof())
        {
            u32 dwPos = impl().tell();

            dwType = r_u32();
            dwSize = r_u32();

            u32 dwId = dwType & (~CFS_CompressMark);
            VERIFY(impl().tell() + dwSize <= impl().length());

            m_test->id2pos.insert(IReaderBase_Test::id2pos_container::value_type(dwId, dwPos));

            impl().advance(dwSize);
        }
    }

    IReaderBase_Test::id2pos_container::iterator it = m_test->id2pos.find(ID);
    if (it != m_test->id2pos.end())
    {
        impl().seek(it->second);
        dwType = r_u32();
        dwSize = r_u32();

        VERIFY((dwType & (~CFS_CompressMark)) == ID);

        if (bCompressed)
            *bCompressed = dwType & CFS_CompressMark;
        return dwSize;
    }

    return 0;
}

#endif // #ifdef FIND_CHUNK_VEC

#ifdef FIND_CHUNK_MAP

#include "xrServerEntities/associative_vector.h"

struct IReaderBase_Test
{
    typedef xr_unordered_map<u32, size_t> id2pos_container;

    id2pos_container id2pos;
    size_t last_pos;
};

template <typename T>
IC size_t IReaderBase<T>::find_chunk(u32 ID, bool* bCompressed)
{
    FIND_CHUNK_AUTO_TIMER;

    u32 dwType;
    size_t dwSize = 0;

    if (!m_test)
    {
        m_test = xr_new<IReaderBase_Test>();
        m_test->last_pos = 0;
    }

    IReaderBase_Test::id2pos_container::iterator it = m_test->id2pos.find(ID);
    if (it != m_test->id2pos.end())
    {
        impl().seek(it->second);
        dwType = r_u32();
        dwSize = r_u32();

        VERIFY((dwType & (~CFS_CompressMark)) == ID);

        if (bCompressed)
            *bCompressed = dwType & CFS_CompressMark;
        return dwSize;
    }

    impl().seek(m_test->last_pos);
    while (!eof())
    {
        size_t dwPos = impl().tell();

        dwType = r_u32();
        dwSize = r_u32();

        VERIFY(impl().tell() + dwSize <= impl().length());

        u32 dwId = dwType & (~CFS_CompressMark);

        m_test->id2pos.insert(IReaderBase_Test::id2pos_container::value_type(dwId, dwPos));

        if (dwId == ID)
        {
            if (bCompressed)
                *bCompressed = dwType & CFS_CompressMark;

            m_test->last_pos = impl().tell() + dwSize;
            return dwSize;
        }
        else
        {
            impl().advance(dwSize);
        }
    }

    m_test->last_pos = impl().tell();
    return 0;
}

#endif // #ifdef FIND_CHUNK_MAP

#endif // #ifndef FS_IMPL_H_INCLUDED
