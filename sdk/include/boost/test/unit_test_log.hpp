//  (C) Copyright Gennadiy Rozental 2001-2002.
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied warranty,
//  and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version including documentation.
//
//  File        : $RCSfile: unit_test_log.hpp,v $
//
//  Version     : $Id: unit_test_log.hpp,v 1.15 2003/02/15 22:26:00 rogeeff Exp $
//
//  Description : defines singleton class unit_test_log and all manipulators.
//  unit_test_log has output stream like interface. It's implementation is
//  completely hidden with pimple idiom
// ***************************************************************************

#ifndef BOOST_UNIT_TEST_LOG_HPP
#define BOOST_UNIT_TEST_LOG_HPP

// Boost.Test
#include <boost/test/detail/unit_test_config.hpp>

// BOOST
#include <boost/utility.hpp>

// STL
#include <iosfwd>   // for std::ostream&
#include <string>   // for std::string&; in fact need only forward declaration

namespace boost {

namespace unit_test_framework {

//  each log level includes all subsequent higher loging levels
enum            log_level {
    log_successful_tests     = 0,
    log_test_suites          = 1,
    log_messages             = 2,
    log_warnings             = 3,
    log_all_errors           = 4, // reported by unit test macros
    log_cpp_exception_errors = 5, // uncaught C++ exceptions
    log_system_errors        = 6, // including timeouts, signals, traps
    log_fatal_errors         = 7, // including unit test macros or
                                     // fatal system errors
    log_progress_only        = 8, // only unit test progress to be reported
    log_nothing              = 9
};

// ************************************************************************** //
// **************                log manipulators              ************** //
// ************************************************************************** //

struct begin {
};

struct end {
};

struct level {
    explicit    level( log_level l_ ) : m_level( l_ ) {}

    log_level m_level;
};

struct line {
    explicit    line( std::size_t ln_ ) : m_line_num( ln_ ) {}

    std::size_t m_line_num;
};

struct file {
    explicit    file( c_string_literal fn_ ) : m_file_name( fn_ ) {}

    c_string_literal m_file_name;
};

struct checkpoint {
    explicit    checkpoint( std::string const& message_ ) : m_message( message_ ) {}

    std::string const& m_message;
};

struct log_exception {
    explicit    log_exception( c_string_literal what_ ) : m_what( what_ ) {}

    c_string_literal     m_what;
};

struct log_progress {
};

// ************************************************************************** //
// **************                 unit_test_log                ************** //
// ************************************************************************** //

class test_case;

class unit_test_log : private boost::noncopyable { //!! Singleton
public:
    // Destructor
    ~unit_test_log();

    // instance access method;
    static unit_test_log& instance();


    void            start( bool print_build_info_ = false );
    void            header( unit_test_counter test_cases_amount_ );
    void            finish( unit_test_counter test_cases_amount_ );

    // log configuration methods
    void            set_log_stream( std::ostream& str_ );
    void            set_log_threshold_level( log_level lev_ );
    void            set_log_threshold_level_by_name( std::string const& lev_ );
    void            set_log_format( std::string const& of );
    void            clear_checkpoint();

    // test case scope tracking
    void            track_test_case_scope( test_case const& tc, bool in_out );

    // entry configuration methods
    unit_test_log&  operator<<( begin const& );         // begin entry 
    unit_test_log&  operator<<( end const& );           // end entry
    unit_test_log&  operator<<( file const& );          // set file name
    unit_test_log&  operator<<( line const& );          // set line number
    unit_test_log&  operator<<( level const& );         // set entry level
    unit_test_log&  operator<<( checkpoint const& );    // set checkpoint

    // print value_ methods
    unit_test_log&  operator<<( log_progress const& );
    unit_test_log&  operator<<( log_exception const& );
    unit_test_log&  operator<<( c_string_literal value_ );
    unit_test_log&  operator<<( std::string const& value_ );

private:
    // Constructor
    unit_test_log();
    friend class unit_test_log_formatter;

    struct          Impl;
    Impl*           m_pimpl;
}; // unit_test_log

// helper macros
#define BOOST_UT_LOG_BEGIN( file_name, line_num, loglevel )                             \
    boost::unit_test_framework::unit_test_log::instance()                               \
                                     << boost::unit_test_framework::begin()             \
                                     << boost::unit_test_framework::level( loglevel )   \
                                     << boost::unit_test_framework::file( file_name )   \
                                     << boost::unit_test_framework::line( line_num ) << \
/**/
#define BOOST_UT_LOG_END             << boost::unit_test_framework::end();

} // namespace unit_test_framework

} // namespace boost

// ***************************************************************************
//  Revision History :
//  
//  $Log: unit_test_log.hpp,v $
//  Revision 1.15  2003/02/15 22:26:00  rogeeff
//  excessive include parameters moved to source file
//
//  Revision 1.14  2003/02/13 08:20:56  rogeeff
//  report_level->log_level
//  log format config methods added
//  log interface slightly changed to allow multiple log formats
//  Unused macros deleted
//
//  Revision 1.13  2002/12/08 17:43:55  rogeeff
//  switched to use c_string_literal
//
//  Revision 1.12  2002/11/02 19:31:04  rogeeff
//  merged into the main trank
//

// ***************************************************************************

#endif // BOOST_UNIT_TEST_LOG_HPP

