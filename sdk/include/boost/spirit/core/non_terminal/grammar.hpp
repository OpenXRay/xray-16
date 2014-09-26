/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Martin Wille
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_GRAMMAR_HPP)
#define BOOST_SPIRIT_GRAMMAR_HPP

///////////////////////////////////////////////////////////////////////////////
#if defined(BOOST_SPIRIT_THREADSAFE) && defined(SPIRIT_SINGLE_GRAMMAR_INSTANCE)
#undef BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE
#endif

#include "boost/spirit/core/parser.hpp"
#include "boost/spirit/core/non_terminal/parser_context.hpp"
#include "boost/spirit/core/non_terminal/impl/grammar.ipp"

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
//
//  grammar class
//
///////////////////////////////////////////////////////////////////////////////
template <typename DerivedT, typename ContextT = parser_context>
struct grammar
    : public parser<DerivedT>
    , public ContextT::base_t
    BOOST_SPIRIT_GRAMMAR_ID
{
    typedef grammar<DerivedT, ContextT>         self_t;
    typedef DerivedT const&                     embed_t;
    typedef typename ContextT::context_linker_t context_t;
    typedef typename context_t::attr_t          attr_t;

    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, attr_t>::type type;
    };

    grammar() {}
    ~grammar() { impl::grammar_destruct(this); }

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse_main(ScannerT const& scan) const
    { return impl::grammar_parser_parse(this, scan); }

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        typedef parser_scanner_linker<ScannerT> scanner_t;
        BOOST_SPIRIT_CONTEXT_PARSE(scan, *this, scanner_t, context_t, result_t)
    }

    BOOST_SPIRIT_GRAMMAR_STATE
};

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#undef BOOST_SPIRIT_GRAMMAR_ID
#undef BOOST_SPIRIT_GRAMMAR_ACCESS
#undef BOOST_SPIRIT_GRAMMAR_STATE
#endif

