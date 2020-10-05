////////////////////////////////////////////////////////////////////////////
//	Module 		: object_saver.h
//	Created 	: 21.01.2003
//  Modified 	: 09.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Object saver
////////////////////////////////////////////////////////////////////////////

#pragma once
#include <type_traits>
#include "xrCore/xrstring.h"
#include "xrCommon/xr_string.h"

template <class M, typename P>
struct CSaver
{
    template <typename T>
    struct CHelper1
    {
        template <bool a>
        IC static void save_data(std::enable_if_t<!a, const T&> data, M& stream, const P& /*p*/)
        {
            static_assert(!std::is_polymorphic<T>::value, "Cannot save polymorphic classes as binary data.");
            stream.w(&data, sizeof(T));
        }

        template <bool a>
        IC static void save_data(std::enable_if_t<a, const T&> data, M& stream, const P& /*p*/)
        {
            T* data1 = const_cast<T*>(&data);
            data1->save(stream);
        }
    };

    template <typename T>
    struct CHelper
    {
        template <bool pointer>
        IC static void save_data(std::enable_if_t<!pointer, const T&> data, M& stream, const P& p)
        {
            CHelper1<T>::template save_data<object_type_traits::is_base_and_derived<ISerializable, T>::value>(data, stream, p);
        }

        template <bool pointer>
        IC static void save_data(std::enable_if_t<pointer, const T&> data, M& stream, const P& p)
        {
            CSaver<M, P>::save_data(*data, stream, p);
        }
    };

    struct CHelper3
    {
        template <typename T>
        IC static void save_data(const T& data, M& stream, const P& p)
        {
            stream.w_u32((u32)data.size());
            typename T::const_iterator I = data.begin();
            typename T::const_iterator E = data.end();
            for (; I != E; ++I)
                if (p(data, *I))
                    CSaver<M, P>::save_data(*I, stream, p);
        }
    };

    template <typename T>
    struct CHelper4
    {
        template <bool a>
        IC static void save_data(std::enable_if_t<!a, const T&> data, M& stream, const P& p)
        {
            CHelper<T>::template save_data<object_type_traits::is_pointer<T>::value>(data, stream, p);
        }

        template <bool a>
        IC static void save_data(std::enable_if_t<a, const T&> data, M& stream, const P& p)
        {
            CHelper3::save_data(data, stream, p);
        }
    };

    IC static void save_data(pstr data, M& stream, const P& /*p*/) { stream.w_stringZ(data); }
    IC static void save_data(LPCSTR data, M& stream, const P& /*p*/) { stream.w_stringZ(data); }
    IC static void save_data(const shared_str& data, M& stream, const P& /*p*/) { stream.w_stringZ(data); }
    IC static void save_data(const xr_string& data, M& stream, const P& /*p*/) { stream.w_stringZ(data.c_str()); }
    template <typename T1, typename T2>
    IC static void save_data(const std::pair<T1, T2>& data, M& stream, const P& p)
    {
        if (p(data, data.first, true))
            CSaver<M, P>::save_data(data.first, stream, p);
        if (p(data, data.second, false))
            CSaver<M, P>::save_data(data.second, stream, p);
    }

    IC static void save_data(const xr_vector<bool>& data, M& stream, const P& /*p*/)
    {
        stream.w_u32((u32)data.size());
        auto I = data.cbegin();
        auto E = data.cend();
        u32 mask = 0;
        if (I != E)
        {
            for (int j = 0; I != E; ++I, ++j)
            {
                if (j >= 32)
                {
                    stream.w_u32(mask);
                    mask = 0;
                    j = 0;
                }
                if (*I)
                    mask |= u32(1) << j;
            }
            stream.w_u32(mask);
        }
    };

    template <typename T, int size>
    IC static void save_data(const svector<T, size>& data, M& stream, const P& p)
    {
        stream.w_u32((u32)data.size());
        auto I = data.cbegin();
        auto E = data.cend();
        for (; I != E; ++I)
            if (p(data, *I))
                CSaver<M, P>::save_data(*I, stream, p);
    }

    template <typename T1, typename T2>
    IC static void save_data(const std::queue<T1, T2>& data, M& stream, const P& p)
    {
        std::queue<T1, T2> temp = data;
        stream.w_u32((u32)data.size());
        for (; !temp.empty(); temp.pop())
            if (p(temp, temp.front()))
                CSaver<M, P>::save_data(temp.front(), stream, p);
    }

    template <template <typename _1, typename _2> class T1, typename T2, typename T3>
    IC static void save_data(const T1<T2, T3>& data, M& stream, const P& p, bool)
    {
        T1<T2, T3> temp = data;
        stream.w_u32((u32)data.size());
        for (; !temp.empty(); temp.pop())
            if (p(temp, temp.top()))
                CSaver<M, P>::save_data(temp.top(), stream, p);
    }

    template <template <typename _1, typename _2, typename _3> class T1, typename T2, typename T3, typename T4>
    IC static void save_data(const T1<T2, T3, T4>& data, M& stream, const P& p, bool)
    {
        T1<T2, T3, T4> temp = data;
        stream.w_u32((u32)data.size());
        for (; !temp.empty(); temp.pop())
            if (p(temp, temp.top()))
                CSaver<M, P>::save_data(temp.top(), stream, p);
    }

    template <typename T1, typename T2>
    IC static void save_data(const xr_stack<T1, T2>& data, M& stream, const P& p)
    {
        save_data(data, stream, p, true);
    }

    template <typename T1, typename T2, typename T3>
    IC static void save_data(const std::priority_queue<T1, T2, T3>& data, M& stream, const P& p)
    {
        save_data(data, stream, p, true);
    }

    template <typename T>
    IC static void save_data(const T& data, M& stream, const P& p)
    {
        CHelper4<T>::template save_data<object_type_traits::is_stl_container<T>::value>(data, stream, p);
    }
};

namespace object_saver
{
namespace detail
{
struct CEmptyPredicate
{
    template <typename T1, typename T2>
    IC bool operator()(const T1& /*data*/, const T2& /*value*/) const
    { return true; }
    template <typename T1, typename T2>
    IC bool operator()(const T1& /*data*/, const T2& /*value*/, bool) const
    { return true; }
};
};
};

template <typename T, typename M, typename P>
IC void save_data(const T& data, M& stream, const P& p)
{
    CSaver<M, P>::save_data(data, stream, p);
}

template <typename T, typename M>
IC void save_data(const T& data, M& stream)
{
    save_data(data, stream, object_saver::detail::CEmptyPredicate());
}
