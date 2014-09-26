/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2002 Joel de Guzman
    Copyright (c) 2001 Daniel Nuffer
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.

    See spirit.hpp for full copyright notices.
=============================================================================*/
#if !defined(BOOST_SPIRIT_SCANNER_HPP)
#define BOOST_SPIRIT_SCANNER_HPP

///////////////////////////////////////////////////////////////////////////////
#include <iterator>

#include "boost/config.hpp"
#include "boost/spirit/core/match.hpp"
#include "boost/spirit/core/non_terminal/parser_id.hpp"

#if defined(BOOST_MSVC)
#include "boost/spirit/core/impl/msvc.hpp"
#endif

#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)
#define BOOST_SPIRIT_IT_NS impl
#define BOOST_SPIRIT_MP_TYPE_COMPUTER_ARGS \
    typename T, typename Pizza = nil_t
#define BOOST_SPIRIT_P_TYPE_COMPUTER_ARGS \
    typename PoliciesT2, typename Pizza = nil_t
#define BOOST_SPIRIT_I_TYPE_COMPUTER_ARGS \
    typename IteratorT2, typename Pizza = nil_t
#else
#define BOOST_SPIRIT_IT_NS std
#define BOOST_SPIRIT_MP_TYPE_COMPUTER_ARGS typename T
#define BOOST_SPIRIT_P_TYPE_COMPUTER_ARGS typename PoliciesT2
#define BOOST_SPIRIT_I_TYPE_COMPUTER_ARGS typename IteratorT2
#endif

#if (defined(BOOST_INTEL_CXX_VERSION) && !defined(_STLPORT_VERSION))
#undef BOOST_SPIRIT_IT_NS
#define BOOST_SPIRIT_IT_NS impl
#endif

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

///////////////////////////////////////////////////////////////////////////////
//
//  iteration_policy class
//
///////////////////////////////////////////////////////////////////////////////
struct iteration_policy
{
    template <typename ScannerT>
    void
    advance(ScannerT const& scan) const
    { ++scan.first; }

    template <typename ScannerT>
    bool at_end(ScannerT const& scan) const
    { return scan.first == scan.last; }

    template <typename T>
    T filter(T ch) const
    { return ch; }

    template <typename ScannerT>
    typename ScannerT::ref_t
    get(ScannerT const& scan) const
    { return *scan.first; }
};

///////////////////////////////////////////////////////////////////////////////
//
//  match_policy class
//
///////////////////////////////////////////////////////////////////////////////
struct match_policy
{
    template <BOOST_SPIRIT_MP_TYPE_COMPUTER_ARGS>
    struct result { typedef match<T> type; };

    const match<nil_t>
    no_match() const
    { return match<nil_t>(); }

    const match<nil_t>
    empty_match() const
    { return match<nil_t>(0, nil_t()); }

    template <typename AttrT, typename IteratorT>
    match<AttrT>
    create_match(
        unsigned            length,
        AttrT const&        val,
        IteratorT const&    /*first*/,
        IteratorT const&    /*last*/) const
    { return match<AttrT>(length, val); }

    template <typename MatchT, typename IteratorT>
    void
    group_match(
        MatchT&             /*m*/,
        parser_id const&    /*id*/,
        IteratorT const&    /*first*/,
        IteratorT const&    /*last*/) const {}

    template <typename Match1T, typename Match2T>
    void
    concat_match(Match1T& l, Match2T const& r) const
    { l.concat(r); }
};

///////////////////////////////////////////////////////////////////////////////
//
//  match_result class
//
///////////////////////////////////////////////////////////////////////////////
#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)

BOOST_SPIRIT_DEPENDENT_TEMPLATE_WRAPPER(match_result_wrapper, result);

//////////////////////////////////
template <typename MatchPolicyT, typename T>
struct match_result
{
    typedef typename impl::match_result_wrapper<MatchPolicyT>
        ::template result_<T>::type type;
};

#else

//////////////////////////////////
template <typename MatchPolicyT, typename T>
struct match_result
{
    typedef typename MatchPolicyT::template result<T>::type type;
};

#endif

///////////////////////////////////////////////////////////////////////////////
//
//  action_policy class
//
///////////////////////////////////////////////////////////////////////////////
template <typename AttrT>
struct attributed_action_policy
{
    template <typename ActorT, typename IteratorT>
    static void
    call(
        ActorT const& actor,
        AttrT const& val,
        IteratorT const&,
        IteratorT const&)
    { actor(val); }
};

//////////////////////////////////
template <>
struct attributed_action_policy<nil_t>
{
    template <typename ActorT, typename IteratorT>
    static void
    call(
        ActorT const& actor,
        nil_t,
        IteratorT const& first,
        IteratorT const& last)
    { actor(first, last); }
};

//////////////////////////////////
struct action_policy
{
    template <typename ActorT, typename AttrT, typename IteratorT>
    void
    do_action(
        ActorT const&       actor,
        AttrT const&        val,
        IteratorT const&    first,
        IteratorT const&    last) const
    { attributed_action_policy<AttrT>::call(actor, val, first, last); }
};

///////////////////////////////////////////////////////////////////////////////
//
//  scanner_policies class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename IterationPolicyT   = iteration_policy,
    typename MatchPolicyT       = match_policy,
    typename ActionPolicyT      = action_policy>
struct scanner_policies :
    public IterationPolicyT,
    public MatchPolicyT,
    public ActionPolicyT
{
    typedef IterationPolicyT    iteration_policy_t;
    typedef MatchPolicyT        match_policy_t;
    typedef ActionPolicyT       action_policy_t;

    scanner_policies(
        IterationPolicyT const& i_policy = IterationPolicyT(),
        MatchPolicyT const&     m_policy = MatchPolicyT(),
        ActionPolicyT const&    a_policy = ActionPolicyT())
    : IterationPolicyT(i_policy)
    , MatchPolicyT(m_policy)
    , ActionPolicyT(a_policy) {}

    template <typename ScannerPoliciesT>
    scanner_policies(ScannerPoliciesT const& policies)
    : IterationPolicyT(policies)
    , MatchPolicyT(policies)
    , ActionPolicyT(policies) {}
};

///////////////////////////////////////////////////////////////////////////////
//
//  scanner class
//
///////////////////////////////////////////////////////////////////////////////
template <
    typename IteratorT  = char const*,
    typename PoliciesT  = scanner_policies<> >
class scanner : public PoliciesT {

public:

    typedef IteratorT iterator_t;
    typedef PoliciesT policies_t;

    typedef typename BOOST_SPIRIT_IT_NS::iterator_traits<IteratorT>::value_type
        value_t;
    typedef typename BOOST_SPIRIT_IT_NS::iterator_traits<IteratorT>::reference
        ref_t;
    typedef typename boost::call_traits<IteratorT>::param_type
        iter_param_t;

    scanner(
        IteratorT&          first_,
        iter_param_t        last_,
        PoliciesT const&    policies = PoliciesT())
    : PoliciesT(policies), first(first_), last(last_) {
        at_end();
    }

    scanner(scanner const& other)
    : PoliciesT(other), first(other.first), last(other.last) {}

    scanner(scanner const& other, IteratorT& first_)
    : PoliciesT(other), first(first_), last(other.last) {}

    bool
    at_end() const
    {
        typedef typename PoliciesT::iteration_policy_t iteration_policy_t;
        return iteration_policy_t::at_end(*this);
    }

    value_t
    operator*() const
    {
        typedef typename PoliciesT::iteration_policy_t iteration_policy_t;
        return iteration_policy_t::filter(iteration_policy_t::get(*this));
    }

    scanner const&
    operator++() const
    {
        typedef typename PoliciesT::iteration_policy_t iteration_policy_t;
        iteration_policy_t::advance(*this);
        return *this;
    }

    template <BOOST_SPIRIT_P_TYPE_COMPUTER_ARGS>
    struct rebind_policies {

        typedef scanner<IteratorT, PoliciesT2> type;
    };

    template <typename PoliciesT2>
    scanner<IteratorT, PoliciesT2>
    change_policies(PoliciesT2 const& policies) const
    {
        return scanner<IteratorT, PoliciesT2>(first, last, policies);
    }

    template <BOOST_SPIRIT_I_TYPE_COMPUTER_ARGS>
    struct rebind_iterator {

        typedef scanner<IteratorT2, PoliciesT> type;
    };

    template <typename IteratorT2>
    scanner<IteratorT2, PoliciesT>
    change_iterator(IteratorT2 const& first_, IteratorT2 const &last_) const
    {
        return scanner<IteratorT2, PoliciesT>(first_, last_, *this);
    }

    IteratorT& first;
    IteratorT const last;

private:

    scanner&
    operator=(scanner const& other);
};

///////////////////////////////////////////////////////////////////////////////
//
//  rebind_scanner_policies class
//
///////////////////////////////////////////////////////////////////////////////
#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1300)

BOOST_SPIRIT_DEPENDENT_TEMPLATE_WRAPPER(
    rebind_scanner_policies_wrapper, rebind_policies);

//////////////////////////////////
template <typename ScannerT, typename PoliciesT>
struct rebind_scanner_policies
{
    typedef typename impl::rebind_scanner_policies_wrapper<ScannerT>
        ::template result_<PoliciesT>::type type;
};

//////////////////////////////////
BOOST_SPIRIT_DEPENDENT_TEMPLATE_WRAPPER(
    rebind_scanner_iterator_wrapper, rebind_iterator);

template <typename ScannerT, typename IteratorT>
struct rebind_scanner_iterator
{
    typedef typename impl::rebind_scanner_iterator_wrapper<ScannerT>
        ::template result_<IteratorT>::type type;
};

#else

//////////////////////////////////
template <typename ScannerT, typename PoliciesT>
struct rebind_scanner_policies
{
    typedef typename ScannerT::template
        rebind_policies<PoliciesT>::type type;
};

//////////////////////////////////
template <typename ScannerT, typename IteratorT>
struct rebind_scanner_iterator
{
    typedef typename ScannerT::template
        rebind_iterator<IteratorT>::type type;
};

#endif

///////////////////////////////////////////////////////////////////////////////
}} // namespace boost::spirit

#endif
#undef BOOST_SPIRIT_IT_NS
#undef BOOST_SPIRIT_MP_TYPE_COMPUTER_ARGS
#undef BOOST_SPIRIT_P_TYPE_COMPUTER_ARGS
