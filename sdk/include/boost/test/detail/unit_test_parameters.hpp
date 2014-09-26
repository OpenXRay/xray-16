//  (C) Copyright Gennadiy Rozental 2001-2002.
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied warranty,
//  and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version including documentation.
//
//  File        : $RCSfile: unit_test_parameters.hpp,v $
//
//  Version     : $Id: unit_test_parameters.hpp,v 1.7 2003/02/13 08:07:20 rogeeff Exp $
//
//  Description : storage for unit test framework parameters information
// ***************************************************************************

#ifndef BOOST_UNIT_TEST_PARAMETERS_HPP
#define BOOST_UNIT_TEST_PARAMETERS_HPP

// STL
#include <string>   // std::string

#include <boost/test/detail/unit_test_config.hpp>

namespace boost {

namespace unit_test_framework {

// framework parameters and there corresponding command-line arguments
c_string_literal const LOG_LEVEL         = "BOOST_TEST_LOG_LEVEL";              // --log_level
c_string_literal const NO_RESULT_CODE    = "BOOST_TEST_RESULT_CODE";            // --result_code
c_string_literal const REPORT_LEVEL      = "BOOST_TEST_REPORT_LEVEL";           // --report_level
c_string_literal const TESTS_TO_RUN      = "BOOST_TESTS_TO_RUN";                // --run_test
c_string_literal const SAVE_TEST_PATTERN = "BOOST_TEST_SAVE_PATTERN";           // --save_pattern
c_string_literal const BUILD_INFO        = "BOOST_TEST_BUILD_INFO";             // --build_info
c_string_literal const CATCH_SYS_ERRORS  = "BOOST_TEST_CATCH_SYSTEM_ERRORS";    // --catch_system_errors
c_string_literal const REPORT_FORMAT     = "BOOST_TEST_REPORT_FORMAT";          // --report_format
c_string_literal const LOG_FORMAT        = "BOOST_TEST_LOG_FORMAT";             // --log_format
c_string_literal const OUTPUT_FORMAT     = "BOOST_TEST_OUTPUT_FORMAT";          // --output_format

enum report_level                             { CONFIRMATION_REPORT, SHORT_REPORT, DETAILED_REPORT, NO_REPORT };
c_string_literal const report_level_names[] = { "confirm"          , "short"     , "detailed"     , "no"      };

enum output_format { HRF /* human readable format */, XML /* XML */ };

std::string retrieve_framework_parameter( c_string_literal parameter_name_, int* argc_, char** argv_ );

} // namespace unit_test_framework

} // namespace boost

// ***************************************************************************
//  Revision History :
//  
//  $Log: unit_test_parameters.hpp,v $
//  Revision 1.7  2003/02/13 08:07:20  rogeeff
//  report_format log_format and output_format introduced
//
//  Revision 1.6  2002/12/08 17:38:44  rogeeff
//  catch_system_error framework cla parameter and envronment variable introduced
//  switch to use c_string_literal
//
//  Revision 1.5  2002/11/02 19:31:05  rogeeff
//  merged into the main trank
//

// ***************************************************************************

#endif // BOOST_UNIT_TEST_CONFIG_HPP
