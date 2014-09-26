
// Copyright (C) 2000 John Maddock (john_maddock@compuserve.com)
// Copyright (C) 2002 Aleksey Gurtovoy (agurtovoy@meta-comm.com)
//
// Permission to copy and use this software is granted, 
// provided this copyright notice appears in all copies. 
// Permission to modify the code and to distribute modified code is granted, 
// provided this copyright notice appears in all copies, and a notice 
// that the code was modified is included with the copyright notice.
//
// This software is provided "as is" without express or implied warranty, 
// and with no claim as to its suitability for any purpose.

#ifndef BOOST_TT_IS_FUNCTION_HPP_INCLUDED
#define BOOST_TT_IS_FUNCTION_HPP_INCLUDED

#include "boost/type_traits/is_reference.hpp"
#include "boost/type_traits/detail/false_result.hpp"
#include "boost/config.hpp"

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
#   include "boost/type_traits/detail/is_function_ptr_helper.hpp"
#else
#   include "boost/type_traits/detail/is_function_ptr_tester.hpp"
#   include "boost/type_traits/detail/yes_no_type.hpp"
#endif

// should be the last #include
#include "boost/type_traits/detail/bool_trait_def.hpp"

// is a type a function?
// Please note that this implementation is unnecessarily complex:
// we could just use !is_convertible<T*, const volatile void*>::value,
// except that some compilers erroneously allow conversions from
// function pointers to void*.

namespace boost {
namespace detail {

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
template<bool is_ref = true>
struct is_function_chooser
    : ::boost::type_traits::false_result
{
};

template <>
struct is_function_chooser<false>
{
    template< typename T > struct result_
        : ::boost::type_traits::is_function_ptr_helper<T*>
    {
    };
};

template <typename T>
struct is_function_impl
    : is_function_chooser< ::boost::is_reference<T>::value >
        ::template result_<T>
{
};

#else

template <typename T>
struct is_function_impl
{
    static T* t;
    BOOST_STATIC_CONSTANT(
        bool, value = sizeof(::boost::type_traits::is_function_ptr_tester(t))
        == sizeof(::boost::type_traits::yes_type)
        );
};

#endif

} // namespace detail

BOOST_TT_AUX_BOOL_TRAIT_DEF1(is_function,T,::boost::detail::is_function_impl<T>::value)

} // namespace boost

#include "boost/type_traits/detail/bool_trait_undef.hpp"

#endif // BOOST_TT_IS_FUNCTION_HPP_INCLUDED
