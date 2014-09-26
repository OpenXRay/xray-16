/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001-2003 Daniel Nuffer
    Copyright (c) 2001-2003 Hartmut Kaiser
    Copyright (c) 2002-2003 Martin Wille
    Copyright (c) 2002 Raghavendra Satish
    Copyright (c) 2001 Bruce Florman
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_CORE_MAIN_HPP)
#define BOOST_SPIRIT_CORE_MAIN_HPP

#include "boost/spirit/debug.hpp"

///////////////////////////////////////////////////////////////////////////////
//
//  Spirit.Core includes
//
///////////////////////////////////////////////////////////////////////////////

//  Spirit.Core.Kernel
#include "boost/spirit/core/basics.hpp"
#include "boost/spirit/core/match.hpp"
#include "boost/spirit/core/parser.hpp"

//  Spirit.Core.Primitives
#include "boost/spirit/core/primitives/primitives.hpp"
#include "boost/spirit/core/primitives/numerics.hpp"

//  Spirit.Core.Meta
#include "boost/spirit/core/meta/fundamental.hpp"
#include "boost/spirit/core/meta/parser_traits.hpp"
#include "boost/spirit/core/meta/impl/parser_type.hpp"
#include "boost/spirit/core/meta/traverse.hpp"

//  Spirit.Core.Scanner
#include "boost/spirit/core/scanner/scanner.hpp"
#include "boost/spirit/core/scanner/skipper.hpp"

//  Spirit.Core.NonTerminal
#include "boost/spirit/core/non_terminal/subrule.hpp"
#include "boost/spirit/core/non_terminal/rule.hpp"
#include "boost/spirit/core/non_terminal/grammar.hpp"

//  Spirit.Core.Composite
#include "boost/spirit/core/composite/actions.hpp"
#include "boost/spirit/core/composite/composite.hpp"
#include "boost/spirit/core/composite/directives.hpp"
#include "boost/spirit/core/composite/operators.hpp"
#include "boost/spirit/core/composite/epsilon.hpp"

#if defined(BOOST_SPIRIT_DEBUG)
    //////////////////////////////////
    #include "boost/spirit/debug/parser_names.hpp"

#endif // BOOST_SPIRIT_DEBUG

#endif // BOOST_SPIRIT_CORE_MAIN_HPP

