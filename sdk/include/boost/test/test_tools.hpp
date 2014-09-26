//  (C) Copyright Gennadiy Rozental 2001-2002.
//  (C) Copyright Ullrich Koethe 2001.
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied warranty,
//  and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version including documentation.
//
//  File        : $RCSfile: test_tools.hpp,v $
//
//  Version     : $Id: test_tools.hpp,v 1.28 2003/02/15 21:54:35 rogeeff Exp $
//
//  Description : contains definition for all test tools in test toolbox
// ***************************************************************************

#ifndef BOOST_TEST_TOOLS_HPP
#define BOOST_TEST_TOOLS_HPP

// Boost.Test
#include <boost/test/detail/unit_test_config.hpp>
#include <boost/test/unit_test_log.hpp>
#include <boost/test/detail/class_properties.hpp>
#include <boost/test/detail/wrap_stringstream.hpp>

// BOOST
#include <boost/cstdlib.hpp> // for boost::exit_success;
#include <boost/config.hpp>  // compilers workarounds
#include <boost/shared_ptr.hpp>

#include <stdexcept>        // for std::exception
#include <cstddef>          // for std::size_t
#include <memory>           // for std::auto_ptr
#include <string>           // for std::string

// ************************************************************************** //
// **************                    TOOL BOX                  ************** //
// ************************************************************************** //

#define BOOST_CHECKPOINT(message_) \
    boost::test_toolbox::detail::checkpoint_impl( \
        boost::wrap_stringstream().ref() << message_, __FILE__, __LINE__)

#define BOOST_WARN(predicate) \
    boost::test_toolbox::detail::warn_and_continue_impl((predicate), \
        boost::wrap_stringstream().ref() << #predicate, __FILE__, __LINE__)

#define BOOST_CHECK(predicate) \
    boost::test_toolbox::detail::test_and_continue_impl((predicate), \
        boost::wrap_stringstream().ref() << #predicate, __FILE__, __LINE__)

#define BOOST_CHECK_EQUAL(left_, right_) \
    boost::test_toolbox::detail::equal_and_continue_impl((left_), (right_), \
        boost::wrap_stringstream().ref() << #left_ " == " #right_, __FILE__, __LINE__)

#define BOOST_CHECK_CLOSE(left_, right_, tolerance_src) \
    boost::test_toolbox::detail::compare_and_continue_impl((left_), (right_), (tolerance_src),\
        boost::wrap_stringstream().ref() << #left_ " ~= " #right_, __FILE__, __LINE__)

#define BOOST_BITWISE_EQUAL(left_, right_) \
    boost::test_toolbox::detail::bitwise_equal_and_continue_impl((left_), (right_), \
        boost::wrap_stringstream().ref() << #left_ " =.= " #right_, __FILE__, __LINE__)

#define BOOST_REQUIRE(predicate) \
    boost::test_toolbox::detail::test_and_throw_impl((predicate), \
        boost::wrap_stringstream().ref() << #predicate, __FILE__, __LINE__)

#define BOOST_MESSAGE(message_) \
    boost::test_toolbox::detail::message_impl( \
        boost::wrap_stringstream().ref() << message_, __FILE__, __LINE__)

#define BOOST_WARN_MESSAGE(predicate, message_) \
    boost::test_toolbox::detail::warn_and_continue_impl((predicate), \
        boost::wrap_stringstream().ref() << message_, __FILE__, __LINE__,false)

#define BOOST_CHECK_MESSAGE(predicate, message_) \
    boost::test_toolbox::detail::test_and_continue_impl((predicate), \
        boost::wrap_stringstream().ref() << message_, __FILE__, __LINE__,false)

#define BOOST_REQUIRE_MESSAGE(predicate, message_) \
    boost::test_toolbox::detail::test_and_throw_impl((predicate), \
        boost::wrap_stringstream().ref() << message_, __FILE__, __LINE__,false)

#define BOOST_CHECK_PREDICATE( predicate, arg_list_size, arg_list ) \
    boost::test_toolbox::detail::test_and_continue_impl(predicate, BOOST_PLACE_PREDICATE_ARGS ## arg_list_size arg_list, \
        boost::wrap_stringstream().ref() << #predicate << "("\
        << BOOST_PRINT_PREDICATE_ARGS ## arg_list_size arg_list << ")", __FILE__, __LINE__)

#define BOOST_REQUIRE_PREDICATE( predicate, arg_list_size, arg_list ) \
    boost::test_toolbox::detail::test_and_throw_impl(predicate, BOOST_PLACE_PREDICATE_ARGS ## arg_list_size arg_list, \
        boost::wrap_stringstream().ref() << #predicate << "("\
        << BOOST_PRINT_PREDICATE_ARGS ## arg_list_size arg_list << ")", __FILE__, __LINE__)

#define BOOST_ERROR(message_) BOOST_CHECK_MESSAGE( false, message_ )

#define BOOST_FAIL(message_) BOOST_REQUIRE_MESSAGE( false, message_ )

#define BOOST_CHECK_THROW( statement, exception ) \
    try { statement; BOOST_ERROR( "exception "#exception" is expected" ); } \
    catch( exception const& ) { \
        BOOST_CHECK_MESSAGE( true, "exception "#exception" is caught" ); \
    }

#define BOOST_CHECK_NO_THROW( statement ) \
    try { statement; BOOST_CHECK_MESSAGE( true, "no exceptions was thrown by "#statement ); } \
    catch( ... ) { \
        BOOST_ERROR( "exception was thrown by "#statement ); \
    }

#define BOOST_CHECK_EQUAL_COLLECTIONS(left_begin_, left_end_, right_begin_) \
    boost::test_toolbox::detail::equal_and_continue_impl( (left_begin_), (left_end_), (right_begin_),\
        boost::wrap_stringstream().ref() << \
            "{" #left_begin_ ", " #left_end_ "}" " == {" #right_begin_ ", ...}", __FILE__, __LINE__)

#define BOOST_IS_DEFINED(symb) boost::test_toolbox::detail::is_defined_impl( #symb, BOOST_STRINGIZE(= symb) )

// ***************************** //
// helper macros

#define BOOST_PLACE_PREDICATE_ARGS1( first_ ) first_
#define BOOST_PLACE_PREDICATE_ARGS2( first_, second_ ) first_, second_

#define BOOST_PRINT_PREDICATE_ARGS1( first_ ) #first_
#define BOOST_PRINT_PREDICATE_ARGS2( first_, second_ ) #first_ << ", " << #second_

// ***************************** //
// depricated interface

#define BOOST_TEST(predicate)            BOOST_CHECK(predicate)
#define BOOST_CRITICAL_TEST(predicate)   BOOST_REQUIRE(predicate)
#define BOOST_CRITICAL_ERROR(message_)   BOOST_FAIL(message_)

namespace boost {

namespace test_toolbox {

namespace detail {

using unit_test_framework::c_string_literal;

// ************************************************************************** //
// **************            extended_predicate_value          ************** //
// ************************************************************************** //

struct extended_predicate_value {
    // Constructor
    explicit    extended_predicate_value( bool predicate_value_ )
    : p_predicate_value( predicate_value_ ), p_message( new wrap_stringstream ) {}

    extended_predicate_value( extended_predicate_value const& rhs )
    : p_predicate_value( rhs.p_predicate_value.get() ), 
      p_message( const_cast<extended_predicate_value&>(rhs).p_message ) {}

    bool        operator!() const { return !p_predicate_value.get(); }

    BOOST_READONLY_PROPERTY( bool, 0, () )  p_predicate_value;
    std::auto_ptr<wrap_stringstream>        p_message;
};

// ************************************************************************** //
// **************                test_tool_failed              ************** //
// ************************************************************************** //

// exception used to implement critical checks

struct test_tool_failed : public std::exception {
};

// ************************************************************************** //
// **************            TOOL BOX Implementation           ************** //
// ************************************************************************** //

void
checkpoint_impl( wrap_stringstream& message_, c_string_literal file_name_, int line_num_ );

//____________________________________________________________________________//

void
message_impl( wrap_stringstream& message_, c_string_literal file_name_, int line_num_ );

//____________________________________________________________________________//

// ************************************* //

void
warn_and_continue_impl( bool predicate_, wrap_stringstream& message_,
                        c_string_literal file_name_, int line_num_,
                        bool add_fail_pass_ = true );

//____________________________________________________________________________//

void
warn_and_continue_impl( extended_predicate_value const& v_, wrap_stringstream& message_,
                        c_string_literal file_name_, int line_num_,
                        bool add_fail_pass_ = true );

//____________________________________________________________________________//

// ************************************* //

bool  // return true if error detected
test_and_continue_impl( bool predicate_, wrap_stringstream& message_,
                        c_string_literal file_name_, int line_num_,
                        bool add_fail_pass_ = true,
                        unit_test_framework::log_level log_level_ = unit_test_framework::log_all_errors );
void
test_and_throw_impl   ( bool predicate_, wrap_stringstream& message_,
                        c_string_literal file_name_, int line_num_,
                        bool add_fail_pass_ = true,
                        unit_test_framework::log_level log_level_ = unit_test_framework::log_fatal_errors );

//____________________________________________________________________________//

bool
test_and_continue_impl( extended_predicate_value const& v_, wrap_stringstream& message_,
                        c_string_literal file_name_, int line_num_,
                        bool add_fail_pass_ = true,
                        unit_test_framework::log_level log_level_ = unit_test_framework::log_all_errors );

//____________________________________________________________________________//

// Borland bug workaround
#if defined(__BORLANDC__) && (__BORLANDC__ < 0x560)
bool
test_and_continue_impl( void* ptr, wrap_stringstream& message_,
                        c_string_literal file_name_, int line_num_,
                        bool add_fail_pass_ = true,
                        unit_test_framework::log_level log_level_ = unit_test_framework::log_all_errors )
{
    return test_and_continue_impl( !!ptr, message_, file_name_, line_num_, add_fail_pass_, log_level_ );
}
#endif

//____________________________________________________________________________//

void
test_and_throw_impl   ( extended_predicate_value const& v_, wrap_stringstream& message_,
                        c_string_literal file_name_, int line_num_,
                        bool add_fail_pass_ = true,
                        unit_test_framework::log_level log_level_ = unit_test_framework::log_fatal_errors );

//____________________________________________________________________________//

template<typename ArgType, typename Predicate>
inline bool
test_and_continue_impl( Predicate const& pred_, ArgType const& arg_,
                        wrap_stringstream& message_,
                        c_string_literal file_name_, int line_num_,
                        unit_test_framework::log_level log_level_ = unit_test_framework::log_all_errors )
{
    bool predicate = pred_( arg_ );

    if( !predicate ) {
        return test_and_continue_impl( predicate,
                                       wrap_stringstream().ref() << "test " << message_ << " failed for " << arg_,
                                       file_name_, line_num_, false, log_level_ );
    }

    return test_and_continue_impl( predicate, message_, file_name_, line_num_, true, log_level_ );
}

//____________________________________________________________________________//

template<typename ArgType, typename Predicate>
inline void
test_and_throw_impl   ( Predicate const& pred_, ArgType const& arg_,
                        wrap_stringstream& message_,
                        c_string_literal file_name_, int line_num_,
                        unit_test_framework::log_level log_level_ = unit_test_framework::log_fatal_errors )
{
    if( test_and_continue_impl( arg_, pred_, message_, file_name_, line_num_, log_level_ ) ) {
        throw test_tool_failed(); // error already reported by test_and_continue_impl
    }
}

//____________________________________________________________________________//

template<typename First, typename Second, typename Predicate>
inline bool
test_and_continue_impl( Predicate const& pred_, First const& first_, Second const& second_,
                        wrap_stringstream& message_,
                        c_string_literal file_name_, int line_num_,
                        unit_test_framework::log_level log_level_ = unit_test_framework::log_all_errors )
{
    bool predicate = pred_( first_, second_ );

    if( !predicate ) {
        return test_and_continue_impl( predicate,
            wrap_stringstream().ref() << "test " << message_ << " failed for (" << first_ << ", " << second_ << ")",
            file_name_, line_num_, false, log_level_ );
    }

    return test_and_continue_impl( predicate, message_, file_name_, line_num_, true, log_level_ );
}

//____________________________________________________________________________//

template<typename First, typename Second, typename Predicate>
inline void
test_and_throw_impl( First const& first_, Second const& second_, Predicate const& pred_,
                     wrap_stringstream& message_, c_string_literal file_name_, int line_num_,
                     unit_test_framework::log_level log_level_ = unit_test_framework::log_fatal_errors )
{
    if( test_and_continue_impl( first_, second_, pred_, message_, file_name_, line_num_, log_level_ ) ) {
        throw test_tool_failed(); // error already reported by test_and_continue_impl
    }
}

//____________________________________________________________________________//

// ************************************* //

bool
equal_and_continue_impl( c_string_literal left_, c_string_literal right_, wrap_stringstream& message_,
                         c_string_literal file_name_, int line_num_,
                         unit_test_framework::log_level log_level_ = unit_test_framework::log_all_errors );

//____________________________________________________________________________//

template <class Left, class Right>
inline bool
equal_and_continue_impl( Left const& left_, Right const& right_,
                         wrap_stringstream& message_, c_string_literal file_name_, int line_num_,
                         unit_test_framework::log_level log_level_ = unit_test_framework::log_all_errors )
{
    bool predicate = (left_ == right_);

    if( !predicate ) {
        return test_and_continue_impl( predicate,
            wrap_stringstream().ref() << "test " << message_
                            << " failed [" << left_ << " != " << right_ << "]",
            file_name_, line_num_, false, log_level_ );
    }

    return test_and_continue_impl( predicate, message_, file_name_, line_num_, true, log_level_ );
}

//____________________________________________________________________________//

template <class Left, class Right>
inline void
equal_and_continue_impl( Left left_begin_, Left left_end_, Right right_begin_,
                         wrap_stringstream& message_,
                         c_string_literal file_name_, int line_num_,
                         unit_test_framework::log_level log_level_ = unit_test_framework::log_all_errors )
{
    for( ;left_begin_ != left_end_; ++left_begin_, ++right_begin_ )
        equal_and_continue_impl( *left_begin_, *right_begin_, message_, file_name_, line_num_, log_level_ );
}

//____________________________________________________________________________//

// ************************************* //

template<typename FPT, typename ToleranceSource>
inline bool
compare_and_continue_impl( FPT left_, FPT right_, ToleranceSource tolerance_src,
                           wrap_stringstream& message_,
                           c_string_literal file_name_, int line_num_,
                           unit_test_framework::log_level log_level_ = unit_test_framework::log_all_errors )
{
    bool predicate = check_is_closed( left_, right_, tolerance_src );

    if( !predicate ) {
        return test_and_continue_impl( predicate,
            wrap_stringstream().ref() << "test " << message_
                            << " failed [" << left_ << " !~= " << right_
                            << " (+/-" << compute_tolerance( tolerance_src, left_ ) << ")]",
            file_name_, line_num_, false, log_level_ );
    }

    return test_and_continue_impl( predicate, message_, file_name_, line_num_, true, log_level_ );
}

//____________________________________________________________________________//

template <class Left, class Right>
inline void
bitwise_equal_and_continue_impl( Left const& left_, Right const& right_,
                                 wrap_stringstream& message_, char const* file_name_, int line_num_,
                                 unit_test_framework::log_level log_level_ = unit_test_framework::log_all_errors )
{
    std::size_t left_bit_size  = sizeof(Left)*CHAR_BIT;
    std::size_t right_bit_size = sizeof(Right)*CHAR_BIT;

    static Left const L1( 1 );
    static Right const R1( 1 );

    if( left_bit_size != right_bit_size )
        warn_and_continue_impl( false, wrap_stringstream().ref() << message_ << ": operands bit sizes does not coinside", 
                                file_name_, line_num_, false );

    std::size_t total_bits = left_bit_size < right_bit_size ? left_bit_size : right_bit_size;

    for( std::size_t counter = 0; counter < total_bits; ++counter ) {
        bool predicate = ( left_ & ( L1 << counter ) ) == ( right_ & ( R1 << counter ) );

        test_and_continue_impl( predicate, wrap_stringstream().ref() << message_.str() << " in the position " << counter,
                                file_name_, line_num_, true, log_level_ );
    }
}

//____________________________________________________________________________//

// ************************************* //

bool
is_defined_impl( c_string_literal symbol_name_, c_string_literal symbol_value_ );

//____________________________________________________________________________//

} // namespace detail

// ************************************************************************** //
// **************               output_test_stream             ************** //
// ************************************************************************** //

// class to be used to simplify testing of ostream print functions

class output_test_stream : public 
#ifdef BOOST_NO_STRINGSTREAM
    std::ostrstream
#else
    std::ostringstream
#endif // BOOST_NO_STRINGSTREAM
{
    typedef detail::extended_predicate_value result_type;
    typedef detail::c_string_literal         c_string_literal;
public:
    // Constructor
    explicit        output_test_stream( std::string const&  pattern_file_name = std::string(),
                                        bool                match_or_save     = true );
    explicit        output_test_stream( c_string_literal    pattern_file_name,
                                        bool                match_or_save     = true );

    // Destructor
    ~output_test_stream();

    // checking function
    result_type     is_empty( bool flush_stream_ = true );
    result_type     check_length( std::size_t length_, bool flush_stream_ = true );
    result_type     is_equal( c_string_literal arg_, bool flush_stream_ = true );
    result_type     is_equal( std::string const& arg_, bool flush_stream_ = true );
    result_type     is_equal( c_string_literal arg_, std::size_t n_, bool flush_stream_ = true );
    bool            match_pattern( bool flush_stream_ = true );

    // helper function
    void            flush();
    std::size_t     length();

private:
    void            sync();

    struct Impl;
    boost::shared_ptr<Impl> m_pimpl;
};

} // namespace test_toolbox

} // namespace boost

// ***************************************************************************
//  Revision History :
//
//  $Log: test_tools.hpp,v $
//  Revision 1.28  2003/02/15 21:54:35  rogeeff
//  is_defined made portable
//
//  Revision 1.27  2003/02/14 06:42:18  rogeeff
//  Mingw fix for is_defined
//  Visual age fix for extendeded boolean value
//
//  Revision 1.26  2003/02/14 00:56:23  rogeeff
//  added std to size_t
//
//  Revision 1.25  2003/02/13 08:18:35  rogeeff
//  BOOST_BITWISE_EQUAL introduced
//  BOOST_CHECK_NO_THROW introduced
//  report_level -> log_level
//  C strings eliminated
//  other minor fixes
//
//  Revision 1.24  2002/12/08 17:54:09  rogeeff
//  wrapstrstream separated in standalone file and renamed
//  switched to use c_string_literal
//
//  Revision 1.23  2002/11/03 03:06:16  rogeeff
//  wrapstream constructor issue fix revisited
//
//  Revision 1.22  2002/11/02 19:31:04  rogeeff
//  merged into the main trank
//

// ***************************************************************************

#endif // BOOST_TEST_TOOLS_HPP
