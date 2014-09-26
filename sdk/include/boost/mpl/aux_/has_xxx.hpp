//-----------------------------------------------------------------------------
// boost mpl/aux_/has_xxx.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2002
// Aleksey Gurtovoy
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee, 
// provided that the above copyright notice appears in all copies and 
// that both the copyright notice and this permission notice appear in 
// supporting documentation. No representations are made about the 
// suitability of this software for any purpose. It is provided "as is" 
// without express or implied warranty.

#ifndef BOOST_MPL_AUX_HAS_XXX_HPP_INCLUDED
#define BOOST_MPL_AUX_HAS_XXX_HPP_INCLUDED

#include "boost/mpl/aux_/type_wrapper.hpp"
#include "boost/mpl/aux_/yes_no.hpp"
#include "boost/mpl/aux_/config/msvc_typename.hpp"
#include "boost/mpl/aux_/config/overload_resolution.hpp"
#include "boost/mpl/aux_/config/static_constant.hpp"

#if !defined(BOOST_MPL_BROKEN_OVERLOAD_RESOLUTION) && (!defined(__GNUC__) || __GNUC__ == 3)

#   if (!defined(BOOST_MSVC) || BOOST_MSVC > 1300)

// the implementation below is based on a USENET newsgroup's posting by  
// Rani Sharoni (comp.lang.c++.moderated, 2002-03-17 07:45:09 PST)

#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(trait, name, unused) \
template< typename T > \
boost::mpl::aux::yes_tag \
trait##_helper( \
      boost::mpl::aux::type_wrapper<T> const volatile* \
    , boost::mpl::aux::type_wrapper<BOOST_MSVC_TYPENAME T::name>* = 0 \
    ); \
\
boost::mpl::aux::no_tag \
trait##_helper(...); \
\
template< typename T > \
struct trait \
{ \
    typedef boost::mpl::aux::type_wrapper<T> t_; \
    BOOST_STATIC_CONSTANT(bool, value = \
          sizeof((trait##_helper)(static_cast<t_*>(0))) \
            == sizeof(boost::mpl::aux::yes_tag) \
        ); \
}; \
/**/

#   else

#include "boost/mpl/if.hpp"
#include "boost/mpl/bool.hpp"
#include "boost/preprocessor/cat.hpp"

// agurt, 11/sep/02: MSVC version, based on a USENET newsgroup's posting by 
// John Madsen (comp.lang.c++.moderated, 1999-11-12 19:17:06 GMT);
// note that the code is _not_ standard-conforming, but it works, 
// and it resolves some nasty ICE cases with the above implementation

// Modified dwa 8/Oct/02 to handle reference types.

namespace boost { namespace mpl { namespace aux {

struct has_xxx_tag;

template< typename T >
struct msvc_is_incomplete
{
    struct incomplete_;
    BOOST_STATIC_CONSTANT(bool, value = 
          sizeof(void (T::*)()) == sizeof(void (incomplete_::*)())
        );
};

template<>
struct msvc_is_incomplete<int>
{
    BOOST_STATIC_CONSTANT(bool, value = false);
};

}}}

#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF_(trait, name, unused) \
template< typename T, typename name = ::boost::mpl::aux::has_xxx_tag > \
struct BOOST_PP_CAT(trait,_impl) : T \
{ \
 private: \
    static boost::mpl::aux::no_tag test(void(*)(::boost::mpl::aux::has_xxx_tag)); \
    static boost::mpl::aux::yes_tag test(...); \
\
 public: \
    BOOST_STATIC_CONSTANT(bool, value =  \
        sizeof(test(static_cast<void(*)(name)>(0))) \
            != sizeof(boost::mpl::aux::no_tag) \
        ); \
}; \
\
template< typename T > struct trait \
    : boost::mpl::if_c< \
          boost::mpl::aux::msvc_is_incomplete<T>::value \
        , boost::mpl::bool_<false> \
        , BOOST_PP_CAT(trait,_impl)<T> \
        >::type \
{ \
}; \
\
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, void) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, bool) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, char) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, signed char) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, unsigned char) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, signed short) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, unsigned short) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, signed int) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, unsigned int) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, signed long) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, unsigned long) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, float) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, double) \
BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, long double) \
/**/

#   define BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, T) \
template<> struct trait<T> \
{ \
    BOOST_STATIC_CONSTANT(bool,value = false); \
}; \
/**/

#if !defined(BOOST_NO_INTRINSIC_WCHAR_T)
#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(trait, name, unused) \
    BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF_(trait, name, unused) \
    BOOST_MPL_AUX_HAS_XXX_TRAIT_SPEC(trait, wchar_t) \
    /**/
#else
#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(trait, name, unused) \
    BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF_(trait, name, unused) \
    /**/
#endif

#   endif // BOOST_MSVC > 1300

#else 

// agurt, 11/jan/03: signals a stub-only implementation
#   define BOOST_NO_MPL_AUX_HAS_XXX

#   define BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(trait, name, default_value) \
template< typename T > \
struct trait \
{ \
     BOOST_STATIC_CONSTANT(bool, value = default_value); \
}; \
/**/

#endif // BOOST_MPL_BROKEN_OVERLOAD_RESOLUTION

#define BOOST_MPL_HAS_XXX_TRAIT_DEF(name) \
BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(has_##name, name, false) \
/**/

#endif // BOOST_MPL_AUX_HAS_XXX_HPP_INCLUDED
