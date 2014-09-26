//  (C) Copyright Gennadiy Rozental 2001-2002.
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied warranty,
//  and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version including documentation.
//
//  File        : $RCSfile: auto_unit_test.hpp,v $
//
//  Version     : $Id: auto_unit_test.hpp,v 1.2 2003/02/13 08:11:34 rogeeff Exp $
//
//  Description : support for automated test cases registration mechanism
//                for simple function based test cases
// ***************************************************************************

#ifndef BOOST_AUTO_UNIT_TEST_HPP
#define BOOST_AUTO_UNIT_TEST_HPP

// Boost.Test
#include <boost/test/unit_test.hpp>

static boost::unit_test_framework::test_suite* test = BOOST_TEST_SUITE( "Auto Unit Test" );

boost::unit_test_framework::test_suite*
init_unit_test_suite( int /* argc */, char* /* argv */ [] ) {
    return test;
}

// ************************************************************************** //
// **************           auto_unit_test_registrar           ************** //
// ************************************************************************** //

namespace boost {

namespace unit_test_framework {

struct auto_unit_test_registrar
{
    // Constructor
    explicit auto_unit_test_registrar( test_case* tc ) { test->add( tc ); }
};

} // unit_test_framework

} // namespace boost

// ************************************************************************** //
// **************             BOOST_AUTO_UNIT_TEST             ************** //
// ************************************************************************** //

#define BOOST_AUTO_UNIT_TEST( func_name )                           \
static void func_name();                                            \
static boost::unit_test_framework::auto_unit_test_registrar         \
    BOOST_JOIN( test_registrar, __LINE__)                           \
        ( BOOST_TEST_CASE( func_name ) );                           \
static void func_name()                                             \
/**/

// ***************************************************************************
//  Revision History :
//  
//  $Log: auto_unit_test.hpp,v $
//  Revision 1.2  2003/02/13 08:11:34  rogeeff
//  minor comment
//
//  Revision 1.1  2002/12/08 17:30:38  rogeeff
//  Automatic registration of unit tests facility introduced
//

// ***************************************************************************

#endif // BOOST_AUTO_UNIT_TEST_HPP
