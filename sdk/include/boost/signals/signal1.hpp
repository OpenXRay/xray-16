// Boost.Signals library
//
// Copyright (C) 2001 Doug Gregor (gregod@cs.rpi.edu)
//
// Permission to copy, use, sell and distribute this software is granted
// provided this copyright notice appears in all copies.
// Permission to modify the code and to distribute modified code is granted
// provided this copyright notice appears in all copies, and a notice
// that the code was modified is included with the copyright notice.
//
// This software is provided "as is" without express or implied warranty,
// and with no claim as to its suitability for any purpose.
 
// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS_SIGNAL1_HEADER
#define BOOST_SIGNALS_SIGNAL1_HEADER

#define BOOST_SIGNALS_NUM_ARGS 1
#define BOOST_SIGNALS_TEMPLATE_PARMS typename T1
#define BOOST_SIGNALS_TEMPLATE_ARGS T1
#define BOOST_SIGNALS_PARMS T1 a1
#define BOOST_SIGNALS_ARGS a1
#define BOOST_SIGNALS_BOUND_ARGS args->a1
#define BOOST_SIGNALS_ARGS_AS_MEMBERS T1 a1;
#define BOOST_SIGNALS_COPY_PARMS T1 ia1
#define BOOST_SIGNALS_INIT_ARGS :a1(ia1)
#define BOOST_SIGNALS_ARG_TYPES typedef T1 arg2_type; 

#include <boost/signals/signal_template.hpp>

#undef BOOST_SIGNALS_ARG_TYPES
#undef BOOST_SIGNALS_INIT_ARGS
#undef BOOST_SIGNALS_COPY_PARMS
#undef BOOST_SIGNALS_ARGS_AS_MEMBERS
#undef BOOST_SIGNALS_BOUND_ARGS
#undef BOOST_SIGNALS_ARGS
#undef BOOST_SIGNALS_PARMS
#undef BOOST_SIGNALS_TEMPLATE_ARGS
#undef BOOST_SIGNALS_TEMPLATE_PARMS
#undef BOOST_SIGNALS_NUM_ARGS

#endif // BOOST_SIGNALS_SIGNAL1_HEADER
