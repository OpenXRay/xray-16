
#ifndef BOOST_MPL_IF_HPP_INCLUDED
#define BOOST_MPL_IF_HPP_INCLUDED

// + file: boost/mpl/if.hpp
// + last modified: 10/mar/03

// Copyright (c) 2000-03 Boost.org
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee, 
// provided that the above copyright notice appears in all copies and 
// that both the copyright notice and this permission notice appear in 
// supporting documentation. No representations are made about the 
// suitability of this software for any purpose. It is provided "as is" 
// without express or implied warranty.
//
// See http://www.boost.org/libs/mpl for documentation.

#include "boost/mpl/aux_/value_wknd.hpp"
#include "boost/mpl/aux_/ice_cast.hpp"
#include "boost/mpl/aux_/void_spec.hpp"
#include "boost/mpl/aux_/lambda_support.hpp"
#include "boost/mpl/aux_/config/workaround.hpp"
#include "boost/config.hpp"

namespace boost {
namespace mpl {

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

template<
      bool C
    , typename T1
    , typename T2
    >
struct if_c
{
    typedef T1 type;
};

template<
      typename T1
    , typename T2
    >
struct if_c<false,T1,T2>
{
    typedef T2 type;
};

template<
      typename BOOST_MPL_AUX_VOID_SPEC_PARAM(C)
    , typename BOOST_MPL_AUX_VOID_SPEC_PARAM(T1)
    , typename BOOST_MPL_AUX_VOID_SPEC_PARAM(T2)
    >
struct if_
{
 private:
    // agurt, 02/jan/03: two-step 'type' definition for the sake of aCC 
    typedef if_c<
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x561))
          BOOST_MPL_AUX_VALUE_WKND(C)::value
#else
          BOOST_MPL_AUX_ICE_CAST(bool, BOOST_MPL_AUX_VALUE_WKND(C)::value)
#endif
        , T1
        , T2
        > almost_type_;
 
 public:
    typedef typename almost_type_::type type;
    
    BOOST_MPL_AUX_LAMBDA_SUPPORT(3,if_,(C,T1,T2))
};

#elif defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)

// MSVC6.5-specific version

template<
      bool C_
    , typename T1
    , typename T2
    >
struct if_c
{
 private:
    template<bool> struct answer        { typedef T1 type; };
    template<>     struct answer<false> { typedef T2 type; };
 
 public:
    typedef typename answer< C_ >::type type;
};

// (almost) copy & paste in order to save one more 
// recursively nested template instantiation to user
template<
      typename C_
    , typename T1
    , typename T2
    >
struct if_
{
 private:
    template<bool> struct answer        { typedef T1 type; };
    template<>     struct answer<false> { typedef T2 type; };

    // agurt, 17/sep/02: in some situations MSVC 7.0 doesn't 
    // handle 'answer<C::value>' expression very well
    enum { c_ = C_::value };

 public:
    typedef typename answer< BOOST_MPL_AUX_ICE_CAST(bool, c_) >::type type;

    BOOST_MPL_AUX_LAMBDA_SUPPORT(3,if_,(C_,T1,T2))
};

#else

// no partial class template specialization

namespace aux {

template< bool C >
struct if_impl
{
    template< typename T1, typename T2 > struct result_
    {
        typedef T1 type;
    };
};

template<>
struct if_impl<false>
{
    template< typename T1, typename T2 > struct result_
    { 
        typedef T2 type;
    };
};

} // namespace aux

template<
      bool C
    , typename T1
    , typename T2
    >
struct if_c
{
    typedef typename aux::if_impl< C >
        ::template result_<T1,T2>::type type;
};

// (almost) copy & paste in order to save one more 
// recursively nested template instantiation to user
template<
      typename BOOST_MPL_AUX_VOID_SPEC_PARAM(C)
    , typename BOOST_MPL_AUX_VOID_SPEC_PARAM(T1)
    , typename BOOST_MPL_AUX_VOID_SPEC_PARAM(T2)
    >
struct if_
{
    typedef typename aux::if_impl< BOOST_MPL_AUX_ICE_CAST(bool, C::value) >
        ::template result_<T1,T2>::type type;

    BOOST_MPL_AUX_LAMBDA_SUPPORT(3,if_,(C,T1,T2))
};

#endif // BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION

BOOST_MPL_AUX_VOID_SPEC(3, if_)

} // namespace mpl
} // namespace boost

#endif // BOOST_MPL_IF_HPP_INCLUDED
