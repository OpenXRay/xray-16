/*=============================================================================
    Phoenix V1.0
    Copyright (c) 2001-2002 Joel de Guzman
    MT code Copyright (c) 2002 Martin Wille

    Permission to copy, use, modify, sell and distribute this software
    is granted provided this copyright notice appears in all copies.
    This software is provided "as is" without express or implied
    warranty, and with no claim as to its suitability for any purpose.
==============================================================================*/
#ifndef PHOENIX_CLOSURES_HPP
#define PHOENIX_CLOSURES_HPP

///////////////////////////////////////////////////////////////////////////////
#include "actor.hpp"
#include <cassert>

#ifdef PHOENIX_THREADSAFE
#include <boost/thread/tss.hpp>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace phoenix {

///////////////////////////////////////////////////////////////////////////////
//
//  Adaptable closures
//
//      The framework will not be complete without some form of closures
//      support. Closures encapsulate a stack frame where local
//      variables are created upon entering a function and destructed
//      upon exiting. Closures provide an environment for local
//      variables to reside. Closures can hold heterogeneous types.
//
//      Phoenix closures are true hardware stack based closures. At the
//      very least, closures enable true reentrancy in lambda functions.
//      A closure provides access to a function stack frame where local
//      variables reside. Modeled after Pascal nested stack frames,
//      closures can be nested just like nested functions where code in
//      inner closures may access local variables from in-scope outer
//      closures (accessing inner scopes from outer scopes is an error
//      and will cause a run-time assertion failure).
//
//      There are three (3) interacting classes:
//
//      1) closure:
//
//      At the point of declaration, a closure does not yet create a
//      stack frame nor instantiate any variables. A closure declaration
//      declares the types and names[note] of the local variables. The
//      closure class is meant to be subclassed. It is the
//      responsibility of a closure subclass to supply the names for
//      each of the local variable in the closure. Example:
//
//          struct my_closure : closure<int, string, double> {
//
//              member1 num;        // names the 1st (int) local variable
//              member2 message;    // names the 2nd (string) local variable
//              member3 real;       // names the 3rd (double) local variable
//          };
//
//          my_closure clos;
//
//      Now that we have a closure 'clos', its local variables can be
//      accessed lazily using the dot notation. Each qualified local
//      variable can be used just like any primitive actor (see
//      primitives.hpp). Examples:
//
//          clos.num = 30
//          clos.message = arg1
//          clos.real = clos.num * 1e6
//
//      The examples above are lazily evaluated. As usual, these
//      expressions return composite actors that will be evaluated
//      through a second function call invocation (see operators.hpp).
//      Each of the members (clos.xxx) is an actor. As such, applying
//      the operator() will reveal its identity:
//
//          clos.num() // will return the current value of clos.num
//
//      *** [note] Acknowledgement: Juan Carlos Arevalo-Baeza (JCAB)
//      introduced and initilally implemented the closure member names
//      that uses the dot notation.
//
//      2) closure_member
//
//      The named local variables of closure 'clos' above are actually
//      closure members. The closure_member class is an actor and
//      conforms to its conceptual interface. member1..memberN are
//      predefined typedefs that correspond to each of the listed types
//      in the closure template parameters.
//
//      3) closure_frame
//
//      When a closure member is finally evaluated, it should refer to
//      an actual instance of the variable in the hardware stack.
//      Without doing so, the process is not complete and the evaluated
//      member will result to an assertion failure. Remember that the
//      closure is just a declaration. The local variables that a
//      closure refers to must still be instantiated.
//
//      The closure_frame class does the actual instantiation of the
//      local variables and links these variables with the closure and
//      all its members. There can be multiple instances of
//      closure_frames typically situated in the stack inside a
//      function. Each closure_frame instance initiates a stack frame
//      with a new set of closure local variables. Example:
//
//          void foo()
//          {
//              closure_frame<my_closure> frame(clos);
//              /* do something */
//          }
//
//      where 'clos' is an instance of our closure 'my_closure' above.
//      Take note that the usage above precludes locally declared
//      classes. If my_closure is a locally declared type, we can still
//      use its self_type as a paramater to closure_frame:
//
//          closure_frame<my_closure::self_type> frame(clos);
//
//      Upon instantiation, the closure_frame links the local variables
//      to the closure. The previous link to another closure_frame
//      instance created before is saved. Upon destruction, the
//      closure_frame unlinks itself from the closure and relinks the
//      preceding closure_frame prior to this instance.
//
//      The local variables in the closure 'clos' above is default
//      constructed in the stack inside function 'foo'. Once 'foo' is
//      exited, all of these local variables are destructed. In some
//      cases, default construction is not desirable and we need to
//      initialize the local closure variables with some values. This
//      can be done by passing in the initializers in a compatible
//      tuple. A compatible tuple is one with the same number of
//      elements as the destination and where each element from the
//      destination can be constructed from each corresponding element
//      in the source. Example:
//
//          tuple<int, char const*, int> init(123, "Hello", 1000);
//          closure_frame<my_closure> frame(clos, init);
//
//      Here now, our closure_frame's variables are initialized with
//      int: 123, char const*: "Hello" and int: 1000.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  closure_frame class
//
///////////////////////////////////////////////////////////////////////////////
template <typename ClosureT>
class closure_frame : public ClosureT::tuple_t {

public:

    closure_frame(ClosureT const& clos)
    : ClosureT::tuple_t(), save(clos.frame), frame(clos.frame)
    { clos.frame = this; }

    template <typename TupleT>
    closure_frame(ClosureT const& clos, TupleT const& init)
    : ClosureT::tuple_t(init), save(clos.frame), frame(clos.frame)
    { clos.frame = this; }

    ~closure_frame()
    { frame = save; }

private:

    closure_frame(closure_frame const&);            // no copy
    closure_frame& operator=(closure_frame const&); // no assign

    closure_frame* save;
    closure_frame*& frame;
};

///////////////////////////////////////////////////////////////////////////////
//
//  closure_member class
//
///////////////////////////////////////////////////////////////////////////////
template <int N, typename ClosureT>
class closure_member {

public:

    typedef typename ClosureT::tuple_t tuple_t;

    closure_member()
    : frame(ClosureT::closure_frame_ref()) {}

    template <typename TupleT>
    struct result {

        typedef typename tuple_element<
            N, typename ClosureT::tuple_t
        >::rtype type;
    };

    template <typename TupleT>
    typename tuple_element<N, typename ClosureT::tuple_t>::rtype
    eval(TupleT const& /*args*/) const
    {
        using namespace std;
        assert(frame != 0);
        return (*frame)[tuple_index<N>()];
    }

private:

    typename ClosureT::closure_frame_t*& frame;
};

///////////////////////////////////////////////////////////////////////////////
//
//  closure class
//
///////////////////////////////////////////////////////////////////////////////
template <
        typename T0 = nil_t
    ,   typename T1 = nil_t
    ,   typename T2 = nil_t

#if PHOENIX_LIMIT > 3
    ,   typename T3 = nil_t
    ,   typename T4 = nil_t
    ,   typename T5 = nil_t

#if PHOENIX_LIMIT > 6
    ,   typename T6 = nil_t
    ,   typename T7 = nil_t
    ,   typename T8 = nil_t

#if PHOENIX_LIMIT > 9
    ,   typename T9 = nil_t
    ,   typename T10 = nil_t
    ,   typename T11 = nil_t

#if PHOENIX_LIMIT > 12
    ,   typename T12 = nil_t
    ,   typename T13 = nil_t
    ,   typename T14 = nil_t

#endif
#endif
#endif
#endif
>
class closure {

public:

    typedef tuple<
            T0, T1, T2
#if PHOENIX_LIMIT > 3
        ,   T3, T4, T5
#if PHOENIX_LIMIT > 6
        ,   T6, T7, T8
#if PHOENIX_LIMIT > 9
        ,   T9, T10, T11
#if PHOENIX_LIMIT > 12
        ,   T12, T13, T14
#endif
#endif
#endif
#endif
        > tuple_t;

    typedef closure<
            T0, T1, T2
#if PHOENIX_LIMIT > 3
        ,   T3, T4, T5
#if PHOENIX_LIMIT > 6
        ,   T6, T7, T8
#if PHOENIX_LIMIT > 9
        ,   T9, T10, T11
#if PHOENIX_LIMIT > 12
        ,   T12, T13, T14
#endif
#endif
#endif
#endif
        > self_t;

    typedef closure_frame<self_t> closure_frame_t;

                            closure()
                            : frame(0)      { closure_frame_ref(&frame); }
    closure_frame_t&        context()       { assert(frame!=0); return frame; }
    closure_frame_t const&  context() const { assert(frame!=0); return frame; }

    typedef actor<closure_member<0, self_t> > member1;
    typedef actor<closure_member<1, self_t> > member2;
    typedef actor<closure_member<2, self_t> > member3;

#if PHOENIX_LIMIT > 3
    typedef actor<closure_member<3, self_t> > member4;
    typedef actor<closure_member<4, self_t> > member5;
    typedef actor<closure_member<5, self_t> > member6;

#if PHOENIX_LIMIT > 6
    typedef actor<closure_member<6, self_t> > member7;
    typedef actor<closure_member<7, self_t> > member8;
    typedef actor<closure_member<8, self_t> > member9;

#if PHOENIX_LIMIT > 9
    typedef actor<closure_member<9, self_t> > member10;
    typedef actor<closure_member<10, self_t> > member11;
    typedef actor<closure_member<11, self_t> > member12;

#if PHOENIX_LIMIT > 12
    typedef actor<closure_member<12, self_t> > member13;
    typedef actor<closure_member<13, self_t> > member14;
    typedef actor<closure_member<14, self_t> > member15;

#endif
#endif
#endif
#endif

#if !defined(__MWERKS__) || (__MWERKS__ > 0x3002)
private:
#endif

    closure(closure const&);            // no copy
    closure& operator=(closure const&); // no assign

#if !defined(__MWERKS__) || (__MWERKS__ > 0x3002)
    template <int N, typename ClosureT>
    friend class closure_member;

    template <typename ClosureT>
    friend class closure_frame;
#endif

    static closure_frame_t*&
    closure_frame_ref(closure_frame_t** frame_ = 0)
    {
#ifdef PHOENIX_THREADSAFE
        static boost::thread_specific_ptr<closure_frame_t **> tsp_frame;
        if (!tsp_frame.get())
            tsp_frame.reset(new closure_frame_t **(0));
        closure_frame_t **& frame = *tsp_frame;
#else
        static closure_frame_t** frame = 0;
#endif
        if (frame_ != 0)
            frame = frame_;
        return *frame;
    }

    mutable closure_frame_t* frame;
};

}
   //  namespace phoenix

#endif
