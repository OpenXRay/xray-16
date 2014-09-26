//  (C) Copyright Gennadiy Rozental 2001-2002.
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied warranty,
//  and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version including documentation.
//
//  File        : $RCSfile: unit_test_suite_ex.hpp,v $
//
//  Version     : $Id: unit_test_suite_ex.hpp,v 1.11 2003/02/13 08:23:56 rogeeff Exp $
//
//  Description : provides extention for unit test framework that allows usage
//  boost::function as a test case base function.
// ***************************************************************************

#ifndef BOOST_UNIT_TEST_SUITE_EX_HPP
#define BOOST_UNIT_TEST_SUITE_EX_HPP

// Boost.Test
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/detail/unit_test_config.hpp>

// BOOST
#include <boost/function/function0.hpp>
#include <boost/function/function1.hpp>

// STL
#include <string>  // for std::string

namespace boost {

namespace unit_test_framework {

// ************************************************************************** //
// **************            function_test_case_ex             ************** //
// ************************************************************************** //

class function_test_case_ex : public test_case {
public:
    typedef function0<void> function_type;

    // Constructor
    function_test_case_ex( function_type f_, std::string const& name_ )
    : test_case( name_, true, 1 ), m_function( f_ ) {}

protected:
    // test case implementation
    void                do_run()        { m_function(); }

private:
    // Data members
    function_type       m_function;
};

// ************************************************************************** //
// **************        parametrized_function_test_case       ************** //
// ************************************************************************** //

template <typename ParamIterator, typename ParameterType>
class parametrized_function_test_case_ex : public test_case {
public:
    typedef function1<void,ParameterType> function_type;

    // Constructor
    parametrized_function_test_case_ex( function_type f_, std::string const& name_,
                                        ParamIterator const& begin_, ParamIterator const& end_ )
    : test_case( name_, true, 0 ), m_first_parameter( begin_ ), m_last_parameter( end_ ), m_function( f_ )
    {
        p_stages_amount.set( detail::distance( begin_, end_ ) );
    }

    // test case implementation
    void                do_init()       { m_curr_parameter = m_first_parameter; }
    void                do_run()        { m_function( *m_curr_parameter ); ++m_curr_parameter; }

private:
    // Data members
    ParamIterator       m_first_parameter;
    ParamIterator       m_last_parameter;
    ParamIterator       m_curr_parameter;

    function_type       m_function;
};

// ************************************************************************** //
// **************               object generators              ************** //
// ************************************************************************** //

inline test_case*
create_test_case( function0<void> const& fct_, std::string name_ )
{
    return new function_test_case_ex( fct_, detail::normalize_test_case_name( name_ ) );
}

template<typename ParamIterator, typename ParameterType>
inline test_case*
create_test_case( function1<void,ParameterType> const& fct_, std::string name_, 
                  ParamIterator const& begin_, ParamIterator const& end_ )
{
    return new parametrized_function_test_case_ex<ParamIterator,ParameterType>(
                    fct_, detail::normalize_test_case_name( name_ ), begin_, end_ );
}

} // unit_test_framework

} // namespace boost

// ***************************************************************************
//  Revision History :
//  
//  $Log: unit_test_suite_ex.hpp,v $
//  Revision 1.11  2003/02/13 08:23:56  rogeeff
//  test case type: virtual method -> property
//
//  Revision 1.10  2002/12/08 17:52:25  rogeeff
//  switched to use c_string_literal
//
//  Revision 1.9  2002/11/02 19:31:04  rogeeff
//  merged into the main trank
//

// ***************************************************************************

#endif // BOOST_UNIT_TEST_SUITE_EX_HPP
