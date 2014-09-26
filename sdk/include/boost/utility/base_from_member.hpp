//  boost utility/base_from_member.hpp header file  --------------------------//

//  (C) Copyright Daryle Walker 2001.  Permission to copy, use, modify, sell
//  and distribute this software is granted provided this copyright
//  notice appears in all copies.  This software is provided "as is" without
//  express or implied warranty, and with no claim as to its suitability for
//  any purpose.

//  See http://www.boost.org for most recent version including documentation.

#ifndef BOOST_UTILITY_BASE_FROM_MEMBER_HPP
#define BOOST_UTILITY_BASE_FROM_MEMBER_HPP

#include <boost/utility_fwd.hpp>  // required for parameter defaults


namespace boost
{

//  Base-from-member class template  -----------------------------------------//

// Helper to initialize a base object so a derived class can use this
// object in the initialization of another base class.  Used by
// Dietmar Kuehl from ideas by Ron Klatcho to solve the problem of a
// base class needing to be initialized by a member.

// Contributed by Daryle Walker

template < typename MemberType, int UniqueID >
class base_from_member
{
protected:
    MemberType  member;

    explicit  base_from_member()
        : member()
        {}

    template< typename T1 >
    explicit base_from_member( T1 x1 )
        : member( x1 )
        {}

    template< typename T1, typename T2 >
    base_from_member( T1 x1, T2 x2 )
        : member( x1, x2 )
        {}

    template< typename T1, typename T2, typename T3 >
    base_from_member( T1 x1, T2 x2, T3 x3 )
        : member( x1, x2, x3 ) 
        {}

};  // boost::base_from_member

}  // namespace boost


#endif  // BOOST_UTILITY_BASE_FROM_MEMBER_HPP
