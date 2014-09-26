/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#ifndef BOOST_SPIRIT_REGEX_IPP
#define BOOST_SPIRIT_REGEX_IPP

///////////////////////////////////////////////////////////////////////////////
#include "boost/spirit/core/primitives/impl/primitives.ipp"

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

namespace impl {

///////////////////////////////////////////////////////////////////////////////
//
//  rx_parser class
//
///////////////////////////////////////////////////////////////////////////////
template <typename CharT = char>
class rx_parser : public parser<rx_parser<CharT> > {

public:
    typedef std::basic_string<CharT> string_t;
    typedef rx_parser<CharT> self_t;

    rx_parser(CharT const *first, CharT const *last)
    { rxstr = string_t("^") + string_t(first, last); }

    rx_parser(CharT const *first)
    { rxstr = string_t("^") + string_t(first, impl::get_last(first)); }

    template <typename ScannerT>
    typename parser_result<self_t, ScannerT>::type
    parse(ScannerT const& scan) const
    {
        boost::match_results<typename ScannerT::iterator_t> what;
        boost::regex_search(scan.first, scan.last, what, rxstr,
            boost::match_default);

        if (!what[0].matched)
            return scan.no_match();

        scan.first = what[0].second;
        return scan.create_match(what[0].length(), nil_t(),
            what[0].first, scan.first);
    }

private:
    boost::reg_expression<CharT> rxstr;    // regular expression to match
};

}   // namespace impl

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif // BOOST_SPIRIT_REGEX_IPP
