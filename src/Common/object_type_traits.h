////////////////////////////////////////////////////////////////////////////
//  Module      : object_type_traits.h
//  Created     : 21.01.2003
//  Modified    : 12.07.2004
//  Author      : Dmitriy Iassenev
//  Description : Object type traits
////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef object_type_traits_h_included
#define object_type_traits_h_included

#include <type_traits>

#define declare_has(a)                                            \
    template <typename T>                                         \
    struct has_##a                                                \
    {                                                             \
        template <typename P>                                     \
        static detail::yes select(detail::other<typename P::a>*); \
        template <typename P>                                     \
        static detail::no select(...);                            \
        enum                                                      \
        {                                                         \
            value = sizeof(detail::yes) == sizeof(select<T>(0))   \
        };                                                        \
    };

template <bool expression, typename T1, typename T2>
struct _if
{
    using result = typename std::conditional<expression, T1, T2>::type;
};

template <typename T1, typename T2>
struct is_type
{
    enum
    {
        value = std::is_same<T1, T2>::value,
    };
};

template <typename T>
struct type
{
    typedef T result;
};

namespace object_type_traits
{
namespace detail
{
struct yes
{
    char a[1];
};
struct no
{
    char a[2];
};
template <typename T>
struct other
{
};
};

template <typename T>
struct remove_pointer
{
    typedef T type;
};

template <typename T>
struct remove_pointer<T*>
{
    typedef T type;
};

template <typename T>
struct remove_pointer<T* const>
{
    typedef T type;
};

template <typename T>
struct remove_reference
{
    typedef T type;
};

template <typename T>
struct remove_reference<T&>
{
    typedef T type;
};

template <typename T>
struct remove_reference<T const&>
{
    typedef T type;
};

template <typename T>
struct remove_const
{
    typedef T type;
};

template <typename T>
struct remove_const<T const>
{
    typedef T type;
};

template <typename T>
struct remove_noexcept;

template <typename R, typename... Args>
struct remove_noexcept<R(Args...) noexcept>
{
    using type = R(Args...);
};

template< typename R, typename... Args>
struct remove_noexcept <R(*)(Args...) noexcept>
{
    using type = R(*)(Args...);
};

template <typename C, typename R, typename... Args>
struct remove_noexcept<R(C::*)(Args...) noexcept>
{
    using type = R(C::*)(Args...);
};

template <typename C, typename R, typename... Args>
struct remove_noexcept<R(C::*)(Args...) const noexcept>
{
    using type = R(C::*)(Args...) const;
};

#define REMOVE_NOEXCEPT(fn) (object_type_traits::remove_noexcept<decltype(fn)>::type)(fn)

template <typename T>
struct is_void
{
    enum
    {
        value = std::is_same<void, T>::value
    };
};

template <typename T>
struct is_const
{
    enum
    {
        value = false
    };
};

template <typename T>
struct is_const<T const>
{
    enum
    {
        value = true
    };
};

template <typename T>
struct is_pointer
{
    template <typename P>
    static detail::yes select(detail::other<P*>);
    static detail::no select(...);

    enum
    {
        value = sizeof(detail::yes) == sizeof(select(detail::other<T>()))
    };
};

template <typename T>
struct is_reference
{
    template <typename P>
    static detail::yes select(detail::other<P&>);
    static detail::no select(...);

    enum
    {
        value = sizeof(detail::yes) == sizeof(select(detail::other<T>()))
    };
};

template <typename _T1, typename _T2>
struct is_same
{
    typedef typename remove_const<_T1>::type T1;
    typedef typename remove_const<_T2>::type T2;

    enum
    {
        value = is_type<T1, T2>::value
    };
};

template <typename _T1, typename _T2>
struct is_base_and_derived
{
    typedef typename remove_const<_T1>::type T1;
    typedef typename remove_const<_T2>::type T2;

    static detail::yes select(T1*);
    static detail::no select(...);

    enum
    {
        value = std::is_class<T1>::value && std::is_class<T2>::value && !is_same<T1, T2>::value &&
            sizeof(detail::yes) == sizeof(select((T2*)(0)))
    };
};

template <template <typename _1> class T1, typename T2, typename T3>
struct is_base_and_derived_or_same_for_template_template_1_1
{
    template <typename P>
    static typename _if<is_base_and_derived<P, T3>::value || is_same<P, T3>::value, detail::yes, detail::no>::result
    select(T1<P>*);
    static detail::no select(...);

    enum
    {
        value = sizeof(detail::yes) == sizeof(select((T2*)0))
    };
};

template <template <typename _1> class T1, typename T2>
struct is_base_and_derived_or_same_from_template
{
    template <typename P>
    static detail::yes select(T1<P>*);
    static detail::no select(...);

    enum
    {
        value = sizeof(detail::yes) == sizeof(select((T2*)0))
    };
};

declare_has(iterator);
declare_has(const_iterator);
//declare_has(reference);
//declare_has(const_reference);
declare_has(value_type);
declare_has(size_type);
//declare_has(value_compare);

template <typename T>
struct is_stl_container
{
    enum
    {
        value = has_iterator<T>::value && has_const_iterator<T>::value &&
            // has_reference<T>::value &&
            // has_const_reference<T>::value &&
            has_size_type<T>::value &&
            has_value_type<T>::value
    };
};

//  template <typename _T>
//  struct is_tree_structure {
//      enum {
//          value = has_value_compare<_T>::value
//      };
//  };
};
#endif //   object_type_traits_h_included
