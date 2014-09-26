/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_FUNDAMENTAL_HPP)
#define BOOST_SPIRIT_FUNDAMENTAL_HPP

///////////////////////////////////////////////////////////////////////////////
#include "boost/spirit/core/meta/impl/fundamental.ipp"

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
//
//  Helper template for counting the number of nodes contained in a
//  given parser type.
//  All parser_category type parsers are counted as nodes.
//
///////////////////////////////////////////////////////////////////////////////
#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)

template <typename ParserT>
struct node_count {

    typedef typename ParserT::parser_category_t parser_category_t;
    typedef impl::nodes<parser_category_t> nodes_t;

    typedef typename impl::count_wrapper<nodes_t>
        ::template result_<ParserT, mpl::int_<0> > count_t;

    BOOST_STATIC_CONSTANT(int, value = count_t::value);
};

#else

template <typename ParserT>
struct node_count {

    typedef typename ParserT::parser_category_t parser_category_t;
    typedef typename impl::nodes<parser_category_t>
        ::template count<ParserT, mpl::int_<0> > count_t;

    BOOST_STATIC_CONSTANT(int, value = count_t::value);
};

#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Helper template for counting the number of leaf nodes contained in a
//  given parser type.
//  Only plain_parser_category type parsers are counted as leaf nodes.
//
///////////////////////////////////////////////////////////////////////////////
#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)

template <typename ParserT>
struct leaf_count {

    typedef typename ParserT::parser_category_t parser_category_t;
    typedef impl::leafs<parser_category_t> nodes_t;

    typedef typename impl::count_wrapper<nodes_t>
        ::template result_<ParserT, mpl::int_<0> > count_t;

    BOOST_STATIC_CONSTANT(int, value = count_t::value);
};

#else

template <typename ParserT>
struct leaf_count {

    typedef typename ParserT::parser_category_t parser_category_t;
    typedef typename impl::leafs<parser_category_t>
        ::template count<ParserT, mpl::int_<0> > count_t;

    BOOST_STATIC_CONSTANT(int, value = count_t::value);
};

#endif

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif // !defined(BOOST_SPIRIT_FUNDAMENTAL_HPP)
