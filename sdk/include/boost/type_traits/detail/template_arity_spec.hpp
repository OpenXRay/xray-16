//-----------------------------------------------------------------------------
// boost/type_traits/detail/template_arity_spec.hpp header file
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

// no include guards, the header is intended for multiple inclusion!

#include "boost/mpl/aux_/template_arity_fwd.hpp"
#include "boost/mpl/aux_/preprocessor/params.hpp"
#include "boost/mpl/aux_/lambda_support.hpp"
#include "boost/mpl/aux_/config/overload_resolution.hpp"
#include "boost/config.hpp"

#if defined(BOOST_MPL_NO_FULL_LAMBDA_SUPPORT) && \
    defined(BOOST_MPL_BROKEN_OVERLOAD_RESOLUTION)
#   define BOOST_TT_AUX_TEMPLATE_ARITY_SPEC(i, name) \
namespace mpl { namespace aux { \
template< BOOST_MPL_PP_PARAMS(i, typename T) > \
struct template_arity< \
      name< BOOST_MPL_PP_PARAMS(i, T) > \
    > \
{ \
    BOOST_STATIC_CONSTANT(int, value = i ); \
}; \
}} \
/**/
#else
#   define BOOST_TT_AUX_TEMPLATE_ARITY_SPEC(i, name) /**/
#endif
