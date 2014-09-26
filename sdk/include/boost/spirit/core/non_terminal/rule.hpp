/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_RULE_HPP)
#define BOOST_SPIRIT_RULE_HPP

///////////////////////////////////////////////////////////////////////////////
#include "boost/scoped_ptr.hpp" // for scoped_ptr
#include "boost/spirit/core/parser.hpp"
#include "boost/spirit/core/scanner/scanner.hpp"
#include "boost/spirit/core/non_terminal/parser_context.hpp"
#include "boost/spirit/core/non_terminal/parser_id.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

    namespace impl
    {
        template <typename ScannerT, typename RT>
        struct abstract_parser;

        template <typename ParserT, typename ScannerT, typename RT>
        struct concrete_parser;
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  rule class
    //
    //      The rule is a polymorphic parser that acts as a named place-
    //      holder capturing the behavior of an EBNF expression assigned to
    //      it. The rule is a template class parameterized by the type of the
    //      scanner (ScannerT, see scanner.hpp), the rule's context
    //      (ContextT, see parser_context.hpp), and an arbitrary tag (TagT,
    //      see parser_id.hpp) that allows a rule to be tagged for
    //      identification.
    //
    //      The definition of the rule (its right hand side, RHS) held by the
    //      rule through a ::boost::scoped_ptr. When a rule is seen in the
    //      the RHS of an assignment or copy construction EBNF expression,
    //      the rule is held by the LHS rule by reference.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename ScannerT = scanner<>,
        typename ContextT = parser_context,
        typename TagT = parser_address_tag>
    class rule
        : public parser<rule<ScannerT, ContextT, TagT> >
        , public ContextT::base_t
        , public context_aux<ContextT, rule<ScannerT, ContextT, TagT> >
    {
    public:

        typedef rule<ScannerT, ContextT, TagT>          self_t;
        typedef rule<ScannerT, ContextT, TagT> const&   embed_t;
        typedef parser_scanner_linker<ScannerT>         scanner_t;
        typedef typename ContextT::context_linker_t     context_t;

        typedef typename context_t::attr_t attr_t;
        typedef typename match_result<ScannerT, attr_t>::type result_t;

        template <typename ScannerT2>
        struct result { typedef result_t type; };

        rule();
        ~rule();

        rule(rule const& r)
        : ptr(new impl::concrete_parser<rule, ScannerT, result_t>(r)) {}

        template <typename ParserT>
        rule(ParserT const& p)
        : ptr(new impl::concrete_parser<ParserT, ScannerT, result_t>(p)) {}

        template <typename ParserT>
        rule& operator=(ParserT const& p)
        {
            ptr.reset(
                new impl::concrete_parser<ParserT, ScannerT, result_t>(p));
            return *this;
        }

    //  If this is placed above the templatized assignment
    //  operator, VC6 incorrectly complains ambiguity with
    //  r1 = r2, where r1 and r2 are both rules.
        rule& operator=(rule const& r)
        {
            ptr.reset(
                new impl::concrete_parser<rule, ScannerT, result_t>(r));
            return *this;
        }

        result_t    parse(ScannerT const& scan) const;
        result_t    parse_main(ScannerT const& scan) const;
        parser_id   id() const;

    private:

        typedef impl::abstract_parser<ScannerT, result_t> abstract_parser_t;
        ::boost::scoped_ptr<abstract_parser_t> ptr;
    };

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#include "boost/spirit/core/non_terminal/impl/rule.ipp"
#endif
