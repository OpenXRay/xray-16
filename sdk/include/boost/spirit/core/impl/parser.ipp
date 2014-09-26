/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_PARSER_IPP)
#define BOOST_SPIRIT_PARSER_IPP

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Generic parse function implementation
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename IteratorT, typename DerivedT>
    inline parse_info<IteratorT>
    parse(
        IteratorT const&        first_,
        IteratorT const&        last,
        parser<DerivedT> const& p)
    {
        IteratorT first = first_;
        scanner<IteratorT, scanner_policies<> > scan(first, last);
        match<nil_t> hit = p.derived().parse(scan);
        return parse_info<IteratorT>(
            first, hit, hit && (first == last), hit.length());
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Parse function for null terminated strings implementation
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename CharT, typename DerivedT>
    inline parse_info<CharT const*>
    parse(
        CharT const*            str,
        parser<DerivedT> const& p)
    {
        CharT const* last = str;
        while (*last)
            last++;
        return parse(str, last, p);
    }

}} // namespace boost::spirit

#endif

