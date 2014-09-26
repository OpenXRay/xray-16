#ifndef BOOST_LEXICAL_CAST_INCLUDED
#define BOOST_LEXICAL_CAST_INCLUDED

// Boost lexical_cast.hpp header  -------------------------------------------//
//
// See http://www.boost.org for most recent version including documentation.
// See end of this header for rights and permissions.
//
// what:  lexical_cast custom keyword cast
// who:   contributed by Kevlin Henney,
//        enhanced with contributions from Terje Slettebø,
//        with additional fixes and suggestions from Gennaro Prota,
//        Beman Dawes, Dave Abrahams, Daryle Walker, Peter Dimov,
//        and other Boosters
// when:  November 2000, March 2003

#include <string>
#include <typeinfo>
#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <boost/type_traits/is_pointer.hpp>

#ifdef BOOST_NO_STRINGSTREAM
#include <strstream>
#else
#include <sstream>
#endif

#if defined(BOOST_NO_STRINGSTREAM) || \
    defined(BOOST_NO_STD_WSTRING) || \
    defined(BOOST_NO_STD_LOCALE) || \
    defined(BOOST_NO_CWCHAR) || \
    defined(BOOST_MSVC) && (BOOST_MSVC <= 1200)
#define DISABLE_WIDE_CHAR_SUPPORT
#endif

#ifdef BOOST_NO_INTRINSIC_WCHAR_T
#include <cwchar>
#endif

namespace boost
{
    // exception used to indicate runtime lexical_cast failure
    class bad_lexical_cast : public std::bad_cast
    {
    public:
        virtual ~bad_lexical_cast() throw()
        {
        }
    };

    namespace detail // actual underlying concrete exception type
    {
        template<typename Target, typename Source>
        class no_lexical_conversion : public bad_lexical_cast
        {
        public:
            no_lexical_conversion()
              : description(
                  std::string() + "bad lexical cast: " +
                  "source type value could not be interpreted as target, Target=" +
                  typeid(Target).name() + ", Source=" + typeid(Source).name())
            {
            }
            virtual ~no_lexical_conversion() throw()
            {
            }
            virtual const char *what() const throw()
            {
                return description.c_str();
            }
        private:
            const std::string description; // static initialization fails on MSVC6
        };
    }

    namespace detail // selectors for choosing stream character type
    {
        template<typename Type>
        struct stream_char
        {
            typedef char type;
        };

        #ifndef DISABLE_WIDE_CHAR_SUPPORT
        template<>
        struct stream_char<wchar_t>
        {
            typedef wchar_t type;
        };

        template<>
        struct stream_char<wchar_t *>
        {
            typedef wchar_t type;
        };

        template<>
        struct stream_char<const wchar_t *>
        {
            typedef wchar_t type;
        };

        template<>
        struct stream_char<std::wstring>
        {
            typedef wchar_t type;
        };
        #endif

        template<typename TargetChar, typename SourceChar>
        struct widest_char
        {
            typedef TargetChar type;
        };

        template<>
        struct widest_char<char, wchar_t>
        {
            typedef wchar_t type;
        };
    }
    
    namespace detail // stream wrapper for handling lexical conversions
    {
        template<typename Target, typename Source>
        class lexical_stream
        {
        public:
            lexical_stream()
            {
                stream.unsetf(std::ios::skipws);

                if(std::numeric_limits<Target>::is_specialized)
                    stream.precision(std::numeric_limits<Target>::digits10 + 1);
                else if(std::numeric_limits<Source>::is_specialized)
                    stream.precision(std::numeric_limits<Source>::digits10 + 1);
            }
            ~lexical_stream()
            {
                #if defined(BOOST_NO_STRINGSTREAM)
                stream.freeze(false);
                #endif
            }
            bool operator<<(const Source &input)
            {
                return stream << input;
            }
            template<typename InputStreamable>
            bool operator>>(InputStreamable &output)
            {
                return !is_pointer<InputStreamable>::value &&
                       stream >> output &&
                       (stream >> std::ws).eof();
            }
            bool operator>>(std::string &output)
            {
                #if defined(BOOST_NO_STRINGSTREAM)
                stream << '\0';
                #endif
                output = stream.str();
                return true;
            }
            #ifndef DISABLE_WIDE_CHAR_SUPPORT
            bool operator>>(std::wstring &output)
            {
                output = stream.str();
                return true;
            }
            #endif
        private:
            typedef typename widest_char<
                typename stream_char<Target>::type,
                typename stream_char<Source>::type>::type char_type;

            #if defined(BOOST_NO_STRINGSTREAM)
            std::strstream stream;
            #elif defined(BOOST_NO_STD_LOCALE)
            std::stringstream stream;
            #else
            std::basic_stringstream<char_type> stream;
            #endif
        };
    }

    template<typename Target, typename Source>
    Target lexical_cast(Source arg)
    {
        detail::lexical_stream<Target, Source> interpreter;
        Target result;

        if(!(interpreter << arg && interpreter >> result))
            throw detail::no_lexical_conversion<Target, Source>();
        return result;
    }
}

// Copyright Kevlin Henney, 2000-2003. All rights reserved.
//
// Permission to use, copy, modify, and distribute this software for any
// purpose is hereby granted without fee, provided that this copyright and
// permissions notice appear in all copies and derivatives.
//
// This software is provided "as is" without express or implied warranty.

#undef DISABLE_WIDE_CHAR_SUPPORT
#endif
