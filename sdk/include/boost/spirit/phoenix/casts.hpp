/*=============================================================================
    Phoenix V1.0
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2001-2003 Hartmut Kaiser

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
==============================================================================*/

#ifndef PHOENIX_CASTS_HPP
#define PHOENIX_CASTS_HPP

///////////////////////////////////////////////////////////////////////////////
#include "boost/spirit/phoenix/actor.hpp"
#include "boost/spirit/phoenix/composite.hpp"
#include "boost/static_assert.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace phoenix {

///////////////////////////////////////////////////////////////////////////////
//
//  Phoenix predefined maximum construct_ limit. This limit defines the maximum
//  number of parameters supported for calles to the set of construct_ template
//  functions (lazy object construction, see below). This number defaults to 3.
//  The actual maximum is rounded up in multiples of 3. Thus, if this value
//  is 4, the actual limit is 6. The ultimate maximum limit in this
//  implementation is 15.
//  PHOENIX_CONSTRUCT_LIMIT should NOT be greater than PHOENIX_LIMIT!

#if !defined(PHOENIX_CONSTRUCT_LIMIT)
#define PHOENIX_CONSTRUCT_LIMIT PHOENIX_LIMIT
#endif

// ensure PHOENIX_CONSTRUCT_LIMIT <= PHOENIX_LIMIT
BOOST_STATIC_ASSERT(PHOENIX_CONSTRUCT_LIMIT <= PHOENIX_LIMIT);

// ensure PHOENIX_CONSTRUCT_LIMIT <= 15
BOOST_STATIC_ASSERT(PHOENIX_CONSTRUCT_LIMIT <= 15);

///////////////////////////////////////////////////////////////////////////////
//
//  Lazy C++ casts
//
//      The set of lazy C++ cast template classes and functions provide a way
//      of lazily casting certain type to another during parsing.
//      The lazy C++ templates are (syntactically) used very much like
//      the well known C++ casts:
//
//          A *a = static_cast_<A *>(...actor returning a convertible type...);
//
//      where the given parameter should be an actor, which eval() function
//      returns a convertible type.
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename A>
struct static_cast_l {

    template <typename TupleT>
    struct result { typedef T type; };

    static_cast_l(A const& a_)
    :   a(a_) {}

    template <typename TupleT>
    T
    eval(TupleT const& args) const
    {
        return static_cast<T>(a.eval(args));
    }

    A a;
};

//////////////////////////////////
template <typename T, typename BaseAT>
inline actor<static_cast_l<T, BaseAT> >
static_cast_(actor<BaseAT> const& a)
{
    typedef static_cast_l<T, BaseAT> cast_t;
    return actor<cast_t>(cast_t(a));
}

//////////////////////////////////
template <typename T, typename A>
struct dynamic_cast_l {

    template <typename TupleT>
    struct result { typedef T type; };

    dynamic_cast_l(A const& a_)
    :   a(a_) {}

    template <typename TupleT>
    T
    eval(TupleT const& args) const
    {
        return dynamic_cast<T>(a.eval(args));
    }

    A a;
};

//////////////////////////////////
template <typename T, typename BaseAT>
inline actor<dynamic_cast_l<T, BaseAT> >
dynamic_cast_(actor<BaseAT> const& a)
{
    typedef dynamic_cast_l<T, BaseAT> cast_t;
    return actor<cast_t>(cast_t(a));
}

//////////////////////////////////
template <typename T, typename A>
struct reinterpret_cast_l {

    template <typename TupleT>
    struct result { typedef T type; };

    reinterpret_cast_l(A const& a_)
    :   a(a_) {}

    template <typename TupleT>
    T
    eval(TupleT const& args) const
    {
        return reinterpret_cast<T>(a.eval(args));
    }

    A a;
};

//////////////////////////////////
template <typename T, typename BaseAT>
inline actor<reinterpret_cast_l<T, BaseAT> >
reinterpret_cast_(actor<BaseAT> const& a)
{
    typedef reinterpret_cast_l<T, BaseAT> cast_t;
    return actor<cast_t>(cast_t(a));
}

//////////////////////////////////
template <typename T, typename A>
struct const_cast_l {

    template <typename TupleT>
    struct result { typedef T type; };

    const_cast_l(A const& a_)
    :   a(a_) {}

    template <typename TupleT>
    T
    eval(TupleT const& args) const
    {
        return const_cast<T>(a.eval(args));
    }

    A a;
};

//////////////////////////////////
template <typename T, typename BaseAT>
inline actor<const_cast_l<T, BaseAT> >
const_cast_(actor<BaseAT> const& a)
{
    typedef const_cast_l<T, BaseAT> cast_t;
    return actor<cast_t>(cast_t(a));
}

///////////////////////////////////////////////////////////////////////////////
//
//  construct_
//
//      Lazy object construction
//
//      The set of construct_<> template classes and functions provide a way
//      of lazily constructing certain object from a arbitrary set of
//      actors during parsing.
//      The construct_ templates are (syntactically) used very much like
//      the well known C++ casts:
//
//          A a = construct_<A>(...arbitrary list of actors...);
//
//      where the given parameters are submitted as parameters to the
//      contructor of the object of type A. (This certainly implies, that
//      type A has a constructor with a fitting set of parameter types
//      defined.)
//
//      The maximum number of needed parameters is controlled through the
//      preprocessor constant PHOENIX_CONSTRUCT_LIMIT. Note though, that this
//      limit should not be greater than PHOENIX_LIMIT.
//
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct construct_l_0 {
    typedef T result_type;

    T operator()() const {
        return T();
    }
};


template <typename T>
struct construct_l {

    template <
            typename A
        ,   typename B
        ,   typename C

#if PHOENIX_CONSTRUCT_LIMIT > 3
        ,   typename D
        ,   typename E
        ,   typename F

#if PHOENIX_CONSTRUCT_LIMIT > 6
        ,   typename G
        ,   typename H
        ,   typename I

#if PHOENIX_CONSTRUCT_LIMIT > 9
        ,   typename J
        ,   typename K
        ,   typename L

#if PHOENIX_CONSTRUCT_LIMIT > 12
        ,   typename M
        ,   typename N
        ,   typename O
#endif
#endif
#endif
#endif
    >
    struct result { typedef T type; };

    T operator()() const {
        return T();
    }

    template <typename A>
    T operator()(A const& a) const {
        return T(a);
    }

    template <typename A, typename B>
    T operator()(A const& a, B const& b) const {
        return T(a, b);
    }

    template <typename A, typename B, typename C>
    T operator()(A const& a, B const& b, C const& c) const {
        return T(a, b, c);
    }

#if PHOENIX_CONSTRUCT_LIMIT > 3
    template <
        typename A, typename B, typename C, typename D
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d) const
    {
        return T(a, b, c, d);
    }

    template <
        typename A, typename B, typename C, typename D, typename E
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e) const
    {
        return T(a, b, c, d, e);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f) const
    {
        return T(a, b, c, d, e, f);
    }

#if PHOENIX_CONSTRUCT_LIMIT > 6
    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g) const
    {
        return T(a, b, c, d, e, f, g);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h) const
    {
        return T(a, b, c, d, e, f, g, h);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i) const
    {
        return T(a, b, c, d, e, f, g, h, i);
    }

#if PHOENIX_CONSTRUCT_LIMIT > 9
    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j) const
    {
        return T(a, b, c, d, e, f, g, h, i, j);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k) const
    {
        return T(a, b, c, d, e, f, g, h, i, j, k);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l) const
    {
        return T(a, b, c, d, e, f, g, h, i, j, k, l);
    }

#if PHOENIX_CONSTRUCT_LIMIT > 12
    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m) const
    {
        return T(a, b, c, d, e, f, g, h, i, j, k, l, m);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M, typename N
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m, N const& n) const
    {
        return T(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
    }

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M, typename N, typename O
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m, N const& n, O const& o) const
    {
        return T(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o);
    }

#endif
#endif
#endif
#endif
};


template <typename T>
struct construct_1 {

    template <
            typename A
    >
    struct result { typedef T type; };

    template <typename A>
    T operator()(A const& a) const {
        return T(a);
    }

};

template <typename T>
struct construct_2 {

    template <
            typename A
        ,   typename B
    >
    struct result { typedef T type; };

    template <typename A, typename B>
    T operator()(A const& a, B const& b) const {
        return T(a, b);
    }

};

template <typename T>
struct construct_3 {

    template <
            typename A
        ,   typename B
        ,   typename C
    >
    struct result { typedef T type; };

    template <typename A, typename B, typename C>
    T operator()(A const& a, B const& b, C const& c) const {
        return T(a, b, c);
    }
};

#if PHOENIX_CONSTRUCT_LIMIT > 3
template <typename T>
struct construct_4 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
    >
    struct result { typedef T type; };


    template <
        typename A, typename B, typename C, typename D
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d) const
    {
        return T(a, b, c, d);
    }
};


template <typename T>
struct construct_5 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
    >
    struct result { typedef T type; };

    template <
        typename A, typename B, typename C, typename D, typename E
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e) const
    {
        return T(a, b, c, d, e);
    }
};


template <typename T>
struct construct_6 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
    >
    struct result { typedef T type; };

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f) const
    {
        return T(a, b, c, d, e, f);
    }
};
#endif


#if PHOENIX_CONSTRUCT_LIMIT > 6
template <typename T>
struct construct_7 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
    >
    struct result { typedef T type; };

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g) const
    {
        return T(a, b, c, d, e, f, g);
    }
};

template <typename T>
struct construct_8 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
    >
    struct result { typedef T type; };

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h) const
    {
        return T(a, b, c, d, e, f, g, h);
    }
};

template <typename T>
struct construct_9 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
    >
    struct result { typedef T type; };

    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i) const
    {
        return T(a, b, c, d, e, f, g, h, i);
    }
};
#endif


#if PHOENIX_CONSTRUCT_LIMIT > 9
template <typename T>
struct construct_10 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
        ,   typename J
    >
    struct result { typedef T type; };


    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j) const
    {
        return T(a, b, c, d, e, f, g, h, i, j);
    }
};

template <typename T>
struct construct_11 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
        ,   typename J
        ,   typename K
    >
    struct result { typedef T type; };


    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k) const
    {
        return T(a, b, c, d, e, f, g, h, i, j, k);
    }

};

template <typename T>
struct construct_12 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
        ,   typename J
        ,   typename K
        ,   typename L
    >
    struct result { typedef T type; };


    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l) const
    {
        return T(a, b, c, d, f, e, g, h, i, j, k, l);
    }
};
#endif

#if PHOENIX_CONSTRUCT_LIMIT > 12
template <typename T>
struct construct_13 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
        ,   typename J
        ,   typename K
        ,   typename L
        ,   typename M
    >
    struct result { typedef T type; };


    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m) const
    {
        return T(a, b, c, d, e, f, g, h, i, j, k, l, m);
    }
};

template <typename T>
struct construct_14 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
        ,   typename J
        ,   typename K
        ,   typename L
        ,   typename M
        ,   typename N
    >
    struct result { typedef T type; };


    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M, typename N
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m, N const& n) const
    {
        return T(a, b, c, d, e, f, g, h, i, j, k, l, m, n);
    }

};

template <typename T>
struct construct_15 {

    template <
            typename A
        ,   typename B
        ,   typename C
        ,   typename D
        ,   typename E
        ,   typename F
        ,   typename G
        ,   typename H
        ,   typename I
        ,   typename J
        ,   typename K
        ,   typename L
        ,   typename M
        ,   typename N
        ,   typename O
    >
    struct result { typedef T type; };


    template <
        typename A, typename B, typename C, typename D, typename E,
        typename F, typename G, typename H, typename I, typename J,
        typename K, typename L, typename M, typename N, typename O
    >
    T operator()(
        A const& a, B const& b, C const& c, D const& d, E const& e,
        F const& f, G const& g, H const& h, I const& i, J const& j,
        K const& k, L const& l, M const& m, N const& n, O const& o) const
    {
        return T(a, b, c, d, f, e, g, h, i, j, k, l, m, n, o);
    }

};
#endif


#if defined(__BORLANDC__) || (defined(__MWERKS__) && (__MWERKS__ <= 0x3002))

///////////////////////////////////////////////////////////////////////////////
//
//  The following specializations are needed because Borland and CodeWarrior
//  does not accept default template arguments in nested template classes in
//  classes (i.e construct_l::result)
//
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename TupleT>
struct composite0_result<construct_l_0<T>, TupleT> {

    typedef T type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A>
struct composite1_result<construct_l<T>, TupleT, A> {

    typedef T type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B>
struct composite2_result<construct_l<T>, TupleT, A, B> {

    typedef T type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C>
struct composite3_result<construct_l<T>, TupleT, A, B, C> {

    typedef T type;
};

#if PHOENIX_LIMIT > 3
//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D>
struct composite4_result<construct_l<T>, TupleT,
    A, B, C, D> {

    typedef T type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E>
struct composite5_result<construct_l<T>, TupleT,
    A, B, C, D, E> {

    typedef T type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F>
struct composite6_result<construct_l<T>, TupleT,
    A, B, C, D, E, F> {

    typedef T type;
};

#if PHOENIX_LIMIT > 6
//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G>
struct composite7_result<construct_l<T>, TupleT,
    A, B, C, D, E, F, G> {

    typedef T type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H>
struct composite8_result<construct_l<T>, TupleT,
    A, B, C, D, E, F, G, H> {

    typedef T type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I>
struct composite9_result<construct_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I> {

    typedef T type;
};

#if PHOENIX_LIMIT > 9
//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J>
struct composite10_result<construct_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I, J> {

    typedef T type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K>
struct composite11_result<construct_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K> {

    typedef T type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L>
struct composite12_result<construct_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K, L> {

    typedef T type;
};

#if PHOENIX_LIMIT > 12
//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M>
struct composite13_result<construct_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K, L, M> {

    typedef T type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N>
struct composite14_result<construct_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N> {

    typedef T type;
};

//////////////////////////////////
template <typename T, typename TupleT,
    typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J,
    typename K, typename L, typename M, typename N, typename O>
struct composite15_result<construct_l<T>, TupleT,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O> {

    typedef T type;
};

#endif
#endif
#endif
#endif
#endif

//////////////////////////////////
template <typename T>
inline typename impl::make_composite<construct_l_0<T> >::type
construct_()
{
    typedef impl::make_composite<construct_l_0<T> > make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(construct_l_0<T>()));
}

//////////////////////////////////
template <typename T, typename A>
inline typename impl::make_composite<construct_1<T>, A>::type
construct_(A const& a)
{
    typedef impl::make_composite<construct_1<T>, A> make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(construct_1<T>(), a));
}

//////////////////////////////////
template <typename T, typename A, typename B>
inline typename impl::make_composite<construct_2<T>, A, B>::type
construct_(A const& a, B const& b)
{
    typedef impl::make_composite<construct_2<T>, A, B> make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(construct_2<T>(), a, b));
}

//////////////////////////////////
template <typename T, typename A, typename B, typename C>
inline typename impl::make_composite<construct_3<T>, A, B, C>::type
construct_(A const& a, B const& b, C const& c)
{
    typedef impl::make_composite<construct_3<T>, A, B, C> make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(construct_3<T>(), a, b, c));
}

#if PHOENIX_CONSTRUCT_LIMIT > 3
//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D
>
inline typename impl::make_composite<construct_4<T>, A, B, C, D>::type
construct_(
    A const& a, B const& b, C const& c, D const& d)
{
    typedef
        impl::make_composite<construct_4<T>, A, B, C, D>
        make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(construct_4<T>(), a, b, c, d));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E
>
inline typename impl::make_composite<construct_5<T>, A, B, C, D, E>::type
construct_(
    A const& a, B const& b, C const& c, D const& d, E const& e)
{
    typedef
        impl::make_composite<construct_5<T>, A, B, C, D, E>
        make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(construct_5<T>(), a, b, c, d, e));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F
>
inline typename impl::make_composite<construct_6<T>, A, B, C, D, E, F>::type
construct_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f)
{
    typedef
        impl::make_composite<construct_6<T>, A, B, C, D, E, F>
        make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(construct_6<T>(), a, b, c, d, e, f));
}

#if PHOENIX_CONSTRUCT_LIMIT > 6
//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G
>
inline typename impl::make_composite<construct_7<T>, A, B, C, D, E, F, G>::type
construct_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g)
{
    typedef
        impl::make_composite<construct_7<T>, A, B, C, D, E, F, G>
        make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(
            construct_7<T>(), a, b, c, d, e, f, g));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H
>
inline typename impl::make_composite<construct_8<T>, A, B, C, D, E, F, G, H>::type
construct_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h)
{
    typedef
        impl::make_composite<construct_8<T>, A, B, C, D, E, F, G, H>
        make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(
            construct_8<T>(), a, b, c, d, e, f, g, h));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I
>
inline typename impl::make_composite<construct_9<T>, A, B, C, D, E, F, G, H, I>::type
construct_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i)
{
    typedef
        impl::make_composite<construct_9<T>, A, B, C, D, E, F, G, H, I>
        make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(
            construct_9<T>(), a, b, c, d, e, f, g, h, i));
}

#if PHOENIX_CONSTRUCT_LIMIT > 9
//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J
>
inline typename impl::make_composite<
    construct_10<T>, A, B, C, D, E, F, G, H, I, J>::type
construct_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j)
{
    typedef
        impl::make_composite<
            construct_10<T>, A, B, C, D, E, F, G, H, I, J
        >
        make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(
            construct_10<T>(), a, b, c, d, e, f, g, h, i, j));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J, typename K
>
inline typename impl::make_composite<
    construct_11<T>, A, B, C, D, E, F, G, H, I, J, K>::type
construct_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k)
{
    typedef
        impl::make_composite<
            construct_11<T>, A, B, C, D, E, F, G, H, I, J, K
        >
        make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(
            construct_11<T>(), a, b, c, d, e, f, g, h, i, j, k));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J, typename K,
    typename L
>
inline typename impl::make_composite<
    construct_12<T>, A, B, C, D, E, F, G, H, I, J, K, L>::type
construct_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k, L const& l)
{
    typedef
        impl::make_composite<
            construct_12<T>, A, B, C, D, E, F, G, H, I, J, K, L
        >
        make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(
            construct_12<T>(), a, b, c, d, e, f, g, h, i, j, k, l));
}

#if PHOENIX_CONSTRUCT_LIMIT > 12
//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J, typename K,
    typename L, typename M
>
inline typename impl::make_composite<
    construct_13<T>, A, B, C, D, E, F, G, H, I, J, K, L, M>::type
construct_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k, L const& l, M const& m)
{
    typedef
        impl::make_composite<
            construct_13<T>, A, B, C, D, E, F, G, H, I, J, K, L, M
        >
        make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(
            construct_13<T>(), a, b, c, d, e, f, g, h, i, j, k, l, m));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J, typename K,
    typename L, typename M, typename N
>
inline typename impl::make_composite<
    construct_14<T>, A, B, C, D, E, F, G, H, I, J, K, L, M>::type
construct_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k, L const& l, M const& m, N const& n)
{
    typedef
        impl::make_composite<
            construct_14<T>, A, B, C, D, E, F, G, H, I, J, K, L, M, N
        >
        make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(
            construct_14<T>(), a, b, c, d, e, f, g, h, i, j, k, l, m, n));
}

//////////////////////////////////
template <
    typename T, typename A, typename B, typename C, typename D, typename E,
    typename F, typename G, typename H, typename I, typename J, typename K,
    typename L, typename M, typename N, typename O
>
inline typename impl::make_composite<
    construct_15<T>, A, B, C, D, E, F, G, H, I, J, K, L, M, O>::type
construct_(
    A const& a, B const& b, C const& c, D const& d, E const& e,
    F const& f, G const& g, H const& h, I const& i, J const& j,
    K const& k, L const& l, M const& m, N const& n, O const& o)
{
    typedef
        impl::make_composite<
            construct_15<T>, A, B, C, D, E, F, G, H, I, J, K, L, M, N, O
        >
        make_composite_t;

    return make_composite_t::type(
        make_composite_t::composite_type(
            construct_15<T>(), a, b, c, d, e, f, g, h, i, j, k, l, m, n, o));
}

#endif
#endif
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
}   //  namespace phoenix

#endif // PHOENIX_CASTS_HPP
