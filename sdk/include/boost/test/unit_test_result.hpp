//  (C) Copyright Gennadiy Rozental 2001-2002.
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied warranty,
//  and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version including documentation.
//
//  File        : $RCSfile: unit_test_result.hpp,v $
//
//  Version     : $Id: unit_test_result.hpp,v 1.13 2003/02/13 08:26:33 rogeeff Exp $
//
//  Description : defines class unit_test_result that is responsible for 
//  gathering test results and presenting this information to end-user
// ***************************************************************************

#ifndef BOOST_UNIT_TEST_RESULT_HPP
#define BOOST_UNIT_TEST_RESULT_HPP

// Boost.Test
#include <boost/test/detail/unit_test_config.hpp>

// BOOST
#include <boost/shared_ptr.hpp>

// STL
#include <iosfwd>                       // std::ostream
#include <string>                       // std::string
#include <cstddef>                      // std::size_t
namespace boost {

namespace unit_test_framework {

// ************************************************************************** //
// **************               unit_test_result               ************** //
// ************************************************************************** //

class unit_test_result {
    friend struct unit_test_result_saver;
public:
    // Destructor
    ~unit_test_result();

    // current test results access and management
    static unit_test_result& instance();
    static void     test_case_start( std::string const& name_, unit_test_counter expected_failures_ = 0 );
    static void     test_case_end();
    
    static void     set_report_format( std::string const& reportformat );

    // use to dynamically change amount of errors expected in current test case
    void            increase_expected_failures( unit_test_counter amount = 1 );

    // reporting
    void            report( std::string const& reportlevel, std::ostream& where_to_ );              // report by level
    void            confirmation_report( std::ostream& where_to_ );                                 // shortest
    void            short_report( std::ostream& where_to_ ) { report( "short", where_to_ ); }       // short
    void            detailed_report( std::ostream& where_to_ ) { report( "detailed", where_to_ ); } // long

    int             result_code();                                                                  // to be returned from main

    // to be used by tool box implementation
    void            inc_failed_assertions();
    void            inc_passed_assertions(); 

    // to be used by monitor to notify that test case thrown exception
    void            caught_exception();

    // access method; to be used by unit_test_log
    std::string const& test_case_name();

    // used mostly by the Boost.Test unit testing
    void            failures_details( unit_test_counter& num_of_failures_, bool& exception_caught_ );

private:
    // report impl method
    void            report_result( std::ostream& where_to_, std::size_t indent_, bool detailed_ );

    // used to temporarely introduce new results set without polluting current one
    static void     reset_current_result_set();

    // Constructor
    unit_test_result( unit_test_result* parent_, std::string const& test_case_name_, unit_test_counter expected_failures_ = 0 );
   
    // Data members
    struct Impl;
    boost::shared_ptr<Impl> m_pimpl;
};

// ************************************************************************** //
// **************            unit_test_result_saver            ************** //
// ************************************************************************** //

struct unit_test_result_saver
{
    unit_test_result_saver()  { unit_test_result::reset_current_result_set(); }
    ~unit_test_result_saver() { unit_test_result::reset_current_result_set(); }
};

} // namespace unit_test_framework

} // namespace boost

// ***************************************************************************
//  Revision History :
//  
//  $Log: unit_test_result.hpp,v $
//  Revision 1.13  2003/02/13 08:26:33  rogeeff
//  report format config method added
//  result set interface changed slightly to allow single entry point with string as report selector
//
//  Revision 1.12  2002/12/11 13:41:19  beman_dawes
//  fix missing std::
//
//  Revision 1.11  2002/12/08 17:47:31  rogeeff
//  switched to use c_string_literal
//  unit_test_result_saver introduced to properly managed reset_current_test_set calls
//  in case of exceptions
//
//  Revision 1.10  2002/11/02 19:31:04  rogeeff
//  merged into the main trank
//

// ***************************************************************************

#endif // BOOST_UNIT_TEST_RESULT_HPP

