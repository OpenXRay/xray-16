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
#if !defined(BOOST_SPIRIT_OPERATORS_IPP)
#define BOOST_SPIRIT_OPERATORS_IPP

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
//
//  sequence class implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B>
inline sequence<A, B>
operator>>(parser<A> const& a, parser<B> const& b)
{
    return sequence<A, B>(a.derived(), b.derived());
}

//////////////////////////////////
template <typename A>
inline sequence<A, chlit<char> >
operator>>(parser<A> const& a, char b)
{
    return sequence<A, chlit<char> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline sequence<chlit<char>, B>
operator>>(char a, parser<B> const& b)
{
    return sequence<chlit<char>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline sequence<A, strlit<char const*> >
operator>>(parser<A> const& a, char const* b)
{
    return sequence<A, strlit<char const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline sequence<strlit<char const*>, B>
operator>>(char const* a, parser<B> const& b)
{
    return sequence<strlit<char const*>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline sequence<A, chlit<wchar_t> >
operator>>(parser<A> const& a, wchar_t b)
{
    return sequence<A, chlit<wchar_t> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline sequence<chlit<wchar_t>, B>
operator>>(wchar_t a, parser<B> const& b)
{
    return sequence<chlit<wchar_t>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline sequence<A, strlit<wchar_t const*> >
operator>>(parser<A> const& a, wchar_t const* b)
{
    return sequence<A, strlit<wchar_t const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline sequence<strlit<wchar_t const*>, B>
operator>>(wchar_t const* a, parser<B> const& b)
{
    return sequence<strlit<wchar_t const*>, B>(a, b.derived());
}

///////////////////////////////////////////////////////////////////////////////
//
//  sequential-and operators implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B>
inline sequence<A, B>
operator&&(parser<A> const& a, parser<B> const& b)
{
    return sequence<A, B>(a.derived(), b.derived());
}

//////////////////////////////////
template <typename A>
inline sequence<A, chlit<char> >
operator&&(parser<A> const& a, char b)
{
    return sequence<A, chlit<char> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline sequence<chlit<char>, B>
operator&&(char a, parser<B> const& b)
{
    return sequence<chlit<char>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline sequence<A, strlit<char const*> >
operator&&(parser<A> const& a, char const* b)
{
    return sequence<A, strlit<char const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline sequence<strlit<char const*>, B>
operator&&(char const* a, parser<B> const& b)
{
    return sequence<strlit<char const*>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline sequence<A, chlit<wchar_t> >
operator&&(parser<A> const& a, wchar_t b)
{
    return sequence<A, chlit<wchar_t> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline sequence<chlit<wchar_t>, B>
operator&&(wchar_t a, parser<B> const& b)
{
    return sequence<chlit<wchar_t>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline sequence<A, strlit<wchar_t const*> >
operator&&(parser<A> const& a, wchar_t const* b)
{
    return sequence<A, strlit<wchar_t const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline sequence<strlit<wchar_t const*>, B>
operator&&(wchar_t const* a, parser<B> const& b)
{
    return sequence<strlit<wchar_t const*>, B>(a, b.derived());
}

///////////////////////////////////////////////////////////////////////////////
//
//  sequential-or class implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B>
inline sequential_or<A, B>
operator||(parser<A> const& a, parser<B> const& b)
{
    return sequential_or<A, B>(a.derived(), b.derived());
}

//////////////////////////////////
template <typename A>
inline sequential_or<A, chlit<char> >
operator||(parser<A> const& a, char b)
{
    return sequential_or<A, chlit<char> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline sequential_or<chlit<char>, B>
operator||(char a, parser<B> const& b)
{
    return sequential_or<chlit<char>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline sequential_or<A, strlit<char const*> >
operator||(parser<A> const& a, char const* b)
{
    return sequential_or<A, strlit<char const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline sequential_or<strlit<char const*>, B>
operator||(char const* a, parser<B> const& b)
{
    return sequential_or<strlit<char const*>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline sequential_or<A, chlit<wchar_t> >
operator||(parser<A> const& a, wchar_t b)
{
    return sequential_or<A, chlit<wchar_t> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline sequential_or<chlit<wchar_t>, B>
operator||(wchar_t a, parser<B> const& b)
{
    return sequential_or<chlit<wchar_t>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline sequential_or<A, strlit<wchar_t const*> >
operator||(parser<A> const& a, wchar_t const* b)
{
    return sequential_or<A, strlit<wchar_t const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline sequential_or<strlit<wchar_t const*>, B>
operator||(wchar_t const* a, parser<B> const& b)
{
    return sequential_or<strlit<wchar_t const*>, B>(a, b.derived());
}

///////////////////////////////////////////////////////////////////////////////
//
//  alternative class implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B>
inline alternative<A, B>
operator|(parser<A> const& a, parser<B> const& b)
{
    return alternative<A, B>(a.derived(), b.derived());
}

//////////////////////////////////
template <typename A>
inline alternative<A, chlit<char> >
operator|(parser<A> const& a, char b)
{
    return alternative<A, chlit<char> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline alternative<chlit<char>, B>
operator|(char a, parser<B> const& b)
{
    return alternative<chlit<char>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline alternative<A, strlit<char const*> >
operator|(parser<A> const& a, char const* b)
{
    return alternative<A, strlit<char const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline alternative<strlit<char const*>, B>
operator|(char const* a, parser<B> const& b)
{
    return alternative<strlit<char const*>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline alternative<A, chlit<wchar_t> >
operator|(parser<A> const& a, wchar_t b)
{
    return alternative<A, chlit<wchar_t> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline alternative<chlit<wchar_t>, B>
operator|(wchar_t a, parser<B> const& b)
{
    return alternative<chlit<wchar_t>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline alternative<A, strlit<wchar_t const*> >
operator|(parser<A> const& a, wchar_t const* b)
{
    return alternative<A, strlit<wchar_t const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline alternative<strlit<wchar_t const*>, B>
operator|(wchar_t const* a, parser<B> const& b)
{
    return alternative<strlit<wchar_t const*>, B>(a, b.derived());
}

///////////////////////////////////////////////////////////////////////////////
//
//  intersection class implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B>
inline intersection<A, B>
operator&(parser<A> const& a, parser<B> const& b)
{
    return intersection<A, B>(a.derived(), b.derived());
}

//////////////////////////////////
template <typename A>
inline intersection<A, chlit<char> >
operator&(parser<A> const& a, char b)
{
    return intersection<A, chlit<char> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline intersection<chlit<char>, B>
operator&(char a, parser<B> const& b)
{
    return intersection<chlit<char>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline intersection<A, strlit<char const*> >
operator&(parser<A> const& a, char const* b)
{
    return intersection<A, strlit<char const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline intersection<strlit<char const*>, B>
operator&(char const* a, parser<B> const& b)
{
    return intersection<strlit<char const*>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline intersection<A, chlit<wchar_t> >
operator&(parser<A> const& a, wchar_t b)
{
    return intersection<A, chlit<wchar_t> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline intersection<chlit<wchar_t>, B>
operator&(wchar_t a, parser<B> const& b)
{
    return intersection<chlit<wchar_t>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline intersection<A, strlit<wchar_t const*> >
operator&(parser<A> const& a, wchar_t const* b)
{
    return intersection<A, strlit<wchar_t const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline intersection<strlit<wchar_t const*>, B>
operator&(wchar_t const* a, parser<B> const& b)
{
    return intersection<strlit<wchar_t const*>, B>(a, b.derived());
}

///////////////////////////////////////////////////////////////////////////////
//
//  difference class implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B>
inline difference<A, B>
operator-(parser<A> const& a, parser<B> const& b)
{
    return difference<A, B>(a.derived(), b.derived());
}

//////////////////////////////////
template <typename A>
inline difference<A, chlit<char> >
operator-(parser<A> const& a, char b)
{
    return difference<A, chlit<char> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline difference<chlit<char>, B>
operator-(char a, parser<B> const& b)
{
    return difference<chlit<char>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline difference<A, strlit<char const*> >
operator-(parser<A> const& a, char const* b)
{
    return difference<A, strlit<char const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline difference<strlit<char const*>, B>
operator-(char const* a, parser<B> const& b)
{
    return difference<strlit<char const*>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline difference<A, chlit<wchar_t> >
operator-(parser<A> const& a, wchar_t b)
{
    return difference<A, chlit<wchar_t> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline difference<chlit<wchar_t>, B>
operator-(wchar_t a, parser<B> const& b)
{
    return difference<chlit<wchar_t>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline difference<A, strlit<wchar_t const*> >
operator-(parser<A> const& a, wchar_t const* b)
{
    return difference<A, strlit<wchar_t const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline difference<strlit<wchar_t const*>, B>
operator-(wchar_t const* a, parser<B> const& b)
{
    return difference<strlit<wchar_t const*>, B>(a, b.derived());
}

///////////////////////////////////////////////////////////////////////////////
//
//  exclusive_or class implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B>
inline exclusive_or<A, B>
operator^(parser<A> const& a, parser<B> const& b)
{
    return exclusive_or<A, B>(a.derived(), b.derived());
}

//////////////////////////////////
template <typename A>
inline exclusive_or<A, chlit<char> >
operator^(parser<A> const& a, char b)
{
    return exclusive_or<A, chlit<char> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline exclusive_or<chlit<char>, B>
operator^(char a, parser<B> const& b)
{
    return exclusive_or<chlit<char>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline exclusive_or<A, strlit<char const*> >
operator^(parser<A> const& a, char const* b)
{
    return exclusive_or<A, strlit<char const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline exclusive_or<strlit<char const*>, B>
operator^(char const* a, parser<B> const& b)
{
    return exclusive_or<strlit<char const*>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline exclusive_or<A, chlit<wchar_t> >
operator^(parser<A> const& a, wchar_t b)
{
    return exclusive_or<A, chlit<wchar_t> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline exclusive_or<chlit<wchar_t>, B>
operator^(wchar_t a, parser<B> const& b)
{
    return exclusive_or<chlit<wchar_t>, B>(a, b.derived());
}

//////////////////////////////////
template <typename A>
inline exclusive_or<A, strlit<wchar_t const*> >
operator^(parser<A> const& a, wchar_t const* b)
{
    return exclusive_or<A, strlit<wchar_t const*> >(a.derived(), b);
}

//////////////////////////////////
template <typename B>
inline exclusive_or<strlit<wchar_t const*>, B>
operator^(wchar_t const* a, parser<B> const& b)
{
    return exclusive_or<strlit<wchar_t const*>, B>(a, b.derived());
}

///////////////////////////////////////////////////////////////////////////////
//
//  optional class implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename S>
optional<S>
operator!(parser<S> const& a)
{
    return optional<S>(a.derived());
}

///////////////////////////////////////////////////////////////////////////////
//
//  kleene_star class implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename S>
inline kleene_star<S>
operator*(parser<S> const& a)
{
    return kleene_star<S>(a.derived());
}

///////////////////////////////////////////////////////////////////////////////
//
//  positive class implementation
//
///////////////////////////////////////////////////////////////////////////////
template <typename S>
inline positive<S>
operator+(parser<S> const& a)
{
    return positive<S>(a.derived());
}

///////////////////////////////////////////////////////////////////////////////
//
//  operator% is defined as:
//  a % b ---> a >> *(b >> a)
//
///////////////////////////////////////////////////////////////////////////////
template <typename A, typename B>
inline sequence<A, kleene_star<sequence<B, A> > >
operator%(parser<A> const& a, parser<B> const& b)
{
    return a.derived() >> *(b.derived() >> a.derived());
}

//////////////////////////////////
template <typename A>
inline sequence<A, kleene_star<sequence<chlit<char>, A> > >
operator%(parser<A> const& a, char b)
{
    return a.derived() >> *(b >> a.derived());
}

//////////////////////////////////
template <typename B>
inline sequence<chlit<char>, kleene_star<sequence<B, chlit<char> > > >
operator%(char a, parser<B> const& b)
{
    return a >> *(b.derived() >> a);
}

//////////////////////////////////
template <typename A>
inline sequence<A, kleene_star<sequence<strlit<char const*>, A> > >
operator%(parser<A> const& a, char const* b)
{
    return a.derived() >> *(b >> a.derived());
}

//////////////////////////////////
template <typename B>
inline sequence<strlit<char const*>,
    kleene_star<sequence<B, strlit<char const*> > > >
operator%(char const* a, parser<B> const& b)
{
    return a >> *(b.derived() >> a);
}

//////////////////////////////////
template <typename A>
inline sequence<A, kleene_star<sequence<chlit<wchar_t>, A> > >
operator%(parser<A> const& a, wchar_t b)
{
    return a.derived() >> *(b >> a.derived());
}

//////////////////////////////////
template <typename B>
inline sequence<chlit<wchar_t>, kleene_star<sequence<B, chlit<wchar_t> > > >
operator%(wchar_t a, parser<B> const& b)
{
    return a >> *(b.derived() >> a);
}

//////////////////////////////////
template <typename A>
inline sequence<A, kleene_star<sequence<strlit<wchar_t const*>, A> > >
operator%(parser<A> const& a, wchar_t const* b)
{
    return a.derived() >> *(b >> a.derived());
}

//////////////////////////////////
template <typename B>
inline sequence<strlit<wchar_t const*>,
    kleene_star<sequence<B, strlit<wchar_t const*> > > >
operator%(wchar_t const* a, parser<B> const& b)
{
    return a >> *(b.derived() >> a);
}

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif
