/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2001-2003 Daniel Nuffer
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#ifndef BOOST_SPIRIT_CHSET_HPP
#define BOOST_SPIRIT_CHSET_HPP

///////////////////////////////////////////////////////////////////////////////
#include "boost/shared_ptr.hpp"

#include "boost/spirit/core/primitives/primitives.hpp"
#include "boost/spirit/utility/impl/chset/basic_chset.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

namespace impl {

    // This is here because some compilers choke on out-of-line member
    // template functions.  And we don't want to put the whole algorithm
    // in the chset constructor in the class definition.
    template <typename CharT, typename CharT2>
    void construct_chset(boost::shared_ptr<basic_chset<CharT> >& ptr,
            CharT2 const* definition);
}

///////////////////////////////////////////////////////////////////////////////
//
//  chset class
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT = char>
class chset: public char_parser<chset<CharT> > {

public:
                    chset();
                    chset(chset const& arg);
    explicit        chset(CharT arg);
    explicit        chset(anychar_parser arg);
    explicit        chset(nothing_parser arg);
    explicit        chset(chlit<CharT> const& arg);
    explicit        chset(range<CharT> const& arg);
                    template <typename CharT2>
    explicit        chset(CharT2 const* definition)
                    : ptr(new basic_chset<CharT>())
                    {
                        impl::construct_chset(ptr, definition);
                    }
                    ~chset();

    chset&          operator=(chset const& rhs);
    chset&          operator=(CharT rhs);
    chset&          operator=(anychar_parser rhs);
    chset&          operator=(nothing_parser rhs);
    chset&          operator=(chlit<CharT> const& rhs);
    chset&          operator=(range<CharT> const& rhs);

    void            set(range<CharT> const& arg);
    void            clear(range<CharT> const& arg);
    bool            test(CharT ch) const;
    chset&          inverse();
    void            swap(chset& x);

    chset&          operator|=(chset const& x);
    chset&          operator&=(chset const& x);
    chset&          operator-=(chset const& x);
    chset&          operator^=(chset const& x);

private:

    boost::shared_ptr<basic_chset<CharT> > ptr;
};

///////////////////////////////////////////////////////////////////////////////
//
//  Generator functions
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT>
inline chset<wchar_t>
chset_p(chlit<CharT> const& arg)
{ return chset<CharT>(arg); }

//////////////////////////////////
template <typename CharT>
inline chset<wchar_t>
chset_p(range<CharT> const& arg)
{ return chset<CharT>(arg); }

//////////////////////////////////
inline chset<char>
chset_p(char const* init)
{ return chset<char>(init); }

//////////////////////////////////
inline chset<wchar_t>
chset_p(wchar_t const* init)
{ return chset<wchar_t>(init); }

//////////////////////////////////
inline chset<char>
chset_p(char ch)
{ return chset<char>(ch); }

//////////////////////////////////
inline chset<wchar_t>
chset_p(wchar_t ch)
{ return chset<wchar_t>(ch); }

//////////////////////////////////
inline chset<int>
chset_p(int ch)
{ return chset<int>(ch); }

//////////////////////////////////
inline chset<unsigned int>
chset_p(unsigned int ch)
{ return chset<unsigned int>(ch); }

//////////////////////////////////
inline chset<short>
chset_p(short ch)
{ return chset<short>(ch); }

#if !defined(BOOST_NO_INTRINSIC_WCHAR_T)
//////////////////////////////////
inline chset<unsigned short>
chset_p(unsigned short ch)
{ return chset<unsigned short>(ch); }
#endif
//////////////////////////////////
inline chset<long>
chset_p(long ch)
{ return chset<long>(ch); }

//////////////////////////////////
inline chset<unsigned long>
chset_p(unsigned long ch)
{ return chset<unsigned long>(ch); }

#ifdef BOOST_HAS_LONG_LONG
//////////////////////////////////
inline chset<long long>
chset_p(long long ch)
{ return chset<long long>(ch); }

//////////////////////////////////
inline chset<unsigned long long>
chset_p(unsigned long long ch)
{ return chset<unsigned long long>(ch); }
#endif

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif

#if !defined(BOOST_SPIRIT_CHSET_IPP)
#include "boost/spirit/utility/impl/chset.ipp"
#endif

#if !defined(BOOST_SPIRIT_CHSET_OPERATORS_HPP)
#include "boost/spirit/utility/chset_operators.hpp"
#endif
