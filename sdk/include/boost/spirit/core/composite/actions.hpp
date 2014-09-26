/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTIONS_HPP
#define BOOST_SPIRIT_ACTIONS_HPP

///////////////////////////////////////////////////////////////////////////////
#include <algorithm>

#include "boost/spirit/core/parser.hpp"
#include "boost/spirit/core/composite/composite.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

    ///////////////////////////////////////////////////////////////////////////
    //
    //  action class
    //
    //      The action class binds a parser with a user defined semantic
    //      action. Instances of action are never created manually. Instead,
    //      action objects are typically created indirectly through
    //      expression templates of the form:
    //
    //          p[f]
    //
    //      where p is a parser and f is a function or functor. The semantic
    //      action may be a function or a functor. When the parser is
    //      successful, the actor calls the scanner's action_policy policy
    //      (see scanner.hpp):
    //
    //          scan.do_action(actor, attribute, first, last);
    //
    //      passing in these information:
    //
    //          actor:        The action's function or functor
    //          attribute:    The match (returned by the parser) object's
    //                        attribute (see match.hpp)
    //          first:        Iterator pointing to the start of the matching
    //                        portion of the input
    //          last:         Iterator pointing to one past the end of the
    //                        matching portion of the input
    //
    //      It is the responsibility of the scanner's action_policy policy to
    //      dispatch the function or functor as it sees fit. The expected
    //      function or functor signature depends on the parser being
    //      wrapped. In general, if the attribute type of the parser being
    //      wrapped is a nil_t, the function or functor expect the signature:
    //
    //          void func(Iterator first, Iterator last); // functions
    //
    //          struct ftor // functors
    //          {
    //              void func(Iterator first, Iterator last) const;
    //          };
    //
    //      where Iterator is the type of the iterator that is being used and
    //      first and last are the iterators pointing to the matching portion
    //      of the input.
    //
    //      If the attribute type of the parser being wrapped is not a nil_t,
    //      the function or functor usually expect the signature:
    //
    //          void func(T val); // functions
    //
    //          struct ftor // functors
    //          {
    //              void func(T val) const;
    //          };
    //
    //      where T is the attribute type and val is the attribute value
    //      returned by the parser being wrapped.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename ParserT, typename ActionT>
    class action : public unary<ParserT, parser<action<ParserT, ActionT> > >
    {
    public:

        typedef action<ParserT, ActionT>        self_t;
        typedef action_parser_category          parser_category_t;
        typedef unary<ParserT, parser<self_t> > base_t;
        typedef ActionT                         predicate_t;

        template <typename ScannerT>
        struct result
        {
            typedef typename parser_result<ParserT, ScannerT>::type type;
        };

        action()
        : base_t(ParserT())
        , actor(ActionT()) {}

        action(ParserT const& p, ActionT const& a)
        : base_t(p)
        , actor(a) {}

        template <typename ScannerT>
        typename parser_result<self_t, ScannerT>::type
        parse(ScannerT const& scan) const
        {
            typedef typename ScannerT::iterator_t iterator_t;
            typedef typename parser_result<self_t, ScannerT>::type result_t;

            scan.at_end(); // allow skipper to take effect
            iterator_t save = scan.first;
            result_t hit = this->subject().parse(scan);
            if (hit)
                scan.do_action(actor, hit.value(), save, scan.first);
            return hit;
        }

        ActionT const& predicate() const { return actor; }

    private:

        ActionT actor;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  assign_actor class
    //
    //      assign_actor is a predefined semantic action functor. It can be
    //      used to extract the result of a successful parse and assign it to
    //      a variable. The functor overloads two function call operators:
    //      operator(), one that takes in a single value argument and another
    //      that accepts two iterators (first and last).
    //
    //      The constructor expects a reference to a variable. The functor is
    //      polymorphic and should work with any variable type as long as it
    //      is compatible with the requirements outlined below.
    //
    //          1 The single argument function call operator assigns the
    //            argument received to the variable. The variable is required
    //            to accept the statement:
    //
    //                v = value;
    //
    //            where v is the variable and value is the extracted result
    //            of the parser.
    //
    //          2 The other function call operator that takes in the
    //            first/last iterator expects the variable to accept the
    //            statement:
    //
    //                v.assign(first, last);
    //
    //            2.a The variable has a member function assign, taking in
    //                the iterator pair. Any STL container that has an
    //                assign(first, last) member function may be used.
    //
    //          Requirements 1 and 2 are exclusive and applies only if the
    //          corresponding single or double argument operator is actually
    //          called.
    //
    //      Instances of assign_actor are not created directly. Instead a
    //      generator function:
    //
    //          assign(T& ref)
    //
    //      taking in a reference to a variable of arbitrary type is used to
    //      instantiate an assign_actor object of the proper type.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    class assign_actor
    {
    public:

        explicit
        assign_actor(T& ref_)
        : ref(ref_) {}

        template <typename T2>
        void operator()(T2 const& val) const
        { ref = val; }

        template <typename IteratorT>
        void operator()(IteratorT const& first, IteratorT const& last) const
        { ref.assign(first, last); }

    private:

        T& ref;
    };

    //////////////////////////////////
    template <typename T>
    inline assign_actor<T> const
    assign(T& ref)
    {
        return assign_actor<T>(ref);
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  append_actor class
    //
    //      append_actor is a predefined semantic action functor. It can be
    //      used to extract the result of a successful parse and append it to
    //      a variable. The functor overloads two function call operators:
    //      operator(), one that takes in a single value argument and another
    //      that accepts two iterators (first and last).
    //
    //      The constructor expects a reference to a variable. The functor is
    //      polymorphic and should work with any variable type as long as it
    //      is compatible with the requirements outlined below.
    //
    //          1 The variable is assumed to be a container of some sort. An
    //            STL container is a perfectly valid candidate.
    //
    //          2 The single argument function call operator appends the
    //            extracted parser result and appends it to the container.
    //            The container is required to accept the statement:
    //
    //                c.insert(c.end(), value)
    //
    //            where c is the container and value is the extracted result
    //            of the parser.
    //
    //            2.a The container is required to have a member function
    //                end() that returns an iterator to its 'end' element.
    //
    //            2.b The container is required to have a member function
    //                insert that takes in the 'end' iterator and a value
    //                compatible with the container's element type.
    //
    //          3 The function call operator that takes in the first/last
    //            iterator first constructs a value from the iterator pair
    //            before appending the value to the container. The container
    //            is required to accept the statement:
    //
    //                c.insert(c.end(), T::value_type(first, last));
    //
    //            where c is the container and T is the container type. In
    //            addition to the requirements 1 and 2 above,
    //
    //            3.a The container is also required to have a typedef
    //                value_type (the container's value type) that can be
    //                constructed given a first/last iterator pair.
    //
    //          Requirement 2 is exclusive of requirement 3. Requirement 3
    //          only applies if the corresponding double argument operator
    //          is actually called.
    //
    //      Instances of append_actor are not created directly. Instead a
    //      generator function:
    //
    //          append(T& ref)
    //
    //      taking in a reference to a variable of arbitrary type is used to
    //      instantiate an append_actor object of the proper type.
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    class append_actor
    {
    public:

        explicit
        append_actor(T& ref_)
        : ref(ref_) {}

        template <typename T2>
        void operator()(T2 const& val) const
        { ref.insert(ref.end(), val); }

        template <typename IteratorT>
        void operator()(IteratorT const& first, IteratorT const& last) const
        {
            typedef typename T::value_type value_type;
            ref.insert(ref.end(), value_type(first, last));
        }

    private:

        T& ref;
    };

    //////////////////////////////////
    template <typename T>
    inline append_actor<T> const
    append(T& ref)
    {
        return append_actor<T>(ref);
    }

}} // namespace boost::spirit

#endif
