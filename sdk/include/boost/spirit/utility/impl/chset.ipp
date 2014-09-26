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
#ifndef BOOST_SPIRIT_CHSET_IPP
#define BOOST_SPIRIT_CHSET_IPP

///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_CHSET_HPP)
#include "boost/spirit/utility/chset.hpp"
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
//
//  chset class
//
///////////////////////////////////////////////////////////////////////////////
namespace impl
{
    template <typename CharT>
    inline void
    detach(boost::shared_ptr<basic_chset<CharT> >& ptr)
    {
        if (!ptr.unique())
            ptr = boost::shared_ptr<basic_chset<CharT> >
                (new basic_chset<CharT>(*ptr));
    }

    template <typename CharT>
    inline void
    detach_clear(boost::shared_ptr<basic_chset<CharT> >& ptr)
    {
        if (ptr.unique())
            ptr->clear();
        else
            ptr = new basic_chset<CharT>();
    }

    template <typename CharT, typename CharT2>
    void construct_chset(boost::shared_ptr<basic_chset<CharT> >& ptr,
            CharT2 const* definition)
    {
        CharT2 ch = *definition++;
        while (ch)
        {
            CharT2 next = *definition++;
            if (next == '-')
            {
                next = *definition++;
                if (next == 0)
                {
                    ptr->set(ch);
                    ptr->set('-');
                    break;
                }
                ptr->set(ch, next);
            }
            else
            {
                ptr->set(ch);
            }
            ch = next;
        }
    }
}

template <typename CharT>
inline chset<CharT>::chset()
: ptr(new basic_chset<CharT>()) {}

template <typename CharT>
inline chset<CharT>::chset(chset const& arg)
: ptr(new basic_chset<CharT>(*arg.ptr)) {}

template <typename CharT>
inline chset<CharT>::chset(CharT arg)
: ptr(new basic_chset<CharT>())
{ ptr->set(arg); }

template <typename CharT>
inline chset<CharT>::chset(anychar_parser arg)
: ptr(new basic_chset<CharT>())
{
    ptr->set(
        std::numeric_limits<CharT>::min(),
        std::numeric_limits<CharT>::max()
    );
}

template <typename CharT>
inline chset<CharT>::chset(nothing_parser arg)
: ptr(new basic_chset<CharT>()) {}

template <typename CharT>
inline chset<CharT>::chset(chlit<CharT> const& arg)
: ptr(new basic_chset<CharT>())
{ ptr->set(arg.ch); }

template <typename CharT>
inline chset<CharT>::chset(range<CharT> const& arg)
: ptr(new basic_chset<CharT>())
{ ptr->set(arg.first, arg.last); }

template <typename CharT>
inline chset<CharT>::~chset() {}

template <typename CharT>
inline chset<CharT>&
chset<CharT>::operator=(chset const& rhs)
{
    ptr = rhs.ptr;
    return *this;
}

template <typename CharT>
inline chset<CharT>&
chset<CharT>::operator=(CharT rhs)
{
    impl::detach_clear(ptr);
    ptr->set(rhs);
    return *this;
}

template <typename CharT>
inline chset<CharT>&
chset<CharT>::operator=(anychar_parser rhs)
{
    impl::detach_clear(ptr);
    ptr->set(
        std::numeric_limits<CharT>::min(),
        std::numeric_limits<CharT>::max()
    );
    return *this;
}

template <typename CharT>
inline chset<CharT>&
chset<CharT>::operator=(nothing_parser rhs)
{
    impl::detach_clear(ptr);
    return *this;
}

template <typename CharT>
inline chset<CharT>&
chset<CharT>::operator=(chlit<CharT> const& rhs)
{
    impl::detach_clear(ptr);
    ptr->set(rhs.ch);
    return *this;
}

template <typename CharT>
inline chset<CharT>&
chset<CharT>::operator=(range<CharT> const& rhs)
{
    impl::detach_clear(ptr);
    ptr->set(rhs.first, rhs.last);
    return *this;
}

template <typename CharT>
inline void
chset<CharT>::set(range<CharT> const& arg)
{
    impl::detach(ptr);
    ptr->set(arg.first, arg.last);
}

template <typename CharT>
inline void
chset<CharT>::clear(range<CharT> const& arg)
{
    impl::detach(ptr);
    ptr->clear(arg.first, arg.last);
}

template <typename CharT>
inline bool
chset<CharT>::test(CharT ch) const
{ return ptr->test(ch); }

template <typename CharT>
inline chset<CharT>&
chset<CharT>::inverse()
{
    impl::detach(ptr);
    ptr->inverse();
    return *this;
}

template <typename CharT>
inline void
chset<CharT>::swap(chset& x)
{ ptr.swap(x.ptr); }

template <typename CharT>
inline chset<CharT>&
chset<CharT>::operator|=(chset const& x)
{
    impl::detach(ptr);
    *ptr |= *x.ptr;
    return *this;
}

template <typename CharT>
inline chset<CharT>&
chset<CharT>::operator&=(chset const& x)
{
    impl::detach(ptr);
    *ptr &= *x.ptr;
    return *this;
}

template <typename CharT>
inline chset<CharT>&
chset<CharT>::operator-=(chset const& x)
{
    impl::detach(ptr);
    *ptr -= *x.ptr;
    return *this;
}

template <typename CharT>
inline chset<CharT>&
chset<CharT>::operator^=(chset const& x)
{
    impl::detach(ptr);
    *ptr ^= *x.ptr;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif

