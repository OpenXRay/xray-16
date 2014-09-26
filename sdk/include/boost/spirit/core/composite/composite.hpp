/*=============================================================================
    Spirit v1.6.0
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Permission to copy, use, modify, sell and distribute this software is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty, and
    with no claim as to its suitability for any purpose.
=============================================================================*/
#if !defined(BOOST_SPIRIT_COMPOSITE_HPP)
#define BOOST_SPIRIT_COMPOSITE_HPP

///////////////////////////////////////////////////////////////////////////////
#include "boost/spirit/core/composite/impl/composite.ipp"

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit {

    ///////////////////////////////////////////////////////////////////////////
    //
    //  unary class.
    //
    //      Composite class composed of a single subject. This template class
    //      is parameterized by the subject type S and a base class to
    //      inherit from, BaseT. The unary class is meant to be a base class
    //      to inherit from. The inheritance structure, given the BaseT
    //      template parameter places the unary class in the middle of a
    //      linear, single parent hierarchy. For instance, given a class S
    //      and a base class B, a class D can derive from unary:
    //
    //          struct D : public unary<S, B> {...};
    //
    //      The inheritance structure is thus:
    //
    //            B
    //            |
    //          unary (has S)
    //            |
    //            D
    //
    //      The subject can be accessed from the derived class D as:
    //      this->subject();
    //
    //      Typically, the subject S is specified as typename S::embed_t.
    //      embed_t specifies how the subject is embedded in the composite
    //      (See parser.hpp for details).
    //
    ///////////////////////////////////////////////////////////////////////////
    template <typename S, typename BaseT>
    class unary : public impl::subject<typename S::embed_t, BaseT>
    {
        typedef impl::subject<typename S::embed_t, BaseT>   base_t;
        typedef typename base_t::param_t                    param_t;
        typedef typename base_t::return_t                   return_t;

    public:

        typedef S                       subject_t;
        typedef typename S::embed_t     subject_embed_t;

        unary()
        : base_t() {}

        unary(BaseT const& base)
        : base_t(base) {}

        unary(param_t s)
        : base_t(s) {}

        unary(BaseT const& base, param_t s)
        : base_t(base, s) {}

        return_t
        subject() const
        { return base_t::get(); }
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    //  binary class.
    //
    //      Composite class composed of a pair (left and right). This
    //      template class is parameterized by the left and right subject
    //      types A and B and a base class to inherit from, BaseT. The binary
    //      class is meant to be a base class to inherit from. The
    //      inheritance structure, given the BaseT template parameter places
    //      the binary class in the middle of a linear, single parent
    //      hierarchy. For instance, given classes X and Y and a base class
    //      B, a class D can derive from binary:
    //
    //          struct D : public binary<X, Y, B> {...};
    //
    //      The inheritance structure is thus:
    //
    //            B
    //            |
    //          binary (has X and Y)
    //            |
    //            D
    //
    //      The left and right subjects can be accessed from the derived
    //      class D as: this->left(); and this->right();
    //
    //      Typically, the pairs X and Y are specified as typename X::embed_t
    //      and typename Y::embed_t. embed_t specifies how the subject is
    //      embedded in the composite (See parser.hpp for details).
    //
    ///////////////////////////////////////////////////////////////////////////////
    template <typename A, typename B, typename BaseT>
    class binary
    : public
        impl::left_subject<typename A::embed_t,
        impl::right_subject<typename B::embed_t, BaseT> >
    {
        typedef
            impl::left_subject<typename A::embed_t,
            impl::right_subject<typename B::embed_t, BaseT> >   left_base_t;
        typedef typename left_base_t::param_t                   left_param_t;
        typedef typename left_base_t::return_t                  left_return_t;

        typedef impl::right_subject<typename B::embed_t, BaseT> right_base_t;
        typedef typename right_base_t::param_t                  right_param_t;
        typedef typename right_base_t::return_t                 right_return_t;

    public:

        typedef A                       left_t;
        typedef typename A::embed_t     left_embed_t;

        typedef B                       right_t;
        typedef typename B::embed_t     right_embed_t;

        binary()
        : left_base_t(right_base_t()) {}

        binary(left_param_t a, right_param_t b)
        : left_base_t(right_base_t(b), a) {}

        left_return_t
        left() const
        { return left_base_t::left(); }

        right_return_t
        right() const
        { return right_base_t::right(); }
    };

}} // namespace boost::spirit

#endif
