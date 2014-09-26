/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_BORLAND_HPP)
#define BOOST_SPIRIT_BORLAND_HPP

#if defined(__BORLANDC__) && (__BORLANDC__ <= 0x561)
namespace boost { namespace spirit { namespace borland_only {

    //  Before including MPL, we define these dummy template
    //  functions. Borland complains when a template class
    //  has the same name as a template function, regardless if
    //  they are in different namespaces.

    template <typename T> void arg(T) {}

    namespace ftors
    {
        //  We define these dummy template functions. Borland complains when
        //  a template class has the same name as a template function,
        //  regardless if they are in different namespaces.

        template <typename T> void if_(T) {}
        template <typename T> void for_(T) {}
        template <typename T> void while_(T) {}
        template <typename T> void do_(T) {}
    }

    namespace tmpls
    {
        //  We define these dummy template functions. Borland complains when
        //  a template class has the same name as a template function,
        //  regardless if they are in different namespaces.

        template <typename T> struct if_ {};
        template <typename T> struct for_ {};
        template <typename T> struct while_ {};
        template <typename T> struct do_ {};
    }

}}} // namespace boost::spirit::borland_only
#endif
#endif

