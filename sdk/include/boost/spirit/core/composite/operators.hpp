/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001 Daniel Nuffer
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_OPERATORS_HPP)
#define BOOST_SPIRIT_OPERATORS_HPP

///////////////////////////////////////////////////////////////////////////////
#include <algorithm>

#include "boost/spirit/core/parser.hpp"
#include "boost/spirit/core/primitives/primitives.hpp"
#include "boost/spirit/core/composite/composite.hpp"
#include "boost/spirit/core/meta/impl/parser_type.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
//
//  sequence class
//
//      Handles expressions of the form:
//
//          a >> b
//
//      where a and b are parsers. The expression returns a composite
//      parser that matches a and b in sequence. One (not both) of the
//      operands may be a literal char, wchar_t or a primitive string
//      char const*, wchar_t const*.
//
///////////////////////////////////////////////////////////////////////////////
struct sequence_parser_gen;

template <typename A, typename B>
struct sequence : public binary<A, B, parser<sequence<A, B> > >
{
    typedef sequence<A, B>                  self_t;
    typedef binary_parser_category          parser_category_t;
    typedef sequence_parser_gen             parser_generator_t;
    typedef binary<A, B, parser<self_t> >   base_t;

    sequence()
    : base_t(A(), B()) {}
    sequence(A const& a, B const& b)
    : base_t(a, b) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        if (result_t ma = this->left().parse(scan))
            if (result_t mb = this->right().parse(scan))
            {
                scan.concat_match(ma, mb);
                return ma;
            }
        return scan.no_match();
    }
};

//////////////////////////////////
struct sequence_parser_gen
{
    template <typename A, typename B>
    struct result {

        typedef
            sequence<typename as_parser<A>::type, typename as_parser<B>::type>
            type;
    };

    template <typename A, typename B>
    static sequence<typename as_parser<A>::type, typename as_parser<B>::type>
    generate(A const& a, B const& b)
    {
        return sequence<BOOST_SPIRIT_TYPENAME as_parser<A>::type,
            BOOST_SPIRIT_TYPENAME as_parser<B>::type>
                (as_parser<A>::convert(a), as_parser<B>::convert(b));
    }
};

//////////////////////////////////
template <typename A, typename B>
sequence<A, B>
operator>>(parser<A> const& a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequence<A, chlit<char> >
operator>>(parser<A> const& a, char b);

//////////////////////////////////
template <typename B>
sequence<chlit<char>, B>
operator>>(char a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequence<A, strlit<char const*> >
operator>>(parser<A> const& a, char const* b);

//////////////////////////////////
template <typename B>
sequence<strlit<char const*>, B>
operator>>(char const* a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequence<A, chlit<wchar_t> >
operator>>(parser<A> const& a, wchar_t b);

//////////////////////////////////
template <typename B>
sequence<chlit<wchar_t>, B>
operator>>(wchar_t a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequence<A, strlit<wchar_t const*> >
operator>>(parser<A> const& a, wchar_t const* b);

//////////////////////////////////
template <typename B>
sequence<strlit<wchar_t const*>, B>
operator>>(wchar_t const* a, parser<B> const& b);

///////////////////////////////////////////////////////////////////////////////
//
//  sequential-and operators
//
//      Handles expressions of the form:
//
//          a && b
//
//      Same as a >> b.
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B>
sequence<A, B>
operator&&(parser<A> const& a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequence<A, chlit<char> >
operator&&(parser<A> const& a, char b);

//////////////////////////////////
template <typename B>
sequence<chlit<char>, B>
operator&&(char a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequence<A, strlit<char const*> >
operator&&(parser<A> const& a, char const* b);

//////////////////////////////////
template <typename B>
sequence<strlit<char const*>, B>
operator&&(char const* a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequence<A, chlit<wchar_t> >
operator&&(parser<A> const& a, wchar_t b);

//////////////////////////////////
template <typename B>
sequence<chlit<wchar_t>, B>
operator&&(wchar_t a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequence<A, strlit<wchar_t const*> >
operator&&(parser<A> const& a, wchar_t const* b);

//////////////////////////////////
template <typename B>
sequence<strlit<wchar_t const*>, B>
operator&&(wchar_t const* a, parser<B> const& b);

///////////////////////////////////////////////////////////////////////////////
//
//  sequential-or class
//
//      Handles expressions of the form:
//
//          a || b
//
//      Equivalent to
//
//          a | b | a >> b;
//
//      where a and b are parsers. The expression returns a composite
//      parser that matches matches a or b in sequence. One (not both) of
//      the operands may be a literal char, wchar_t or a primitive string
//      char const*, wchar_t const*.
//
///////////////////////////////////////////////////////////////////////////////
struct sequential_or_parser_gen;

template <typename A, typename B>
struct sequential_or : public binary<A, B, parser<sequential_or<A, B> > >
{
    typedef sequential_or<A, B>             self_t;
    typedef binary_parser_category          parser_category_t;
    typedef sequential_or_parser_gen        parser_generator_t;
    typedef binary<A, B, parser<self_t> >   base_t;

    sequential_or()
    : base_t(A(), B()) {}
    sequential_or(A const& a, B const& b)
    : base_t(a, b) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        typedef typename ScannerT::iterator_t iterator_t;
        { // scope for save
            iterator_t save = scan.first;
            if (result_t ma = this->left().parse(scan))
            {
                save = scan.first;
                if (result_t mb = this->right().parse(scan))
                {
                    // matched a b
                    scan.concat_match(ma, mb);
                    return ma;
                }
                else
                {
                    // matched a
                    scan.first = save;
                    return ma;
                }
            }
            scan.first = save;
        }

        // matched b
        return this->right().parse(scan);
    }
};

//////////////////////////////////
struct sequential_or_parser_gen
{
    template <typename A, typename B>
    struct result {

        typedef sequential_or<
                    typename as_parser<A>::type, typename as_parser<B>::type
                > type;
    };

    template <typename A, typename B>
    static sequential_or
    <
        typename as_parser<A>::type,
        typename as_parser<B>::type
    >
    generate(A const& a, B const& b)
    {
        return sequential_or<BOOST_SPIRIT_TYPENAME as_parser<A>::type,
            BOOST_SPIRIT_TYPENAME as_parser<B>::type>
                (as_parser<A>::convert(a), as_parser<B>::convert(b));
    }
};

//////////////////////////////////
template <typename A, typename B>
sequential_or<A, B>
operator||(parser<A> const& a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequential_or<A, chlit<char> >
operator||(parser<A> const& a, char b);

//////////////////////////////////
template <typename B>
sequential_or<chlit<char>, B>
operator||(char a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequential_or<A, strlit<char const*> >
operator||(parser<A> const& a, char const* b);

//////////////////////////////////
template <typename B>
sequential_or<strlit<char const*>, B>
operator||(char const* a, parser<B> const& b);
//////////////////////////////////
template <typename A>
sequential_or<A, chlit<wchar_t> >
operator||(parser<A> const& a, wchar_t b);

//////////////////////////////////
template <typename B>
sequential_or<chlit<wchar_t>, B>
operator||(wchar_t a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequential_or<A, strlit<wchar_t const*> >
operator||(parser<A> const& a, wchar_t const* b);

//////////////////////////////////
template <typename B>
sequential_or<strlit<wchar_t const*>, B>
operator||(wchar_t const* a, parser<B> const& b);

///////////////////////////////////////////////////////////////////////////////
//
//  alternative class
//
//      Handles expressions of the form:
//
//          a | b
//
//      where a and b are parsers. The expression returns a composite
//      parser that matches a or b. One (not both) of the operands may
//      be a literal char, wchar_t or a primitive string char const*,
//      wchar_t const*.
//
//      The expression is short circuit evaluated. b is never touched
//      when a is returns a successful match.
//
///////////////////////////////////////////////////////////////////////////////
struct alternative_parser_gen;

template <typename A, typename B>
struct alternative
:   public binary<A, B, parser<alternative<A, B> > >
{
    typedef alternative<A, B>               self_t;
    typedef binary_parser_category          parser_category_t;
    typedef alternative_parser_gen          parser_generator_t;
    typedef binary<A, B, parser<self_t> >   base_t;

    alternative()
    : base_t(A(), B()) {}
    alternative(A const& a, B const& b)
    : base_t(a, b) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        typedef typename ScannerT::iterator_t iterator_t;
        { // scope for save
            iterator_t save = scan.first;
            if (result_t hit = this->left().parse(scan))
                return hit;
            scan.first = save;
        }
        return this->right().parse(scan);
    }
};

//////////////////////////////////
struct alternative_parser_gen
{
    template <typename A, typename B>
    struct result {

        typedef alternative<
                    typename as_parser<A>::type, typename as_parser<B>::type
                > type;
    };

    template <typename A, typename B>
    static alternative
    <
        typename as_parser<A>::type,
        typename as_parser<B>::type
    >
    generate(A const& a, B const& b)
    {
        return alternative<BOOST_SPIRIT_TYPENAME as_parser<A>::type,
            BOOST_SPIRIT_TYPENAME as_parser<B>::type>
                (as_parser<A>::convert(a), as_parser<B>::convert(b));
    }
};

//////////////////////////////////
template <typename A, typename B>
alternative<A, B>
operator|(parser<A> const& a, parser<B> const& b);

//////////////////////////////////
template <typename A>
alternative<A, chlit<char> >
operator|(parser<A> const& a, char b);

//////////////////////////////////
template <typename B>
alternative<chlit<char>, B>
operator|(char a, parser<B> const& b);

//////////////////////////////////
template <typename A>
alternative<A, strlit<char const*> >
operator|(parser<A> const& a, char const* b);

//////////////////////////////////
template <typename B>
alternative<strlit<char const*>, B>
operator|(char const* a, parser<B> const& b);

//////////////////////////////////
template <typename A>
alternative<A, chlit<wchar_t> >
operator|(parser<A> const& a, wchar_t b);

//////////////////////////////////
template <typename B>
alternative<chlit<wchar_t>, B>
operator|(wchar_t a, parser<B> const& b);

//////////////////////////////////
template <typename A>
alternative<A, strlit<wchar_t const*> >
operator|(parser<A> const& a, wchar_t const* b);

//////////////////////////////////
template <typename B>
alternative<strlit<wchar_t const*>, B>
operator|(wchar_t const* a, parser<B> const& b);

///////////////////////////////////////////////////////////////////////////////
//
//  intersection class
//
//      Handles expressions of the form:
//
//          a & b
//
//      where a and b are parsers. The expression returns a composite
//      parser that matches a and b. One (not both) of the operands may
//      be a literal char, wchar_t or a primitive string char const*,
//      wchar_t const*.
//
//      The expression is short circuit evaluated. b is never touched
//      when a is returns a no-match.
//
///////////////////////////////////////////////////////////////////////////////
struct intersection_parser_gen;

template <typename A, typename B>
struct intersection
:   public binary<A, B, parser<intersection<A, B> > >
{
    typedef intersection<A, B>              self_t;
    typedef binary_parser_category          parser_category_t;
    typedef intersection_parser_gen         parser_generator_t;
    typedef binary<A, B, parser<self_t> >   base_t;

    intersection()
    : base_t(A(), B()) {}
    intersection(A const& a, B const& b)
    : base_t(a, b) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        typedef typename ScannerT::iterator_t iterator_t;
        iterator_t save = scan.first;
        if (result_t hl = this->left().parse(scan))
        {
            ScannerT bscan(scan.first, scan.first);
            scan.first = save;
            result_t hr = this->right().parse(bscan);
            if (hl.length() == hr.length())
                return hl;
        }

        return scan.no_match();
    }
};

//////////////////////////////////
struct intersection_parser_gen
{
    template <typename A, typename B>
    struct result {

        typedef intersection<
                    typename as_parser<A>::type, typename as_parser<B>::type
                > type;
    };

    template <typename A, typename B>
    static intersection
    <
        typename as_parser<A>::type,
        typename as_parser<B>::type
    >
    generate(A const& a, B const& b)
    {
        return intersection<BOOST_SPIRIT_TYPENAME as_parser<A>::type,
            BOOST_SPIRIT_TYPENAME as_parser<B>::type>
                (as_parser<A>::convert(a), as_parser<B>::convert(b));
    }
};

//////////////////////////////////
template <typename A, typename B>
intersection<A, B>
operator&(parser<A> const& a, parser<B> const& b);

//////////////////////////////////
template <typename A>
intersection<A, chlit<char> >
operator&(parser<A> const& a, char b);

//////////////////////////////////
template <typename B>
intersection<chlit<char>, B>
operator&(char a, parser<B> const& b);

//////////////////////////////////
template <typename A>
intersection<A, strlit<char const*> >
operator&(parser<A> const& a, char const* b);

//////////////////////////////////
template <typename B>
intersection<strlit<char const*>, B>
operator&(char const* a, parser<B> const& b);

//////////////////////////////////
template <typename A>
intersection<A, chlit<wchar_t> >
operator&(parser<A> const& a, wchar_t b);

//////////////////////////////////
template <typename B>
intersection<chlit<wchar_t>, B>
operator&(wchar_t a, parser<B> const& b);

//////////////////////////////////
template <typename A>
intersection<A, strlit<wchar_t const*> >
operator&(parser<A> const& a, wchar_t const* b);

//////////////////////////////////
template <typename B>
intersection<strlit<wchar_t const*>, B>
operator&(wchar_t const* a, parser<B> const& b);

///////////////////////////////////////////////////////////////////////////////
//
//  difference: a - b; Matches a but not b
//
//      Handles expressions of the form:
//
//          a - b
//
//      where a and b are parsers. The expression returns a composite
//      parser that matches a but not b. One (not both) of the operands
//      may be a literal char, wchar_t or a primitive string char const*,
//      wchar_t const*.
//
///////////////////////////////////////////////////////////////////////////////
struct difference_parser_gen;

template <typename A, typename B>
struct difference
:   public binary<A, B, parser<difference<A, B> > >
{
    typedef difference<A, B>                self_t;
    typedef binary_parser_category          parser_category_t;
    typedef difference_parser_gen           parser_generator_t;
    typedef binary<A, B, parser<self_t> >   base_t;

    difference()
    : base_t(A(), B()) {}
    difference(A const& a, B const& b)
    : base_t(a, b) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        typedef typename ScannerT::iterator_t iterator_t;
        iterator_t save = scan.first;
        if (result_t hl = this->left().parse(scan))
        {
            std::swap(save, scan.first);
            result_t hr = this->right().parse(scan);
            if (!hr || (hr.length() < hl.length()))
            {
                scan.first = save;
                return hl;
            }
        }

        return scan.no_match();
    }
};

//////////////////////////////////
struct difference_parser_gen
{
    template <typename A, typename B>
    struct result {

        typedef difference<
                    typename as_parser<A>::type, typename as_parser<B>::type
                > type;
    };

    template <typename A, typename B>
    static difference
    <
        typename as_parser<A>::type,
        typename as_parser<B>::type
    >
    generate(A const& a, B const& b)
    {
        return difference<BOOST_SPIRIT_TYPENAME as_parser<A>::type,
            BOOST_SPIRIT_TYPENAME as_parser<B>::type>
                (as_parser<A>::convert(a), as_parser<B>::convert(b));
    }
};

//////////////////////////////////
template <typename A, typename B>
difference<A, B>
operator-(parser<A> const& a, parser<B> const& b);

//////////////////////////////////
template <typename A>
difference<A, chlit<char> >
operator-(parser<A> const& a, char b);

//////////////////////////////////
template <typename B>
difference<chlit<char>, B>
operator-(char a, parser<B> const& b);

//////////////////////////////////
template <typename A>
difference<A, strlit<char const*> >
operator-(parser<A> const& a, char const* b);

//////////////////////////////////
template <typename B>
difference<strlit<char const*>, B>
operator-(char const* a, parser<B> const& b);

//////////////////////////////////
template <typename A>
difference<A, chlit<wchar_t> >
operator-(parser<A> const& a, wchar_t b);

//////////////////////////////////
template <typename B>
difference<chlit<wchar_t>, B>
operator-(wchar_t a, parser<B> const& b);

//////////////////////////////////
template <typename A>
difference<A, strlit<wchar_t const*> >
operator-(parser<A> const& a, wchar_t const* b);

//////////////////////////////////
template <typename B>
difference<strlit<wchar_t const*>, B>
operator-(wchar_t const* a, parser<B> const& b);

///////////////////////////////////////////////////////////////////////////////
//
//  exclusive_or class
//
//      Handles expressions of the form:
//
//          a ^ b
//
//      where a and b are parsers. The expression returns a composite
//      parser that matches a or b but not both. One (not both) of the
//      operands may be a literal char, wchar_t or a primitive string
//      char const*, wchar_t const*.
//
///////////////////////////////////////////////////////////////////////////////
struct exclusive_or_parser_gen;

template <typename A, typename B>
struct exclusive_or
:   public binary<A, B, parser<exclusive_or<A, B> > >
{
    typedef exclusive_or<A, B>              self_t;
    typedef binary_parser_category          parser_category_t;
    typedef exclusive_or_parser_gen         parser_generator_t;
    typedef binary<A, B, parser<self_t> >   base_t;

    exclusive_or()
    : base_t(A(), B()) {}
    exclusive_or(A const& a, B const& b)
    : base_t(a, b) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        typedef typename ScannerT::iterator_t iterator_t;

        iterator_t save = scan.first;
        result_t l = this->left().parse(scan);
        std::swap(save, scan.first);
        result_t r = this->right().parse(scan);

        if (bool(l) ^ bool(r))
        {
            if (l)
                scan.first = save;
            return bool(l) ? l : r;
        }

        return scan.no_match();
    }
};

//////////////////////////////////
struct exclusive_or_parser_gen
{
    template <typename A, typename B>
    struct result {

        typedef exclusive_or<
                    typename as_parser<A>::type, typename as_parser<B>::type
                > type;
    };

    template <typename A, typename B>
    static exclusive_or
    <
        typename as_parser<A>::type,
        typename as_parser<B>::type
    >
    generate(A const& a, B const& b)
    {
        return exclusive_or<BOOST_SPIRIT_TYPENAME as_parser<A>::type,
            BOOST_SPIRIT_TYPENAME as_parser<B>::type>
                (as_parser<A>::convert(a), as_parser<B>::convert(b));
    }
};

//////////////////////////////////
template <typename A, typename B>
exclusive_or<A, B>
operator^(parser<A> const& a, parser<B> const& b);

//////////////////////////////////
template <typename A>
exclusive_or<A, chlit<char> >
operator^(parser<A> const& a, char b);

//////////////////////////////////
template <typename B>
exclusive_or<chlit<char>, B>
operator^(char a, parser<B> const& b);

//////////////////////////////////
template <typename A>
exclusive_or<A, strlit<char const*> >
operator^(parser<A> const& a, char const* b);

//////////////////////////////////
template <typename B>
exclusive_or<strlit<char const*>, B>
operator^(char const* a, parser<B> const& b);

//////////////////////////////////
template <typename A>
exclusive_or<A, chlit<wchar_t> >
operator^(parser<A> const& a, wchar_t b);

//////////////////////////////////
template <typename B>
exclusive_or<chlit<wchar_t>, B>
operator^(wchar_t a, parser<B> const& b);

//////////////////////////////////
template <typename A>
exclusive_or<A, strlit<wchar_t const*> >
operator^(parser<A> const& a, wchar_t const* b);

//////////////////////////////////
template <typename B>
exclusive_or<strlit<wchar_t const*>, B>
operator^(wchar_t const* a, parser<B> const& b);

///////////////////////////////////////////////////////////////////////////////
//
//  optional class
//
//      Handles expressions of the form:
//
//          !a
//
//      where a is a parser. The expression returns a composite
//      parser that matches its subject zero (0) or one (1) time.
//
///////////////////////////////////////////////////////////////////////////////
struct optional_parser_gen;

template <typename S>
struct optional
:   public unary<S, parser<optional<S> > >
{
    typedef optional<S>                 self_t;
    typedef unary_parser_category       parser_category_t;
    typedef optional_parser_gen         parser_generator_t;
    typedef unary<S, parser<self_t> >   base_t;

    optional()
    : base_t(S()) {}
    optional(S const& a)
    : base_t(a) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        typedef typename ScannerT::iterator_t iterator_t;
        iterator_t save = scan.first;
        if (result_t r = this->subject().parse(scan))
        {
            return r;
        }
        else
        {
            scan.first = save;
            return scan.empty_match();
        }
    }
};

//////////////////////////////////
struct optional_parser_gen
{
    template <typename S>
    struct result {

        typedef optional<S> type;
    };

    template <typename S>
    static optional<S>
    generate(parser<S> const& a)
    {
        return optional<S>(a.derived());
    }
};

//////////////////////////////////
template <typename S>
optional<S>
operator!(parser<S> const& a);

///////////////////////////////////////////////////////////////////////////////
//
//  kleene_star class
//
//      Handles expressions of the form:
//
//          *a
//
//      where a is a parser. The expression returns a composite
//      parser that matches its subject zero (0) or more times.
//
///////////////////////////////////////////////////////////////////////////////
struct kleene_star_parser_gen;

template <typename S>
struct kleene_star
:   public unary<S, parser<kleene_star<S> > >
{
    typedef kleene_star<S>              self_t;
    typedef unary_parser_category       parser_category_t;
    typedef kleene_star_parser_gen      parser_generator_t;
    typedef unary<S, parser<self_t> >   base_t;

    kleene_star()
    : base_t(S()) {}
    kleene_star(S const& a)
    : base_t(a) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        typedef typename ScannerT::iterator_t iterator_t;
        result_t hit = scan.empty_match();

        for (;;)
        {
            iterator_t save = scan.first;
            if (result_t next = this->subject().parse(scan))
            {
                scan.concat_match(hit, next);
            }
            else
            {
                scan.first = save;
                return hit;
            }
        }
    }
};

//////////////////////////////////
struct kleene_star_parser_gen
{
    template <typename S>
    struct result {

        typedef kleene_star<S> type;
    };

    template <typename S>
    static kleene_star<S>
    generate(parser<S> const& a)
    {
        return kleene_star<S>(a.derived());
    }
};

//////////////////////////////////
template <typename S>
kleene_star<S>
operator*(parser<S> const& a);

///////////////////////////////////////////////////////////////////////////////
//
//  positive class
//
//      Handles expressions of the form:
//
//          +a
//
//      where a is a parser. The expression returns a composite
//      parser that matches its subject one (1) or more times.
//
///////////////////////////////////////////////////////////////////////////////
struct positive_parser_gen;

template <typename S>
struct positive
:   public unary<S, parser<positive<S> > >
{
    typedef positive<S>                 self_t;
    typedef unary_parser_category       parser_category_t;
    typedef positive_parser_gen         parser_generator_t;
    typedef unary<S, parser<self_t> >   base_t;

    positive()
    : base_t(S()) {}
    positive(S const& a)
    : base_t(a) {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        typedef typename ScannerT::iterator_t iterator_t;
        result_t hit = this->subject().parse(scan);

        if (hit)
        {
            for (;;)
            {
                iterator_t save = scan.first;
                if (result_t next = this->subject().parse(scan))
                {
                    scan.concat_match(hit, next);
                }
                else
                {
                    scan.first = save;
                    break;
                }
            }
        }
        return hit;
    }
};

//////////////////////////////////
struct positive_parser_gen
{
    template <typename S>
    struct result {

        typedef positive<S> type;
    };

    template <typename S>
    static positive<S>
    generate(parser<S> const& a)
    {
        return positive<S>(a.derived());
    }
};

//////////////////////////////////
template <typename S>
inline positive<S>
operator + (parser<S> const& a);

///////////////////////////////////////////////////////////////////////////////
//
//  operator% is defined as:
//  a % b ---> a >> *(b >> a)
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B>
sequence<A, kleene_star<sequence<B, A> > >
operator%(parser<A> const& a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequence<A, kleene_star<sequence<chlit<char>, A> > >
operator%(parser<A> const& a, char b);

//////////////////////////////////
template <typename B>
sequence<chlit<char>, kleene_star<sequence<B, chlit<char> > > >
operator%(char a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequence<A, kleene_star<sequence<strlit<char const*>, A> > >
operator%(parser<A> const& a, char const* b);

//////////////////////////////////
template <typename B>
sequence<strlit<char const*>,
    kleene_star<sequence<B, strlit<char const*> > > >
operator%(char const* a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequence<A, kleene_star<sequence<chlit<wchar_t>, A> > >
operator%(parser<A> const& a, wchar_t b);

//////////////////////////////////
template <typename B>
sequence<chlit<wchar_t>, kleene_star<sequence<B, chlit<wchar_t> > > >
operator%(wchar_t a, parser<B> const& b);

//////////////////////////////////
template <typename A>
sequence<A, kleene_star<sequence<strlit<wchar_t const*>, A> > >
operator%(parser<A> const& a, wchar_t const* b);

//////////////////////////////////
template <typename B>
sequence<strlit<wchar_t const*>,
    kleene_star<sequence<B, strlit<wchar_t const*> > > >
operator%(wchar_t const* a, parser<B> const& b);

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif

#include "boost/spirit/core/composite/impl/operators.ipp"
