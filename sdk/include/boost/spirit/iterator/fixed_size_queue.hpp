/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2001, Daniel C. Nuffer
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#ifndef FIXED_SIZE_QUEUE
#define FIXED_SIZE_QUEUE

#include <cstdlib>
#include <iterator>
#include <cstddef>

#include "boost/spirit/core/assert.hpp" // for BOOST_SPIRIT_ASSERT

// FIXES for broken compilers
#include <boost/config.hpp>

//// fix for cygwin
//#ifdef size_t
//#undef size_t
//namespace std { typedef unsigned size_t; }
//#endif

#define BOOST_SPIRIT_ASSERT_FSQ_SIZE \
    BOOST_SPIRIT_ASSERT(((m_tail + N + 1) - m_head) % (N+1) == m_size % (N+1)) \
    /**/

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
template <typename T, std::size_t N>
class fixed_size_queue_iterator;

template <typename T, std::size_t N>
class fixed_size_queue
{
public:
    fixed_size_queue();
    fixed_size_queue(const fixed_size_queue& x);
    fixed_size_queue& operator=(const fixed_size_queue& x);
    ~fixed_size_queue();

    void push_back(const T& e);
    void push_front(const T& e);
    void serve(T& e);
    void pop_front();

    bool empty()
    {
        return m_size == 0;
    }

    bool full()
    {
        return m_size == N;
    }


    typedef fixed_size_queue_iterator<T, N> iterator;

    iterator begin()
    {
        return iterator(this, m_head);
    }

    iterator end()
    {
        return iterator(this, m_tail);
    }

    std::size_t size()
    {
        return m_size;
    }

    T& front()
    {
        return m_queue[m_head];
    }

private:

    //friend class iterator;
    friend class fixed_size_queue_iterator<T, N>;

    std::size_t m_head;
    std::size_t m_tail;
    std::size_t m_size;
    T m_queue[N+1];

};


template <typename T, std::size_t N>
class fixed_size_queue_iterator
{
public:

    typedef std::forward_iterator_tag iterator_category;
    typedef T value_type;
    typedef std::ptrdiff_t difference_type;
    typedef T* pointer;
    typedef T& reference;
    typedef fixed_size_queue<T, N> queue;

    fixed_size_queue_iterator(queue* q, std::size_t p)
        : m_queue(q)
        , pos(p)
    {}

    fixed_size_queue_iterator& operator++()
    {
        ++pos;
        if (pos == N+1)
            pos = 0;

        return *this;
    }

    fixed_size_queue_iterator operator++(int)
    {
        fixed_size_queue_iterator tmp = *this;
        ++*this;
        return tmp;
    }

    T& operator*()
    {
        return m_queue->m_queue[pos];
    }

    bool operator==(const fixed_size_queue_iterator& x)
    {
        return pos == x.pos && m_queue == x.m_queue;
    }

    bool operator<(const fixed_size_queue_iterator& x)
    {
        std::size_t p1 = pos < m_queue->m_head ? pos + N : pos;
        std::size_t p2 = x.pos < x.m_queue->m_head ? x.pos + N : x.pos;
        return p1 < p2;
    }

    bool operator!=(const fixed_size_queue_iterator& x)
    {
        return !(*this == x);
    }

private:
    queue* m_queue;
    std::size_t pos;
};

template <typename T, std::size_t N>
inline
fixed_size_queue<T, N>::fixed_size_queue()
    : m_head(0)
    , m_tail(0)
    , m_size(0)
{
    BOOST_SPIRIT_ASSERT(m_size <= N+1);
    BOOST_SPIRIT_ASSERT_FSQ_SIZE;
    BOOST_SPIRIT_ASSERT(m_head <= N+1);
    BOOST_SPIRIT_ASSERT(m_tail <= N+1);
}

template <typename T, std::size_t N>
inline
fixed_size_queue<T, N>::fixed_size_queue(const fixed_size_queue& x)
    : m_head(x.m_head)
    , m_tail(x.m_tail)
    , m_size(x.m_size)
{
    copy(x.begin(), x.end(), begin());
    BOOST_SPIRIT_ASSERT(m_size <= N+1);
    BOOST_SPIRIT_ASSERT_FSQ_SIZE;
    BOOST_SPIRIT_ASSERT(m_head <= N+1);
    BOOST_SPIRIT_ASSERT(m_tail <= N+1);
}

template <typename T, std::size_t N>
inline fixed_size_queue<T, N>&
fixed_size_queue<T, N>::operator=(const fixed_size_queue& x)
{
    if (this != &x)
    {
        m_head = x.m_head;
        m_tail = x.m_tail;
        m_size = x.m_size;
        copy(x.begin(), x.end(), begin());
    }
    BOOST_SPIRIT_ASSERT(m_size <= N+1);
    BOOST_SPIRIT_ASSERT_FSQ_SIZE;
    BOOST_SPIRIT_ASSERT(m_head <= N+1);
    BOOST_SPIRIT_ASSERT(m_tail <= N+1);

    return *this;
}

template <typename T, std::size_t N>
inline
fixed_size_queue<T, N>::~fixed_size_queue()
{
    BOOST_SPIRIT_ASSERT(m_size <= N+1);
    BOOST_SPIRIT_ASSERT_FSQ_SIZE;
    BOOST_SPIRIT_ASSERT(m_head <= N+1);
    BOOST_SPIRIT_ASSERT(m_tail <= N+1);
}

template <typename T, std::size_t N>
inline void
fixed_size_queue<T, N>::push_back(const T& e)
{
    BOOST_SPIRIT_ASSERT(m_size <= N+1);
    BOOST_SPIRIT_ASSERT_FSQ_SIZE;
    BOOST_SPIRIT_ASSERT(m_head <= N+1);
    BOOST_SPIRIT_ASSERT(m_tail <= N+1);

    BOOST_SPIRIT_ASSERT(!full());

    m_queue[m_tail] = e;
    ++m_size;
    ++m_tail;
    if (m_tail == N+1)
        m_tail = 0;


    BOOST_SPIRIT_ASSERT(m_size <= N+1);
    BOOST_SPIRIT_ASSERT_FSQ_SIZE;
    BOOST_SPIRIT_ASSERT(m_head <= N+1);
    BOOST_SPIRIT_ASSERT(m_tail <= N+1);
}

template <typename T, std::size_t N>
inline void
fixed_size_queue<T, N>::push_front(const T& e)
{
    BOOST_SPIRIT_ASSERT(m_size <= N+1);
    BOOST_SPIRIT_ASSERT_FSQ_SIZE;
    BOOST_SPIRIT_ASSERT(m_head <= N+1);
    BOOST_SPIRIT_ASSERT(m_tail <= N+1);

    BOOST_SPIRIT_ASSERT(!full());

    if (m_head == 0)
        m_head = N;
    else
        --m_head;

    m_queue[m_head] = e;
    ++m_size;

    BOOST_SPIRIT_ASSERT(m_size <= N+1);
    BOOST_SPIRIT_ASSERT_FSQ_SIZE;
    BOOST_SPIRIT_ASSERT(m_head <= N+1);
    BOOST_SPIRIT_ASSERT(m_tail <= N+1);
}


template <typename T, std::size_t N>
inline void
fixed_size_queue<T, N>::serve(T& e)
{
    BOOST_SPIRIT_ASSERT(m_size <= N+1);
    BOOST_SPIRIT_ASSERT_FSQ_SIZE;
    BOOST_SPIRIT_ASSERT(m_head <= N+1);
    BOOST_SPIRIT_ASSERT(m_tail <= N+1);

    e = m_queue[m_head];
    pop_front();
}



template <typename T, std::size_t N>
inline void
fixed_size_queue<T, N>::pop_front()
{
    BOOST_SPIRIT_ASSERT(m_size <= N+1);
    BOOST_SPIRIT_ASSERT_FSQ_SIZE;
    BOOST_SPIRIT_ASSERT(m_head <= N+1);
    BOOST_SPIRIT_ASSERT(m_tail <= N+1);

    ++m_head;
    if (m_head == N+1)
        m_head = 0;
    --m_size;

    BOOST_SPIRIT_ASSERT(m_size <= N+1);
    BOOST_SPIRIT_ASSERT_FSQ_SIZE;
    BOOST_SPIRIT_ASSERT(m_head <= N+1);
    BOOST_SPIRIT_ASSERT(m_tail <= N+1);
}

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#undef BOOST_SPIRIT_ASSERT_FSQ_SIZE

#endif
