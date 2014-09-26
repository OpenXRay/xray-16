//-----------------------------------------------------------------------------
// boost mpl/aux_/include_preprocessed.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2001-02
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

#include "boost/mpl/aux_/config/compiler.hpp"
#include "boost/preprocessor/cat.hpp"
#include "boost/preprocessor/stringize.hpp"

#   define AUX_PREPROCESSED_HEADER \
    aux_/preprocessed/BOOST_MPL_COMPILER_DIR/BOOST_MPL_PREPROCESSED_HEADER \
/**/

#   include BOOST_PP_STRINGIZE(boost/mpl/AUX_PREPROCESSED_HEADER)
#   undef AUX_PREPROCESSED_HEADER

#undef BOOST_MPL_PREPROCESSED_HEADER
