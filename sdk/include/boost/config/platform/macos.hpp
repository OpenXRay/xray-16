//  (C) Copyright Boost.org 2001. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

//  See http://www.boost.org for most recent version.

//  Mac OS specific config options:

#define BOOST_PLATFORM "Mac OS"

#if __MACH__ && !defined(_MSL_USING_MSL_C)

// Using the Mac OS X system BSD-style C library.

#  define BOOST_NO_CTYPE_FUNCTIONS
#  define BOOST_NO_CWCHAR
#  ifndef BOOST_HAS_UNISTD_H
#    define BOOST_HAS_UNISTD_H
#  endif
// boilerplate code:
#  ifndef TARGET_CARBON
#     include <boost/config/posix_features.hpp>
#  endif
#  ifndef BOOST_HAS_STDINT_H
#     define BOOST_HAS_STDINT_H
#  endif

//
// BSD runtime has pthreads, sigaction, sched_yield and gettimeofday,
// of these only pthreads are advertised in <unistd.h>, so set the 
// other options explicitly:
//
#  define BOOST_HAS_SCHED_YIELD
#  define BOOST_HAS_GETTIMEOFDAY
#  define BOOST_HAS_SIGACTION

#  ifndef __APPLE_CC__

// GCC strange "ignore std" mode works better if you pretend everything
// is in the std namespace, for the most part.

#    define BOOST_NO_STDC_NAMESPACE
#  endif

#else

// Using the MSL C library.

// We will eventually support threads in non-Carbon builds, but we do
// not support this yet.
#  if TARGET_CARBON

#    define BOOST_HAS_MPTASKS

// The MP task implementation of Boost Threads aims to replace MP-unsafe
// parts of the MSL, so we turn on threads unconditionally.
#    define BOOST_HAS_THREADS

// The remote call manager depends on this.
#    define BOOST_BIND_ENABLE_PASCAL

#  endif

#endif

