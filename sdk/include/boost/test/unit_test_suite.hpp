//  (C) Copyright Gennadiy Rozental 2001-2002.
//  (C) Copyright Ullrich Koethe 2001.
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied warranty,
//  and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version including documentation.
//
//  File        : $RCSfile: unit_test_suite.hpp,v $
//
//  Version     : $Id: unit_test_suite.hpp,v 1.13 2003/02/13 08:23:08 rogeeff Exp $
//
//  Description : defines all classes in test_case hierarchy and object generators
//  for them.
// ***************************************************************************

#ifndef BOOST_UNIT_TEST_SUITE_HPP
#define BOOST_UNIT_TEST_SUITE_HPP

// Boost.Test
#include <boost/test/detail/unit_test_monitor.hpp>
#include <boost/test/detail/unit_test_config.hpp>
#include <boost/test/detail/class_properties.hpp>

// BOOST
#include <boost/shared_ptr.hpp>

// STL
#include <string>  // for std::string

#define BOOST_TEST_CASE( function ) \
boost::unit_test_framework::create_test_case((function), #function )
#define BOOST_CLASS_TEST_CASE( function, tc_instance ) \
boost::unit_test_framework::create_test_case((function), #function, tc_instance )
#define BOOST_PARAM_TEST_CASE( function, begin, end ) \
boost::unit_test_framework::create_test_case((function), #function, (begin), (end) )
#define BOOST_PARAM_CLASS_TEST_CASE( function, tc_instance, begin, end ) \
boost::unit_test_framework::create_test_case((function), #function, tc_instance, (begin), (end) )
#define BOOST_TEST_SUITE( testsuite_name ) \
( new boost::unit_test_framework::test_suite( testsuite_name ) )

namespace boost {

namespace unit_test_framework {

// ************************************************************************** //
// **************                   test_case                  ************** //
// ************************************************************************** //

class test_case {
public:
    typedef detail::unit_test_monitor::error_level error_level_type;

    // Destructor
    virtual             ~test_case()    {}

    // number of tests in this test case
    virtual unit_test_counter size() const;

    // execute this method to run the test case
    void                run();

    // public properties
    BOOST_READONLY_PROPERTY( int, 2, (test_case,test_suite) )
                        p_timeout;                  // timeout for the excecution monitor
    BOOST_READONLY_PROPERTY( unit_test_counter, 1, (test_suite) )
                        p_expected_failures;        // number of aseertions that are expected to fail in this test case
    BOOST_READONLY_PROPERTY( bool, 0, () )
                        p_type;                     // true = test case, false - test suite
    BOOST_READONLY_PROPERTY( std::string, 0, () )
                        p_name;                     // name for this test case


protected:
    // protected properties
    BOOST_READONLY_PROPERTY( bool, 2, (test_case,test_suite) )
                        p_compound_stage;           // used to properly manage progress report
    BOOST_READWRITE_PROPERTY( unit_test_counter )
                        p_stages_amount;            // number of stages this test consist of; stage could be another test case
                                                    // like with test_suite, another parameterized test for parameterized_test_case
                                                    // or 1 stage that reflect single test_case behaviour

    // access methods
    void                curr_stage_is_compound();

    // Constructor
    explicit            test_case( std::string const&   name_,
                                   bool                 type_,
                                   unit_test_counter    stages_number_,
                                   bool                 monitor_run_    = true );

    // test case implementation hooks to be called with unit_test_monitor or alone
    virtual void        do_init()       {}
    virtual void        do_run()        {}
    virtual void        do_destroy()    {}

private:
    // Data members
    bool                m_monitor_run;              // true - unit_test_monitor will be user to monitor running
                                                    // of implementation function
    static bool         s_abort_testing;            // used to flag critical error and try gracefully stop testing
};

//____________________________________________________________________________//

// ************************************************************************** //
// **************              function_test_case              ************** //
// ************************************************************************** //

class function_test_case : public test_case {
public:
    typedef void  (*function_type)();

    // Constructor
    function_test_case( function_type f_, std::string const& name_ )
    : test_case( name_, true, 1 ), m_function( f_ ) {}

protected:
    // test case implementation
    void                do_run()        { m_function(); }

private:
    // Data members
    function_type       m_function;
};

// ************************************************************************** //
// **************                class_test_case               ************** //
// ************************************************************************** //

template<class UserTestCase>
class class_test_case : public test_case {
public:
    typedef void  (UserTestCase::*function_type)();

    // Constructor
    class_test_case( function_type f_, std::string const& name_, boost::shared_ptr<UserTestCase> const& user_test_case_ )
    : test_case( name_, true, 1 ), m_user_test_case( user_test_case_ ), m_function( f_ ) 
    {}

private:
    // test case implementation
    void                do_run()
    { 
        if( (!!m_user_test_case) && m_function ) 
            ((*m_user_test_case).*m_function)();
    }
    void                do_destroy()
    { 
        m_user_test_case.reset(); // t ofree the reference to the shared use test case instance
    }

    // Data members
    boost::shared_ptr<UserTestCase> m_user_test_case;
    function_type       m_function;
};

// ************************************************************************** //
// **************        parametrized_function_test_case       ************** //
// ************************************************************************** //

template <typename ParamIterator, typename ParameterType>
class parametrized_function_test_case : public test_case {
public:
    typedef void  (*function_type)( ParameterType );

    // Constructor
    parametrized_function_test_case( function_type f_, std::string const& name_,
                                     ParamIterator const& par_begin_, ParamIterator const& par_end_ )
    : test_case( name_, true, 0 ), m_first_parameter( par_begin_ ), m_last_parameter( par_end_ ), m_function( f_ )
    {
       // the typecasts are here to keep Borland C++ Builder 5 happy, for other compilers they have no effect:
       p_stages_amount.set( detail::distance( (ParamIterator)par_begin_, (ParamIterator)par_end_ ) );
    }

    // test case implementation
    void                do_init()       { m_curr_parameter = m_first_parameter; }
    void                do_run()        { m_function( *m_curr_parameter++ ); }

private:
    // Data members
    ParamIterator       m_first_parameter;
    ParamIterator       m_last_parameter;
    ParamIterator       m_curr_parameter;

    function_type       m_function;
};

// ************************************************************************** //
// **************         parametrized_class_test_case         ************** //
// ************************************************************************** //

template <class UserTestCase, class ParamIterator, typename ParameterType>
class parametrized_class_test_case : public test_case {
public:
    typedef void  (UserTestCase::*function_type)( ParameterType );

    // Constructor
    parametrized_class_test_case( function_type f_, std::string const& name_, boost::shared_ptr<UserTestCase>const & user_test_case_,
                                  ParamIterator const& par_begin_, ParamIterator const& par_end_ )
    : test_case( name_, true, 0 ), m_first_parameter( par_begin_ ), m_last_parameter( par_end_ ),
      m_user_test_case( user_test_case_ ), m_function( f_ )
    {
       // the typecasts are here to keep Borland C++ Builder 5 happy, for other compilers they have no effect:
       p_stages_amount.set( detail::distance( (ParamIterator)par_begin_, (ParamIterator)par_end_ ) );
    }

    // test case implementation
    void                do_init()       { m_curr_parameter = m_first_parameter; }
    void                do_run()        { ((*m_user_test_case).*m_function)( *m_curr_parameter++ ); }
    void                do_destroy()    { m_user_test_case.reset(); }

private:
    // Data members
    ParamIterator       m_first_parameter;
    ParamIterator       m_last_parameter;
    ParamIterator       m_curr_parameter;

    boost::shared_ptr<UserTestCase> m_user_test_case;
    function_type       m_function;
};

// ************************************************************************** //
// **************                  test_suite                  ************** //
// ************************************************************************** //

class test_suite : public test_case {
public:
    // Constructor
    explicit test_suite( std::string const& name_ = "Master" );

    // Destructor
    virtual             ~test_suite();

    // test case list management
    void                add( test_case* tc_, unit_test_counter expected_failures_ = 0, int timeout_ = 0 );

    // access methods
    unit_test_counter   size() const;

    // test case implementation
    void                do_init();
    void                do_run();

private:
    // Data members
    struct Impl;
    boost::shared_ptr<Impl> m_pimpl;
};

// ************************************************************************** //
// **************               object generators              ************** //
// ************************************************************************** //

namespace detail {

std::string const& normalize_test_case_name( std::string& name_ );

} // namespace detail

//____________________________________________________________________________//

inline test_case*
create_test_case( void (*fct_)(), std::string name_ )
{
    return new function_test_case( fct_, detail::normalize_test_case_name( name_ ) );
}

//____________________________________________________________________________//

template<class UserTestCase>
inline test_case*
create_test_case( void (UserTestCase::*fct_)(), std::string name_, boost::shared_ptr<UserTestCase> const& user_test_case_ )
{
    return new class_test_case<UserTestCase>( fct_, detail::normalize_test_case_name( name_ ), user_test_case_ );
}

//____________________________________________________________________________//

template<typename ParamIterator, typename ParamType>
inline test_case*
create_test_case( void (*fct_)( ParamType ), std::string name_, ParamIterator const& par_begin_, ParamIterator const& par_end_ )
{
    return new parametrized_function_test_case<ParamIterator,ParamType>(
        fct_, detail::normalize_test_case_name( name_ ), par_begin_, par_end_ );
}

//____________________________________________________________________________//

template<class UserTestCase, typename ParamIterator, typename ParamType>
inline test_case*
create_test_case( void (UserTestCase::*fct_)( ParamType ), std::string name_, boost::shared_ptr<UserTestCase> const& user_test_case_,
                  ParamIterator const& par_begin_, ParamIterator const& par_end_ )
{
    return new parametrized_class_test_case<UserTestCase,ParamIterator,ParamType>(
        fct_, detail::normalize_test_case_name( name_ ), user_test_case_, par_begin_, par_end_ );
}

//____________________________________________________________________________//

} // unit_test_framework

} // namespace boost

// ***************************************************************************
//  Revision History :
//  
//  $Log: unit_test_suite.hpp,v $
//  Revision 1.13  2003/02/13 08:23:08  rogeeff
//  C strings eliminated
//  type: virtual method -> property
//  const added to user test case shared instance reference
//
//  Revision 1.12  2002/12/08 17:51:04  rogeeff
//  notion of number of stages is separated from number of test cases, so that
//  parameterized test case in reported as one test case
//  fixed bug in both parameterized function/class test cases with not iterating
//  parameter iterator if exception occured
//  switched to use c_string_literal
//
//  Revision 1.11  2002/11/02 19:31:04  rogeeff
//  merged into the main trank
//

// ***************************************************************************

#endif // BOOST_UNIT_TEST_SUITE_HPP

