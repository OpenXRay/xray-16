////////////////////////////////////////////////////////////////////////////
//  Module      : object_destroyer.h
//  Created     : 21.01.2003
//  Modified    : 09.07.2004
//  Author      : Dmitriy Iassenev
//  Description : Object destroyer
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrCore/xrMemory.h"

struct CDestroyer
{
    static void delete_data(pcstr /*data*/) {}
    static void delete_data(pstr data) { xr_free(data); }
    template <typename T1, typename T2>
    static void delete_data(std::pair<T1, T2>& data)
    {
        delete_data(data.first);
        delete_data(data.second);
    }

    template <typename T, int size>
    static void delete_data(svector<T, size>& data)
    {
        for (auto& it : data)
            delete_data(it);
        data.clear();
    }

    template <typename T, int n>
    static void delete_data(T (&array)[n])
    {
        for (auto& it : array)
            delete_data(it);
    }

    template <typename T1, typename T2>
    static void delete_data(std::queue<T1, T2>& data)
    {
        for (auto& it : data)
            delete_data(it);
    }

    template <template <typename TX1, typename TX2> class T1, typename T2, typename T3>
    static void delete_data(T1<T2, T3>& data, bool)
    {
        for (auto& it : data)
            delete_data(it);
    }

    template <template <typename TX1, typename TX2, typename TX3> class T1, typename T2, typename T3, typename T4>
    static void delete_data(T1<T2, T3, T4>& data, bool)
    {
        for (auto& it : data)
            delete_data(it);
    }

    template <typename T1, typename T2>
    static void delete_data(xr_stack<T1, T2>& data)
    {
        delete_data(data, true);
    }

    template <typename T1, typename T2, typename T3>
    static void delete_data(std::priority_queue<T1, T2, T3>& data)
    {
        delete_data(data, true);
    }

    template <typename T>
    struct CHelper1
    {
        template <bool a>
        static void delete_data(std::enable_if_t<!a, T&>)
        {
        }

        template <bool a>
        static void delete_data(std::enable_if_t<a, T&> data)
        {
            data.destroy();
        }
    };

    template <typename T>
    struct CHelper2
    {
        template <bool a>
        static void delete_data(std::enable_if_t<!a, T&> data)
        {
            CHelper1<T>::template delete_data<object_type_traits::is_base_and_derived<IPureDestroyableObject, T>::value>(data);
        }

        template <bool a>
        static void delete_data(std::enable_if_t<a, T&> data)
        {
            if (data)
                CDestroyer::delete_data(*data);
            xr_delete(data);
        }
    };

    struct CHelper3
    {
        template <typename T>
        static void delete_data(T& data)
        {
            for (auto& it : data)
                CDestroyer::delete_data(it);
            data.clear();
        }
    };

    template <typename T>
    struct CHelper4
    {
        template <bool a>
        static void delete_data(std::enable_if_t<!a, T&> data)
        {
            CHelper2<T>::template delete_data<object_type_traits::is_pointer<T>::value>(data);
        }

        template <bool a>
        static void delete_data(std::enable_if_t<a, T&> data)
        {
            CHelper3::delete_data(data);
        }
    };

    template <typename T>
    static void delete_data(T& data)
    {
        CHelper4<T>::template delete_data<object_type_traits::is_stl_container<T>::value>(data);
    }
};

template <typename T>
void delete_data(const T& data)
{
    T* temp = const_cast<T*>(&data);
    CDestroyer::delete_data(*temp);
}
