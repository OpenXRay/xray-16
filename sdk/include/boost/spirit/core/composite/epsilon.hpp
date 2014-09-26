/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2002-2003 Martin Wille
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#ifndef BOOST_SPIRIT_EPSILON_HPP
#define BOOST_SPIRIT_EPSILON_HPP

////////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_PARSER_HPP)
#include "boost/spirit/core/parser.hpp"
#endif

#if !defined(BOOST_SPIRIT_PARSER_TRAITS_HPP)
#include "boost/spirit/core/meta/parser_traits.hpp"
#endif

#if !defined(BOOST_SPIRIT_COMPOSITE_HPP)
#include "boost/spirit/core/composite/composite.hpp"
#endif

////////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
//
//  condition_parser class
//
//      handles expresions of the form
//          epsilon_p(cond)
//      where cond is a function or a functor that returns a value
//      suitable to be used in boolean context. The expression returns
//      a parser that returns an empty match when the condition evaluates
//      to true.
//
///////////////////////////////////////////////////////////////////////////////
template <typename CondT, bool positive = true>
struct condition_parser
    : public impl::subject<CondT, parser<condition_parser<CondT, positive> > >
{
    typedef condition_parser<CondT, positive> self_t;
    typedef impl::subject<CondT, parser<self_t> > base_t;

    // not explicit! (needed for implementation of if_p et al.)
    condition_parser(CondT const &cond) : base_t(cond) {}
    condition_parser() : base_t() {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        if (positive == this->get()())
            return scan.empty_match();
        else
            return scan.no_match();
    }

    condition_parser<CondT, !positive>
    negate() const
    {
        return condition_parser<CondT, !positive>(this->get());
    }

private:
};

template <typename CondT, bool positive>
inline condition_parser<CondT, !positive>
operator~(condition_parser<CondT, positive> const &p)
{
    return p.negate();
}

///////////////////////////////////////////////////////////////////////////////
//
//  empty_match_parser class
//
//      handles expressions of the form
//          epsilon_p(subject)
//      where subject is a parser. The expresion returns a composite
//      parser that returns an empty match if the subject parser matches.
//
///////////////////////////////////////////////////////////////////////////////
struct empty_match_parser_gen;
struct negated_empty_match_parser_gen;

template<typename SubjectT> struct negated_empty_match_parser;

template<typename SubjectT>
struct empty_match_parser
    : public unary<SubjectT, parser<empty_match_parser<SubjectT> > >
{
    typedef empty_match_parser<SubjectT>        self_t;
    typedef unary<SubjectT, parser<self_t> >    base_t;
    typedef unary_parser_category               parser_category_t;
    typedef empty_match_parser_gen              parser_genererator_t;
    typedef self_t embed_t;

    explicit empty_match_parser(SubjectT const &p) : base_t(p) {}
    empty_match_parser() : base_t() {}

    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, nil_t>::type type;
    };

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typename ScannerT::iterator_t save(scan.first);
        bool matches = this->subject().parse(scan);
        if (matches)
        {
            scan.first = save; // reset the position
            return scan.empty_match();
        }
        else
            return scan.no_match();
    }

    negated_empty_match_parser<SubjectT>
    negate() const
    {
        return negated_empty_match_parser<SubjectT>(this->subject());
    }
};

template<typename SubjectT>
struct negated_empty_match_parser
    : public unary<SubjectT, parser<negated_empty_match_parser<SubjectT> > >
{
    typedef negated_empty_match_parser<SubjectT>    self_t;
    typedef unary<SubjectT, parser<self_t> >        base_t;
    typedef unary_parser_category                   parser_category_t;
    typedef negated_empty_match_parser_gen          parser_genererator_t;

    explicit negated_empty_match_parser(SubjectT const &p) : base_t(p) {}
    negated_empty_match_parser() : base_t() {}

    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, nil_t>::type type;
    };

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typename ScannerT::iterator_t save(scan.first);

        bool matches = this->subject().parse(scan);
        if (!matches)
        {
            scan.first = save; // reset the position
            return scan.empty_match();
        }
        else
            return scan.no_match();
    }

    empty_match_parser<SubjectT>
    negate() const
    {
        return empty_match_parser<SubjectT>(this->subject());
    }
};

//////////////////////////////
struct empty_match_parser_gen
{
    template <typename SubjectT>
    struct result
    {
        typedef empty_match_parser<SubjectT> type;
    };

    template <typename SubjectT>
    static empty_match_parser<SubjectT>
    generate(parser<SubjectT> const &subject)
    {
        return empty_match_parser<SubjectT>(subject.derived());
    }
};

struct negated_empty_match_parser_gen
{
    template <typename SubjectT>
    struct result
    {
        typedef negated_empty_match_parser<SubjectT> type;
    };

    template <typename SubjectT>
    static negated_empty_match_parser<SubjectT>
    generate(parser<SubjectT> const &subject)
    {
        return negated_empty_match_parser<SubjectT>(subject.derived());
    }
};

//////////////////////////////
template <typename SubjectT>
inline /*struct*/ negated_empty_match_parser<SubjectT>
operator ~(empty_match_parser<SubjectT> const &p)
{
    return p.negate();
}

template <typename SubjectT>
inline /*struct*/ empty_match_parser<SubjectT>
operator ~(negated_empty_match_parser<SubjectT> const &p)
{
    return p.negate();
}

///////////////////////////////////////////////////////////////////////////////
//
//  epsilon_ parser and parser generator class
//
//      Operates as primitive parser that always matches an empty sequence.
//
//      Also operates as a parser generator. According to the type of the
//      argument an instance of empty_match_parser<> (when the argument is
//      a parser) or condition_parser<> (when the argument is not a parser)
//      is returned by operator().
//
///////////////////////////////////////////////////////////////////////////////
namespace impl
{
    template <typename SubjectT>
    struct epsilon_selector
    {
        typedef typename as_parser<SubjectT>::type subject_t;
        typedef typename
            mpl::if_<
                is_parser<subject_t>
                ,empty_match_parser<subject_t>
                ,condition_parser<subject_t>
                >::type type;
    };
} // namespace impl

struct epsilon_parser : public parser<epsilon_parser>
{
    typedef epsilon_parser self_t;

    epsilon_parser() {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    { return scan.empty_match(); }

    template <typename SubjectT>
    typename impl::epsilon_selector<SubjectT>::type
    operator()(SubjectT const &subject) const
    {
        typedef typename impl::epsilon_selector<SubjectT>::type result_t;
        return result_t(subject);
    }
};

//////////////////////////////////
epsilon_parser const epsilon_p = epsilon_parser();
epsilon_parser const eps_p = epsilon_parser();

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif
