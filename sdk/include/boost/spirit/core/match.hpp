/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_MATCH_HPP)
#define BOOST_SPIRIT_MATCH_HPP

///////////////////////////////////////////////////////////////////////////////
#include "boost/call_traits.hpp"

#if !defined(BOOST_SPIRIT_BASICS_HPP)
#include "boost/spirit/core/basics.hpp"
#endif

#if !defined(BOOST_SPIRIT_MATCH_IPP)
#include "boost/spirit/core/impl/match.ipp"
#endif

#if !defined(BOOST_SPIRIT_ASSERT_HPP)
#include "boost/spirit/core/assert.hpp"
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

    ///////////////////////////////////////////////////////////////////////////
    //
    //  match class
    //
    //      The match holds the result of a parser. A match object evaluates
    //      to true when a succesful match is found, otherwise false. The
    //      length of the match is the number of characters (or tokens) that
    //      is successfully matched. This can be queried through its length()
    //      member function. A negative value means that the match is
    //      unsucessful.
    //
    //      Each parser may have an associated attribute. This attribute is
    //      also returned back to the client on a successful parse through
    //      the match object. The match's value() member function returns a
    //      reference to this value.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename T = nil_t>
    class match
    {
    public:

        typedef T attr_t;
        typedef typename boost::call_traits<T>::param_type      param_type;
        typedef typename boost::call_traits<T>::reference       reference;
        typedef typename boost::call_traits<T>::const_reference const_reference;

        match()
        : len(-1), val(impl::match_attr<T>::get_default()) {}

        explicit
        match(unsigned length)
        : len(length), val((impl::match_attr<T>::get_default())) {}

        match(unsigned length, param_type val_)
        : len(length), val(val_) {}

        operator impl::safe_bool() const
        { return BOOST_SPIRIT_SAFE_BOOL(len >= 0); }

        bool operator!() const
        { return len < 0; }

        int             length() const  { return len; }
        const_reference value() const   { return val; }
        reference       value()         { return val; }

        template <typename T2>
        match(match<T2> const& other)
        : len(other.length()), val(impl::match_attr<T>::get(other)) {}

        template <typename T2>
        match&
        operator=(match<T2> const& other)
        {
            len = other.length();
            val = impl::match_attr<T>::get(other);
            return *this;
        }

        template <typename MatchT>
        void
        concat(MatchT const& other)
        {
            BOOST_SPIRIT_ASSERT(*this && other);
            len += other.length();
        }

    private:

        int len;
        T   val;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  match class specialization for nil_t values
    //
    ///////////////////////////////////////////////////////////////////////////
    template <>
    class match<nil_t>
    {

    public:

        typedef nil_t attr_t;

        match()
        : len(-1) {}

        explicit
        match(unsigned length)
        : len(length) {}

        match(unsigned length, nil_t)
        : len(length) {}

        operator impl::safe_bool() const
        { return BOOST_SPIRIT_SAFE_BOOL(len >= 0); }

        bool operator!() const
        { return len < 0; }

        int     length() const  { return len; }
        nil_t   value() const   { return nil_t(); }

        template <typename T>
        match(match<T> const& other)
        : len(other.length()) {}

        template <typename T>
        match<>&
        operator=(match<T> const& other)
        {
            len = other.length();
            return *this;
        }

        template <typename T>
        void
        concat(match<T> const& other)
        {
            BOOST_SPIRIT_ASSERT(*this && other);
            len += other.length();
        }

    private:

        int len;
    };

}} // namespace boost::spirit

#endif

