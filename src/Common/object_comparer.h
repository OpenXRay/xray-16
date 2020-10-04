////////////////////////////////////////////////////////////////////////////
//  Module      : object_comparer.h
//  Created     : 13.07.2004
//  Modified    : 13.07.2004
//  Author      : Dmitriy Iassenev
//  Description : Object equality checker
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/FixedVector.h"
#include "xrCore/xrstring.h"
#include "xrCommon/xr_stack.h"

template <typename P>
struct CComparer
{
    template <typename T>
    struct CHelper
    {
        template <bool a>
        IC static bool compare(std::enable_if_t<!a, const T&> a1, const T& a2, const P& p)
        {
            return p(a1, a2);
        }

        template <bool a>
        IC static bool compare(std::enable_if_t<a, const T&> a1, const T& a2, const P& p)
        {
            return CComparer::compare(*a1, *a2, p);
        }
    };

    IC static bool compare(LPCSTR s1, LPCSTR s2, const P& p) { return p(s1, s2); }
    IC static bool compare(pstr s1, pstr s2, const P& p) { return p(s1, s2); }
    IC static bool compare(const shared_str& s1, const shared_str& s2, const P& p) { return p(s1, s2); }
    template <typename T1, typename T2>
    IC static bool compare(const std::pair<T1, T2>& p1, const std::pair<T1, T2>& p2, const P& p)
    {
        return compare(p1.first, p2.first, p) && compare(p1.second, p2.second, p);
    }

    template <typename T, int size>
    IC static bool compare(const svector<T, size>& v1, const svector<T, size>& v2, const P& p)
    {
        if (v1.size() != v2.size())
            return p();

        auto I = v1.cbegin(), J = v2.cbegin();
        auto E = v1.end();
        for (; I != E; ++I, ++J)
            if (!compare(*I, *J, p))
                return false;
        return true;
    }

    template <typename T1, typename T2>
    IC static bool compare(const std::queue<T1, T2>& q1, const std::queue<T1, T2>& q2, const P& p)
    {
        std::queue<T1, T2> lq1 = q1;
        std::queue<T1, T2> lq2 = q2;

        if (lq1.size() != lq2.size())
            return p();

        for (; !lq1.empty(); lq1.pop(), lq2.pop())
            if (!compare(lq1.front(), lq2.front(), p))
                return false;
        return true;
    }

    template <template <typename T1X, typename T2X> class T1, typename T2, typename T3>
    IC static bool compare(const T1<T2, T3>& a1, const T1<T2, T3>& a2, const P& p, bool)
    {
        T1<T2, T3> la1 = a1;
        T1<T2, T3> la2 = a2;

        if (la1.size() != la2.size())
            return p();

        for (; !la1.empty(); la1.pop(), la2.pop())
            if (!compare(la1.top(), la2.top(), p))
                return false;
        return true;
    }

    template <template <typename T1X, typename T2X, typename T3X> class T1, typename T2, typename T3, typename T4>
    IC static bool compare(const T1<T2, T3, T4>& a1, const T1<T2, T3, T4>& a2, const P& p, bool)
    {
        T1<T2, T3, T4> la1 = a1;
        T1<T2, T3, T4> la2 = a2;

        if (la1.size() != la2.size())
            return p();

        for (; !la1.empty(); la1.pop(), la2.pop())
            if (!compare(la1.top(), la2.top(), p))
                return false;
        return true;
    }

    template <typename T1, typename T2>
    IC static bool compare(const xr_stack<T1, T2>& s1, const xr_stack<T1, T2>& s2, const P& p)
    {
        return compare(s1, s2, p, true);
    }

    template <typename T1, typename T2, typename T3>
    IC static bool compare(
        const std::priority_queue<T1, T2, T3>& q1, const std::priority_queue<T1, T2, T3>& q2, const P& p)
    {
        return compare(q1, q2, p, true);
    }

    struct CHelper3
    {
        template <typename T>
        IC static bool compare(const T& a1, const T& a2, const P& p)
        {
            if (a1.size() != a2.size())
                return p();

            typename T::const_iterator I = a1.begin(), J = a2.begin();
            typename T::const_iterator E = a1.end();
            for (; I != E; ++I, ++J)
                if (!CComparer::compare(*I, *J, p))
                    return false;
            return true;
        }
    };

    template <typename T>
    struct CHelper4
    {
        template <bool a>
        IC static bool compare(std::enable_if_t<!a, const T&> a1, const T& a2, const P& p)
        {
            return CHelper<T>::template compare<object_type_traits::is_pointer<T>::value>(a1, a2, p);
        }

        template <bool a>
        IC static bool compare(std::enable_if_t<a, const T&> a1, const T& a2, const P& p)
        {
            return CHelper3::compare(a1, a2, p);
        }
    };

    template <typename T>
    IC static bool compare(const T& a1, const T& a2, const P& p)
    {
        return CHelper4<T>::template compare<object_type_traits::is_stl_container<T>::value>(a1, a2, p);
    }
};

template <typename P>
IC bool compare(LPCSTR p0, pstr p1, const P& p)
{
    return p(p0, p1);
}

template <typename P>
IC bool compare(pstr p0, LPCSTR p1, const P& p)
{
    return p(p0, p1);
}

template <typename T, typename P>
IC bool compare(const T& p0, const T& p1, const P& p)
{
    return CComparer<P>::compare(p0, p1, p);
}

namespace object_comparer
{
namespace detail
{
template <template <typename TX> class P>
struct comparer
{
    template <typename T>
    IC bool operator()(const T& a1, const T& a2) const
    {
        return P<T>()(a1, a2);
    }
    IC bool operator()() const { return P<bool>()(false, true); }
    IC bool operator()(LPCSTR s1, LPCSTR s2) const { return (P<int>()(xr_strcmp(s1, s2), 0)); }
    IC bool operator()(pstr s1, pstr s2) const { return (P<int>()(xr_strcmp(s1, s2), 0)); }
    IC bool operator()(LPCSTR s1, pstr s2) const { return (P<int>()(xr_strcmp(s1, s2), 0)); }
    IC bool operator()(pstr s1, LPCSTR s2) const { return (P<int>()(xr_strcmp(s1, s2), 0)); }
};
};
};

#define declare_comparer(a, b)\
    template <typename T1, typename T2>\
    IC bool a(const T1& p0, const T2& p1)\
    { return (compare(p0, p1, object_comparer::detail::comparer<b>())); }

declare_comparer(equal, std::equal_to);
declare_comparer(greater_equal, std::greater_equal);
declare_comparer(greater, std::greater);
declare_comparer(less_equal, std::less_equal);
declare_comparer(less, std::less);
declare_comparer(not_equal, std::not_equal_to);
declare_comparer(logical_and, std::logical_and);
declare_comparer(logical_or, std::logical_or);
