// Copyright David Abrahams 2002. Permission to copy, use,
// modify, sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.
#ifndef WORKAROUND_DWA2002126_HPP
# define WORKAROUND_DWA2002126_HPP

// Compiler/library version workaround macro
//
// Usage:
//
//     #if BOOST_WORKAROUND(BOOST_MSVC, <= 1200)
//        ... // workaround code here
//     #endif
//
// When BOOST_STRICT_CONFIG is defined, expands to 0. Otherwise, the
// first argument must be undefined or expand to a numeric
// value. The above expands to:
//
//     (BOOST_MSVC) != 0 && (BOOST_MSVC) <= 1200
//
// When used for workarounds on the latest known version of a
// compiler, the following convention should be observed:
//
//     #if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1301))
//
// The version number in this case corresponds to the last version in
// which the workaround was known to have been required.  It only has
// value as a comment unless BOOST_DETECT_OUTDATED_WORKAROUNDS is
// defined, in which case a compiler warning or error will be issued
// when the compiler version exceeds the argument to BOOST_TESTED_AT

# ifndef BOOST_STRICT_CONFIG

#  define BOOST_WORKAROUND(symbol, test)                \
        ((symbol != 0) && (1 % (( (symbol test) ) + 1)))
//                              ^ ^           ^ ^
// The extra level of parenthesis nesting above, along with the
// BOOST_OPEN_PAREN indirection below, is required to satisfy the
// broken preprocessor in MWCW 8.3 and earlier.
//
// The basic mechanism works as follows:
//      (symbol test) + 1        =>   2 if the test passes, 1 otherwise
//      1 % ((symbol test) + 1)  =>   1 if the test passes, 0 otherwise
//
// The complication with % is for cooperation with BOOST_TESTED_AT().
// When "test" is BOOST_TESTED_AT(x) and
// BOOST_DETECT_OUTDATED_WORKAROUNDS is #defined,
//
//      symbol test              =>   1 if symbol <= x, -1 otherwise
//      (symbol test) + 1        =>   2 if symbol <= x, 0 otherwise
//      1 % ((symbol test) + 1)  =>   1 if symbol <= x, zero divide otherwise
//

#  ifdef BOOST_DETECT_OUTDATED_WORKAROUNDS
#   define BOOST_OPEN_PAREN (
#   define BOOST_TESTED_AT(value)  > value) ?(-1): BOOST_OPEN_PAREN 1
#  else
#   define BOOST_TESTED_AT(value) != 0
#  endif

# else

#  define BOOST_WORKAROUND(symbol, test) 0

# endif 

#endif // WORKAROUND_DWA2002126_HPP
