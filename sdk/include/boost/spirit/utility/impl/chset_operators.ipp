/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#ifndef BOOST_SPIRIT_CHSET_OPERATORS_IPP
#define BOOST_SPIRIT_CHSET_OPERATORS_IPP

///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_LIMITS_HPP)
#include "boost/limits.hpp"
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
//
//  chset free operators implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) |= b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) -= b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator~(chset<CharT> const& a)
{
    return chset<CharT>(a).inverse();
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) &= b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) ^= b;
}

///////////////////////////////////////////////////////////////////////////////
//
//  range <--> chset free operators implementation
//
///////////////////////////////////////////////////////////////////////////////
namespace impl {

    //////////////////////////////////
    template <typename CharT>
    inline CharT
    decr(CharT v)
    {
        return v == std::numeric_limits<CharT>::min() ? v : v-1;
    }

    //////////////////////////////////
    template <typename CharT>
    inline CharT
    incr(CharT v)
    {
        return v == std::numeric_limits<CharT>::max() ? v : v+1;
    }
}

template <typename CharT>
inline chset<CharT>
operator~(range<CharT> const& a)
{
    chset<CharT> a_;
    a_.set(range<CharT>(std::numeric_limits<CharT>::min(),
                impl::decr(a.first)));
    a_.set(range<CharT>(impl::incr(a.last),
                std::numeric_limits<CharT>::max()));
    return a_;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const& a, range<CharT> const& b)
{
    chset<CharT> a_(a);
    a_.set(b);
    return a_;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& a, range<CharT> const& b)
{
    chset<CharT> a_(a);
    a_.clear(range<CharT>(std::numeric_limits<CharT>::min(),
                impl::decr(b.first)));
    a_.clear(range<CharT>(impl::incr(b.last),
                std::numeric_limits<CharT>::max()));
    return a_;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const& a, range<CharT> const& b)
{
    chset<CharT> a_(a);
    a_.clear(b);
    return a_;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, range<CharT> const& b)
{
    return a ^ chset<CharT>(b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(range<CharT> const& a, chset<CharT> const& b)
{
    chset<CharT> b_(b);
    b_.set(a);
    return b_;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(range<CharT> const& a, chset<CharT> const& b)
{
    chset<CharT> b_(b);
    b_.clear(range<CharT>(std::numeric_limits<CharT>::min(),
                impl::decr(a.first)));
    b_.clear(range<CharT>(impl::incr(a.last),
                std::numeric_limits<CharT>::max()));
    return b_;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(range<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) - b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(range<CharT> const& a, chset<CharT> const& b)
{
    return chset<CharT>(a) ^ b;
}

///////////////////////////////////////////////////////////////////////////////
//
//  literal primitives <--> chset free operators implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const& a, CharT b)
{
    return a | range<CharT>(b, b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& a, CharT b)
{
    return a & range<CharT>(b, b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const& a, CharT b)
{
    return a - range<CharT>(b, b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, CharT b)
{
    return a ^ range<CharT>(b, b);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(CharT a, chset<CharT> const& b)
{
    return range<CharT>(a, a) | b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(CharT a, chset<CharT> const& b)
{
    return range<CharT>(a, a) & b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(CharT a, chset<CharT> const& b)
{
    return range<CharT>(a, a) - b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(CharT a, chset<CharT> const& b)
{
    return range<CharT>(a, a) ^ b;
}

///////////////////////////////////////////////////////////////////////////////
//
//  chlit <--> chset free operators implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator~(chlit<CharT> const& a)
{
    return ~range<CharT>(a.ch, a.ch);
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const& a, chlit<CharT> const& b)
{
    return a | b.ch;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& a, chlit<CharT> const& b)
{
    return a & b.ch;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const& a, chlit<CharT> const& b)
{
    return a - b.ch;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, chlit<CharT> const& b)
{
    return a ^ b.ch;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chlit<CharT> const& a, chset<CharT> const& b)
{
    return a.ch | b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chlit<CharT> const& a, chset<CharT> const& b)
{
    return a.ch & b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chlit<CharT> const& a, chset<CharT> const& b)
{
    return a.ch - b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chlit<CharT> const& a, chset<CharT> const& b)
{
    return a.ch ^ b;
}

///////////////////////////////////////////////////////////////////////////////
//
//  anychar_parser <--> chset free operators
//
//      Where a is chset and b is a anychar_parser, and vice-versa, implements:
//
//          a | b, a & b, a - b, a ^ b
//
//      Where a is a chlit, implements:
//
//          ~a
//
///////////////////////////////////////////////////////////////////////////////
namespace impl {

    template <typename CharT>
    inline boost::spirit::range<CharT> const&
    full()
    {
        static boost::spirit::range<CharT> full_(
            std::numeric_limits<CharT>::min(),
            std::numeric_limits<CharT>::max());
        return full_;
    }

    template <typename CharT>
    inline boost::spirit::range<CharT> const&
    empty()
    {
        static boost::spirit::range<CharT> empty_;
        return empty_;
    }
}

//////////////////////////////////
inline nothing_parser
operator~(anychar_parser)
{
    return nothing_p;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const&, anychar_parser)
{
    return chset<CharT>(impl::full<CharT>());
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& a, anychar_parser)
{
    return a;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const&, anychar_parser)
{
    return chset<CharT>();
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, anychar_parser)
{
    return ~a;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(anychar_parser, chset<CharT> const& /*b*/)
{
    return chset<CharT>(impl::full<CharT>());
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(anychar_parser, chset<CharT> const& b)
{
    return b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(anychar_parser, chset<CharT> const& b)
{
    return ~b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(anychar_parser, chset<CharT> const& b)
{
    return ~b;
}

///////////////////////////////////////////////////////////////////////////////
//
//  nothing_parser <--> chset free operators implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(chset<CharT> const& a, nothing_parser)
{
    return a;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(chset<CharT> const& /*a*/, nothing_parser)
{
    return impl::empty<CharT>();
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(chset<CharT> const& a, nothing_parser)
{
    return a;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(chset<CharT> const& a, nothing_parser)
{
    return a;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator|(nothing_parser, chset<CharT> const& b)
{
    return b;
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator&(nothing_parser, chset<CharT> const& /*b*/)
{
    return impl::empty<CharT>();
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator-(nothing_parser, chset<CharT> const& /*b*/)
{
    return impl::empty<CharT>();
}

//////////////////////////////////
template <typename CharT>
inline chset<CharT>
operator^(nothing_parser, chset<CharT> const& b)
{
    return b;
}

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif

