/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2002-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_PARSER_TYPE_HPP)
#define BOOST_SPIRIT_PARSER_TYPE_HPP

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

#if !defined(BOOST_SPIRIT_PRIMITIVES_IPP)
#include "boost/spirit/core/primitives/primitives.hpp"
#endif

///////////////////////////////////////////////////////////////////////////////
//
//  Helper templates to derive the parser type from an auxilliary type and to
//  generate an object of the required parser type given an auxilliary object.
//  Supported types to convert are parsers, single characters and character
//  strings.
//
///////////////////////////////////////////////////////////////////////////////

    namespace impl
    {
        template<typename T>
        struct default_as_parser
        {
            typedef T type;
            static type const& convert(type const& p)
            { return p; }
        };

        struct char_as_parser
        {
            typedef chlit<char> type;
            static type convert(char ch)
            { return type(ch); }
        };

        struct wchar_as_parser
        {
            typedef chlit<wchar_t> type;
            static type convert(wchar_t ch)
            { return type(ch); }
        };

        struct string_as_parser
        {
            typedef strlit<char const*> type;
            static type convert(char const* str)
            { return type(str); }
        };

        struct wstring_as_parser
        {
            typedef strlit<wchar_t const*> type;
            static type convert(wchar_t const* str)
            { return type(str); }
        };
    }

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

    template<typename T>
    struct as_parser : impl::default_as_parser<T> {};

#else // !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

    template<typename T>
    struct as_parser
    {
        enum
        {
            is_cptr = boost::is_convertible<T, char const*>::value,
            is_wcptr
                = is_cptr ? false
                : boost::is_convertible<T, wchar_t const*>::value
        };

        typedef
            typename mpl::if_c<is_cptr,
                strlit<char const*>,
                typename mpl::if_c<is_wcptr,
                    strlit<wchar_t const*>,
                    T
                >::type
            >::type
        type;

        typedef
            typename mpl::if_c<is_cptr,
                char const*,
                typename mpl::if_c<is_wcptr,
                    wchar_t const*,
                    T const&
                >::type
            >::type
        param_type;

        typedef
            typename mpl::if_c<(is_cptr || is_wcptr),
                type,
                type const&
            >::type
        return_type;

        static return_type convert(param_type p)
        { return p; }
    };

#endif // !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

    template<>
    struct as_parser<char> : impl::char_as_parser {};

    template<>
    struct as_parser<wchar_t> : impl::wchar_as_parser {};

    template<>
    struct as_parser<char const*> : impl::string_as_parser {};

    template<>
    struct as_parser<wchar_t const*> : impl::wstring_as_parser {};

#ifdef __BORLANDC__

    template<>
    struct as_parser<char*> : impl::string_as_parser {};

    template<>
    struct as_parser<wchar_t*> : impl::wstring_as_parser {};

#endif // __BORLANDC__

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

    template<int N>
    struct as_parser<char[N]> : impl::string_as_parser {};

    template<int N>
    struct as_parser<wchar_t[N]> : impl::wstring_as_parser {};

#if !defined(__MWERKS__) || (__MWERKS__ > 0x2407)

    template<int N>
    struct as_parser<char const[N]> : impl::string_as_parser {};

    template<int N>
    struct as_parser<wchar_t const[N]> : impl::string_as_parser {};

#endif // !defined(__MWERKS__) || (__MWERKS__ > 0x2407)
#endif // !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)

}} // namespace boost::spirit

#endif
