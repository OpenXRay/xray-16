
// (C) Copyright John Maddock and Steve Cleary 2000.
// Permission to copy, use, modify, sell and distribute this software is 
// granted provided this copyright notice appears in all copies. This software 
// is provided "as is" without express or implied warranty, and with no claim 
// as to its suitability for any purpose.

// See http://www.boost.org for most recent version including documentation.
//
// macros and helpers for working with integral-constant-expressions.

#ifndef BOOST_TT_DETAIL_YES_NO_TYPE_HPP_INCLUDED
#define BOOST_TT_DETAIL_YES_NO_TYPE_HPP_INCLUDED

namespace boost {
namespace type_traits {

typedef char yes_type;
struct no_type
{
   char padding[8];
};

} // namespace type_traits
} // namespace boost

#endif // BOOST_TT_DETAIL_YES_NO_TYPE_HPP_INCLUDED
