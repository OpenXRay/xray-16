////////////////////////////////////////////////////////////////////////////
//  Module      : object_loader.h
//  Created     : 21.01.2003
//  Modified    : 09.07.2004
//  Author      : Dmitriy Iassenev
//  Description : Object loader
////////////////////////////////////////////////////////////////////////////

#pragma once
#include <type_traits>
#include "xrCore/xrstring.h"
#include "xrCommon/xr_string.h"

template <class M, typename P>
struct CLoader
{
    template <typename T>
    struct CHelper1
    {
        template <bool a>
        static void load_data(std::enable_if_t<!a, T&> data, M& stream, const P& /*p*/)
        {
            static_assert(!std::is_polymorphic<T>::value, "Cannot load polymorphic classes as binary data.");
            stream.r(&data, sizeof(T));
        }

        template <bool a>
        static void load_data(std::enable_if_t<a, T&> data, M& stream, const P& /*p*/)
        {
            T* data1 = const_cast<T*>(&data);
            data1->load(stream);
        }
    };

    template <typename T>
    struct CHelper
    {
        template <bool pointer>
        static void load_data(std::enable_if_t<!pointer, T&> data, M& stream, const P& p)
        {
            CHelper1<T>::template load_data<object_type_traits::is_base_and_derived<ISerializable, T>::value>(data, stream, p);
        }

        template <bool pointer>
        static void load_data(std::enable_if_t<pointer, T&> data, M& stream, const P& p)
        {
            CLoader<M, P>::load_data(*(data = new typename object_type_traits::remove_pointer<T>::type()), stream, p);
        }
    };

    struct CHelper3
    {
        template <typename T>
        struct has_value_compare
        {
            template <typename _P>
            static object_type_traits::detail::yes select(
                object_type_traits::detail::other<typename _P::value_compare>*);
            template <typename _P>
            static object_type_traits::detail::no select(...);
            enum
            {
                value = sizeof(object_type_traits::detail::yes) == sizeof(select<T>(0))
            };
        };

        template <typename T>
        struct is_tree_structure
        {
            enum
            {
                value = has_value_compare<T>::value
            };
        };

        template <typename T1, typename T2>
        struct add_helper
        {
            template <bool a>
            static void add(std::enable_if_t<!a, T1&> data, T2& value)
            {
                data.push_back(value);
            }

            template <bool a>
            static void add(std::enable_if_t<a, T1&> data, T2& value)
            {
                data.insert(value);
            }
        };

        template <typename T1, typename T2>
        static void add(T1& data, T2& value)
        {
            add_helper<T1, T2>::template add<is_tree_structure<T1>::value>(data, value);
        }

        template <typename T>
        static void load_data(T& data, M& stream, const P& p)
        {
            if (p.can_clear())
                data.clear();
            const u32 count = stream.r_u32();
            for (u32 i = 0; i < count; ++i)
            {
                typename T::value_type temp;
                CLoader<M, P>::load_data(temp, stream, p);
                if (p(data, temp))
                    add(data, temp);
            }
        }
    };

    template <typename T>
    struct CHelper4
    {
        template <bool a>
        static void load_data(std::enable_if_t<!a, T&> data, M& stream, const P& p)
        {
            CHelper<T>::template load_data<object_type_traits::is_pointer<T>::value>(data, stream, p);
        }

        template <bool a>
        static void load_data(std::enable_if_t<a, T&> data, M& stream, const P& p)
        {
            CHelper3::load_data(data, stream, p);
        }
    };

    static void load_data(LPCSTR& data, M& stream, const P& p) { NODEFAULT; }
    static void load_data(LPSTR& data, M& stream, const P& p)
    {
        shared_str S;
        stream.r_stringZ(S);
        data = xr_strdup(*S);
    }

    static void load_data(shared_str& data, M& stream, const P& p) { stream.r_stringZ(data); }
    static void load_data(xr_string& data, M& stream, const P& p)
    {
        shared_str S;
        stream.r_stringZ(S);
        data = *S;
    }

    template <typename T1, typename T2>
    static void load_data(std::pair<T1, T2>& data, M& stream, const P& p)
    {
        if (p(data, const_cast<typename object_type_traits::remove_const<T1>::type&>(data.first), true))
        {
            const bool value = object_type_traits::is_same<T1, LPCSTR>::value;
            VERIFY(!value);
            load_data(const_cast<typename object_type_traits::remove_const<T1>::type&>(data.first), stream, p);
        }
        if (p(data, data.second, false))
            load_data(data.second, stream, p);
        p.after_load(data, stream);
    }

    static void load_data(xr_vector<bool>& data, M& stream, const P& p)
    {
        if (p.can_clear())
            data.clear();
        const u32 prev_count = data.size();
        data.resize(prev_count + stream.r_u32());
        xr_vector<bool>::iterator I = data.begin() + prev_count;
        xr_vector<bool>::iterator E = data.end();
        u32 mask = 0;
        for (int j = 32; I != E; ++I, ++j)
        {
            if (j >= 32)
            {
                mask = stream.r_u32();
                j = 0;
            }
            *I = !!(mask & (u32(1) << j));
        }
    };

    template <typename T, int size>
    static void load_data(svector<T, size>& data, M& stream, const P& p)
    {
        if (p.can_clear())
            data.clear();
        const u32 count = stream.r_u32();
        for (u32 i = 0; i < count; ++i)
        {
            typename svector<T, size>::value_type temp;
            CLoader<M, P>::load_data(temp, stream, p);
            if (p(data, temp))
                data.push_back(temp);
        }
    }

    template <typename T1, typename T2>
    static void load_data(std::queue<T1, T2>& data, M& stream, const P& p)
    {
        if (p.can_clear())
        {
            while (!data.empty())
                data.pop();
        }
        std::queue<T1, T2> temp;
        const u32 count = stream.r_u32();
        for (u32 i = 0; i < count; ++i)
        {
            typename std::queue<T1, T2>::value_type t;
            CLoader<M, P>::load_data(t, stream, p);
            if (p(temp, t))
                temp.push(t);
        }
        for (; !temp.empty(); temp.pop())
            data.push(temp.front());
    }

    template <template <typename _1, typename _2> class T1, typename T2, typename T3>
    static void load_data(T1<T2, T3>& data, M& stream, const P& p, bool)
    {
        if (p.can_clear())
        {
            while (!data.empty())
                data.pop();
        }
        T1<T2, T3> temp;
        const u32 count = stream.r_u32();
        for (u32 i = 0; i < count; ++i)
        {
            typename  T1<T2, T3>::value_type t;
            CLoader<M, P>::load_data(t, stream, p);
            if (p(temp, t))
                temp.push(t);
        }
        for (; !temp.empty(); temp.pop())
            data.push(temp.top());
    }

    template <template <typename _1, typename _2, typename _3> class T1, typename T2, typename T3, typename T4>
    static void load_data(T1<T2, T3, T4>& data, M& stream, const P& p, bool)
    {
        if (p.can_clear())
        {
            while (!data.empty())
                data.pop();
        }
        T1<T2, T3, T4> temp;
        u32 count = stream.r_u32();
        for (u32 i = 0; i < count; ++i)
        {
            typename T1<T2, T3, T4>::value_type t;
            CLoader<M, P>::load_data(t, stream, p);
            if (p(temp, t))
                temp.push(t);
        }
        for (; !temp.empty(); temp.pop())
            data.push(temp.top());
    }

    template <typename T1, typename T2>
    static void load_data(xr_stack<T1, T2>& data, M& stream, const P& p)
    {
        load_data(data, stream, p, true);
    }

    template <typename T1, typename T2, typename T3>
    static void load_data(std::priority_queue<T1, T2, T3>& data, M& stream, const P& p)
    {
        load_data(data, stream, p, true);
    }

    template <typename T>
    static void load_data(T& data, M& stream, const P& p)
    {
        CHelper4<T>::template load_data<object_type_traits::is_stl_container<T>::value>(data, stream, p);
    }
};

namespace object_loader
{
namespace detail
{
struct CEmptyPredicate
{
    template <typename T1, typename T2>
    void after_load(T1& /*data*/, T2& /*stream*/) const {}
    template <typename T1, typename T2>
    bool operator()(T1& /*data*/, const T2& /*value*/) const { return true; }
    template <typename T1, typename T2>
    bool operator()(T1& /*data*/, const T2& /*value*/, bool) const { return true; }
    bool can_clear() const { return true; }
    bool can_add() const { return true; }
};
};
};

template <typename T, typename M, typename P>
void load_data(const T& data, M& stream, const P& p)
{
    T* temp = const_cast<T*>(&data);
    CLoader<M, P>::load_data(*temp, stream, p);
}

template <typename T, typename M>
void load_data(const T& data, M& stream)
{
    load_data(data, stream, object_loader::detail::CEmptyPredicate());
}
