
// Copyright (c) 2001 Aleksey Gurtovoy.
// Permission to copy, use, modify, sell and distribute this software is 
// granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied warranty, 
// and with no claim as to its suitability for any purpose.

#ifndef BOOST_TT_BROKEN_COMPILER_SPEC_HPP_INCLUDED
#define BOOST_TT_BROKEN_COMPILER_SPEC_HPP_INCLUDED

#include "boost/mpl/aux_/lambda_support.hpp"
#include "boost/config.hpp"

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

#   define BOOST_TT_BROKEN_COMPILER_SPEC(T) /**/

#else

namespace boost { namespace detail {
template< typename T > struct remove_const_impl     { typedef T type; };
template< typename T > struct remove_volatile_impl  { typedef T type; };
template< typename T > struct remove_pointer_impl   { typedef T type; };
template< typename T > struct remove_reference_impl { typedef T type; };
}}

// same as BOOST_TT_AUX_TYPE_TRAIT_IMPL_SPEC1 macro, except that it
// never gets #undef-ined
#   define BOOST_TT_AUX_BROKEN_TYPE_TRAIT_SPEC1(trait,spec,result) \
template<> struct trait##_impl<spec> \
{ \
    typedef result type; \
}; \
/**/

#   define BOOST_TT_AUX_REMOVE_CONST_VOLATILE_RANK1_SPEC(T)                         \
    BOOST_TT_AUX_BROKEN_TYPE_TRAIT_SPEC1(remove_const,T const,T)                    \
    BOOST_TT_AUX_BROKEN_TYPE_TRAIT_SPEC1(remove_const,T const volatile,T volatile)  \
    BOOST_TT_AUX_BROKEN_TYPE_TRAIT_SPEC1(remove_volatile,T volatile,T)              \
    BOOST_TT_AUX_BROKEN_TYPE_TRAIT_SPEC1(remove_volatile,T const volatile,T const)  \
    /**/

#   define BOOST_TT_AUX_REMOVE_PTR_REF_RANK_1_SPEC(T)                               \
    BOOST_TT_AUX_BROKEN_TYPE_TRAIT_SPEC1(remove_pointer,T*,T)                       \
    BOOST_TT_AUX_BROKEN_TYPE_TRAIT_SPEC1(remove_reference,T&,T)                     \
    /**/

#   define BOOST_TT_AUX_REMOVE_PTR_REF_RANK_2_SPEC(T)                               \
    BOOST_TT_AUX_REMOVE_PTR_REF_RANK_1_SPEC(T)                                      \
    BOOST_TT_AUX_REMOVE_PTR_REF_RANK_1_SPEC(T const)                                \
    BOOST_TT_AUX_REMOVE_PTR_REF_RANK_1_SPEC(T volatile)                             \
    BOOST_TT_AUX_REMOVE_PTR_REF_RANK_1_SPEC(T const volatile)                       \
    /**/

#   define BOOST_TT_AUX_REMOVE_ALL_RANK_1_SPEC(T)                                   \
    BOOST_TT_AUX_REMOVE_PTR_REF_RANK_2_SPEC(T)                                      \
    BOOST_TT_AUX_REMOVE_CONST_VOLATILE_RANK1_SPEC(T)                                \
    /**/

#   define BOOST_TT_AUX_REMOVE_ALL_RANK_2_SPEC(T)                                   \
    BOOST_TT_AUX_REMOVE_ALL_RANK_1_SPEC(T*)                                         \
    BOOST_TT_AUX_REMOVE_ALL_RANK_1_SPEC(T const*)                                   \
    BOOST_TT_AUX_REMOVE_ALL_RANK_1_SPEC(T volatile*)                                \
    BOOST_TT_AUX_REMOVE_ALL_RANK_1_SPEC(T const volatile*)                          \
    /**/

#   define BOOST_TT_BROKEN_COMPILER_SPEC(T)                                         \
    namespace boost { namespace detail {                                            \
    BOOST_TT_AUX_REMOVE_ALL_RANK_1_SPEC(T)                                          \
    BOOST_TT_AUX_REMOVE_ALL_RANK_2_SPEC(T)                                          \
    BOOST_TT_AUX_REMOVE_ALL_RANK_2_SPEC(T*)                                         \
    BOOST_TT_AUX_REMOVE_ALL_RANK_2_SPEC(T const*)                                   \
    BOOST_TT_AUX_REMOVE_ALL_RANK_2_SPEC(T volatile*)                                \
    BOOST_TT_AUX_REMOVE_ALL_RANK_2_SPEC(T const volatile*)                          \
    }}                                                                              \
    /**/

#   include "boost/type_traits/detail/type_trait_undef.hpp"

#endif // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

BOOST_TT_BROKEN_COMPILER_SPEC(bool)
BOOST_TT_BROKEN_COMPILER_SPEC(char)
#ifndef BOOST_NO_INTRINSIC_WCHAR_T
BOOST_TT_BROKEN_COMPILER_SPEC(wchar_t)
#endif
BOOST_TT_BROKEN_COMPILER_SPEC(signed char)
BOOST_TT_BROKEN_COMPILER_SPEC(unsigned char)
BOOST_TT_BROKEN_COMPILER_SPEC(signed short)
BOOST_TT_BROKEN_COMPILER_SPEC(unsigned short)
BOOST_TT_BROKEN_COMPILER_SPEC(signed int)
BOOST_TT_BROKEN_COMPILER_SPEC(unsigned int)
BOOST_TT_BROKEN_COMPILER_SPEC(signed long)
BOOST_TT_BROKEN_COMPILER_SPEC(unsigned long)
BOOST_TT_BROKEN_COMPILER_SPEC(float)
BOOST_TT_BROKEN_COMPILER_SPEC(double)
//BOOST_TT_BROKEN_COMPILER_SPEC(long double)

// for backward compatibility
#define BOOST_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(T) \
    BOOST_TT_BROKEN_COMPILER_SPEC(T) \
/**/

#endif // BOOST_TT_BROKEN_COMPILER_SPEC_HPP_INCLUDED
