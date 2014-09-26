//  (C) Copyright Boost.org 2001. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version.

//  IBM/Aix specific config options:

#define BOOST_PLATFORM "IBM Aix"

#define BOOST_HAS_UNISTD_H
#define BOOST_HAS_NL_TYPES_H
#define BOOST_HAS_NANOSLEEP
#define BOOST_HAS_CLOCK_GETTIME

// This needs support in "boost/cstdint.hpp" exactly like FreeBSD.
// This platform has header named <inttypes.h> which includes all
// the things needed.
#define BOOST_HAS_STDINT_H

// Threading API's:
#define BOOST_HAS_PTHREADS
#define BOOST_HAS_PTHREAD_DELAY_NP
#define BOOST_HAS_SCHED_YIELD
//#define BOOST_HAS_PTHREAD_YIELD

// boilerplate code:
#include <boost/config/posix_features.hpp>



