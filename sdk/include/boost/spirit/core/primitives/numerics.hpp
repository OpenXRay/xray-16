/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    Copyright (c) 2001-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#ifndef BOOST_SPIRIT_NUMERICS_HPP
#define BOOST_SPIRIT_NUMERICS_HPP

///////////////////////////////////////////////////////////////////////////////
#include "boost/config.hpp"

#include "boost/spirit/core/parser.hpp"
#include "boost/spirit/core/composite/directives.hpp"
#include "boost/spirit/core/primitives/impl/numerics.ipp"

//  VC++6 chokes and ICEs with parser_result on the real parser traits when
//  the real parser is used inside a grammar. This workaround solves the
//  problem
#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1200)
#define BOOST_SPIRIT_NUMP_RESULT(parser_t, scanner_t, T) \
    typename match_result<scanner_t, T>::type
#else
#define BOOST_SPIRIT_NUMP_RESULT(parser_t, scanner_t, T) \
    typename parser_result<parser_t, scanner_t>::type
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
//
//  uint_parser class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename T = unsigned,
    int Radix = 10,
    unsigned MinDigits = 1,
    int MaxDigits = -1
>
struct uint_parser
:   public parser<uint_parser<T, Radix, MinDigits, MaxDigits> >
{
    typedef uint_parser<T, Radix, MinDigits, MaxDigits> self_t;

    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, T>::type type;
    };

// Don't take the #if out. VC6 will hate it!
#if defined(__MWERKS__) && (__MWERKS__ <= 0x2407)
// Don't take the empty constructor out line CW7.2 will hate it!
    uint_parser() {}
#endif

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef impl::uint_parser_impl<T, Radix, MinDigits, MaxDigits> impl_t;
        typedef typename parser_result<impl_t, ScannerT>::type result_t;
        return impl::contiguous_parser_parse<result_t>(impl_t(), scan, scan);
    }
};

///////////////////////////////////////////////////////////////////////////////
//
//  int_parser class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename T = unsigned,
    int Radix = 10,
    unsigned MinDigits = 1,
    int MaxDigits = -1
>
struct int_parser
:   public parser<int_parser<T, Radix, MinDigits, MaxDigits> >
{
    typedef int_parser<T, Radix, MinDigits, MaxDigits> self_t;

    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, T>::type type;
    };

// Don't take the #if out. VC6 will hate it!
#if defined(__MWERKS__) && (__MWERKS__ <= 0x2407)
// Don't take the empty constructor out line CW7.2 will hate it!
    int_parser() {}
#endif

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef impl::int_parser_impl<T, Radix, MinDigits, MaxDigits> impl_t;
        typedef typename parser_result<impl_t, ScannerT>::type result_t;
        return impl::contiguous_parser_parse<result_t>(impl_t(), scan, scan);
    }
};

///////////////////////////////////////////////////////////////////////////////
//
//  uint_parser/int_parser instantiations
//
///////////////////////////////////////////////////////////////////////////////
int_parser<int, 10, 1, -1> const
    int_p   = int_parser<int, 10, 1, -1>();

uint_parser<unsigned, 10, 1, -1> const
    uint_p  = uint_parser<unsigned, 10, 1, -1>();

uint_parser<unsigned, 2, 1, -1> const
    bin_p   = uint_parser<unsigned, 2, 1, -1>();

uint_parser<unsigned, 8, 1, -1> const
    oct_p   = uint_parser<unsigned, 8, 1, -1>();

uint_parser<unsigned, 16, 1, -1> const
    hex_p   = uint_parser<unsigned, 16, 1, -1>();

//  Borland 5.5 again gets confused if the default template parameters
//  are not spelled out explicitly above. Sigh, Borland...

///////////////////////////////////////////////////////////////////////////////
//
//  sign_parser class
//
///////////////////////////////////////////////////////////////////////////////
    namespace impl
    {
        //  Utility to extract the prefix sign ('-' | '+')
        template <typename ScannerT>
        bool extract_sign(ScannerT const& scan, unsigned& count);
    }

struct sign_parser : public parser<sign_parser>
{
    typedef sign_parser self_t;
    template <typename ScannerT>
    struct result {

        typedef typename match_result<ScannerT, bool>::type type;
    };

    sign_parser() {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        if (!scan.at_end())
        {
            unsigned length;
            typename ScannerT::iterator_t save(scan.first);
            bool neg = impl::extract_sign(scan, length);
            if (length)
                return scan.create_match(1, neg, save, scan.first);
        }
        return scan.no_match();
    }
};

sign_parser const sign_p = sign_parser();

///////////////////////////////////////////////////////////////////////////////
//
//  default real number policies
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct ureal_parser_policies
{
    // trailing dot policy suggested suggested by Gustavo Guerra
    BOOST_STATIC_CONSTANT(bool, allow_leading_dot = true);
    BOOST_STATIC_CONSTANT(bool, allow_trailing_dot = true);
    BOOST_STATIC_CONSTANT(bool, expect_dot = false);

    typedef uint_parser<T, 10, 1, -1>   uint_parser_t;
    typedef int_parser<T, 10, 1, -1>    int_parser_t;

    template <typename ScannerT>
    static typename match_result<ScannerT, nil_t>::type
    parse_sign(ScannerT& scan)
    { return scan.no_match(); }

    template <typename ScannerT>
    static BOOST_SPIRIT_NUMP_RESULT(uint_parser_t, ScannerT, T)
    parse_n(ScannerT& scan)
    { return uint_parser_t().parse(scan); }

    template <typename ScannerT>
    static typename parser_result<chlit<>, ScannerT>::type
    parse_dot(ScannerT& scan)
    { return ch_p('.').parse(scan); }

    template <typename ScannerT>
    static BOOST_SPIRIT_NUMP_RESULT(uint_parser_t, ScannerT, T)
    parse_frac_n(ScannerT& scan)
    { return uint_parser_t().parse(scan); }

    template <typename ScannerT>
    static typename parser_result<chlit<>, ScannerT>::type
    parse_exp(ScannerT& scan)
    { return as_lower_d['e'].parse(scan); }

    template <typename ScannerT>
    static BOOST_SPIRIT_NUMP_RESULT(int_parser_t, ScannerT, T)
    parse_exp_n(ScannerT& scan)
    { return int_parser_t().parse(scan); }
};

template <typename T>
struct real_parser_policies : public ureal_parser_policies<T>
{
    template <typename ScannerT>
    static typename parser_result<sign_parser, ScannerT>::type
    parse_sign(ScannerT& scan)
    { return sign_p.parse(scan); }
};

///////////////////////////////////////////////////////////////////////////////
//
//  real_parser class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename T = double,
    typename RealPoliciesT = ureal_parser_policies<T>
>
struct real_parser
:   public parser<real_parser<T, RealPoliciesT> >
{
    typedef real_parser<T, RealPoliciesT> self_t;

    template <typename ScannerT>
    struct result
    {
        typedef typename match_result<ScannerT, T>::type type;
    };

    real_parser() {}

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        typedef typename parser_result<self_t, ScannerT>::type result_t;
        return impl::real_parser_impl<result_t, T, RealPoliciesT>::parse(scan);
    }
};

///////////////////////////////////////////////////////////////////////////////
//
//  real_parser instantiations
//
///////////////////////////////////////////////////////////////////////////////
real_parser<double, ureal_parser_policies<double> > const
    ureal_p     = real_parser<double, ureal_parser_policies<double> >();

real_parser<double, real_parser_policies<double> > const
    real_p      = real_parser<double, real_parser_policies<double> >();

///////////////////////////////////////////////////////////////////////////////
//
//  strict reals (do not allow plain integers (no decimal point))
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct strict_ureal_parser_policies : public ureal_parser_policies<T>
{
    BOOST_STATIC_CONSTANT(bool, expect_dot = true);
};

template <typename T>
struct strict_real_parser_policies : public real_parser_policies<T>
{
    BOOST_STATIC_CONSTANT(bool, expect_dot = true);
};

real_parser<double, strict_ureal_parser_policies<double> > const
    strict_ureal_p
        = real_parser<double, strict_ureal_parser_policies<double> >();

real_parser<double, strict_real_parser_policies<double> > const
    strict_real_p
        = real_parser<double, strict_real_parser_policies<double> >();

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#undef BOOST_SPIRIT_NUMP_RESULT
#endif
