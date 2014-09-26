//  (C) Copyright Boost.org 2001. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version.

//  hpux specific config options:

#define BOOST_PLATFORM "HP-UX"

// In principle, HP-UX has a nice <stdint.h> under the name <inttypes.h>
// However, it has the following problem:
// Use of UINT32_C(0) results in "0u l" for the preprocessed source
// (verifyable with gcc 2.95.3, assumed for HP aCC)
// #define BOOST_HAS_STDINT_H

#define BOOST_NO_SWPRINTF 
#define BOOST_NO_CWCTYPE

// boilerplate code:
#define BOOST_HAS_UNISTD_H
#include <boost/config/posix_features.hpp>

// HPUX has an incomplete pthreads implementation, so it doesn't
// define _POSIX_THREADS, but it might be enough to implement
// Boost.Threads.
#if !defined(BOOST_HAS_PTHREADS) && defined(_POSIX_THREAD_ATTR_STACKADDR)
# define BOOST_HAS_PTHREADS 
#endif

// the following are always available:
#ifndef BOOST_HAS_GETTIMEOFDAY
#  define BOOST_HAS_GETTIMEOFDAY
#endif
#ifndef BOOST_HAS_SCHED_YIELD
#    define BOOST_HAS_SCHED_YIELD
#endif
#ifndef BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
#    define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
#endif
#ifndef BOOST_HAS_NL_TYPES_H
#    define BOOST_HAS_NL_TYPES_H
#endif
#ifndef BOOST_HAS_NANOSLEEP
#    define BOOST_HAS_NANOSLEEP
#endif
#ifndef BOOST_HAS_GETTIMEOFDAY
#    define BOOST_HAS_GETTIMEOFDAY
#endif
#ifndef BOOST_HAS_DIRENT_H
#    define BOOST_HAS_DIRENT_H
#endif
#ifndef BOOST_HAS_CLOCK_GETTIME
#    define BOOST_HAS_CLOCK_GETTIME
#endif
#ifndef BOOST_HAS_SIGACTION
#  define BOOST_HAS_SIGACTION
#endif

