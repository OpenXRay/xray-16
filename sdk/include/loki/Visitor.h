////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design 
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any 
//     purpose is hereby granted without fee, provided that the above copyright 
//     notice appear in all copies and that both that copyright notice and this 
//     permission notice appear in supporting documentation.
// The author or Addison-Welsey Longman make no representations about the 
//     suitability of this software for any purpose. It is provided "as is" 
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

// Last update: May 19, 2002

#ifndef VISITOR_INC_
#define VISITOR_INC_

#include "Typelist.h"
#include "HierarchyGenerators.h"

namespace Loki
{

////////////////////////////////////////////////////////////////////////////////
// class template BaseVisitor
// The base class of any Acyclic Visitor
////////////////////////////////////////////////////////////////////////////////

    class BaseVisitor
    {
    public:
        virtual ~BaseVisitor() {}
    };
    
////////////////////////////////////////////////////////////////////////////////
// class template Visitor
// Forward decleration
////////////////////////////////////////////////////////////////////////////////

    template <class T, typename R = void>
    class Visitor;

    namespace Private
    {
    // for some reason VC7 needs the base definition altough not in use
    template <typename TListTag> 
    struct VisitorHelper1
    {
        template <typename T, typename R>
        struct In 
        { 
            typedef typename T::ERROR_THIS_INSTANCE_SELECTED Result; 
        };
    };
    
    template <typename TListTag> 
    struct VisitorHelper2
    {
        template <typename T, typename R>
        struct In 
        { 
            typedef typename T::ERROR_THIS_INSTANCE_SELECTED Result; 
        };
    };
    
    template <> 
    struct VisitorHelper1<TL::Typelist_tag>
    {
        template <class TList, typename R>
        struct In
        {
            typedef Visitor<typename TList::Head, R> Result;
        };
    };
    
    template <> 
    struct VisitorHelper2<TL::Typelist_tag>
    {
        template <class TList, typename R>
        struct In
        {
        private:
            template<typename Tail>
            struct In1
            {
                typedef Visitor<typename TList::Tail, R> Result;
            };

            template<>
            struct In1<NullType>
            {
                struct Result {};
            };

        public:
            typedef typename In1<typename TList::Tail>::Result Result;
        };
    };
    
    
    template <> 
    struct VisitorHelper1<TL::NoneList_tag>
    {
        template <class T, typename R>
        struct In 
        { 
            struct Result
            {
                typedef R ReturnType;
                virtual ReturnType Visit(T&) = 0;
            };
        };
    };
    
    template <> 
    struct VisitorHelper2<TL::NoneList_tag>
    {
        template <class T, typename R>
        struct In { struct Result {}; };        
    };
    
    } // namespace Private

////////////////////////////////////////////////////////////////////////////////
// class template Visitor
// The building block of Acyclic Visitor
////////////////////////////////////////////////////////////////////////////////

    template <class T, typename R>
    class Visitor
        : public Private::VisitorHelper1
          <
            typename TL::is_Typelist<T>::type_tag
          >
          ::template In<T, R>::Result

        , public Private::VisitorHelper2
          <
            typename TL::is_Typelist<T>::type_tag
          >
          ::template In<T, R>::Result
    {  
    public:
        typedef R ReturnType;
    };


////////////////////////////////////////////////////////////////////////////////
// class template BaseVisitorImpl
// Implements non-strict visitation (you can implement only part of the Visit
//     functions)
////////////////////////////////////////////////////////////////////////////////
    template <class TList, typename R = void> 
    class BaseVisitorImpl;

    namespace Private
    {
    // for some reason VC7 needs the base definition altough not in use
    template<typename TListTag> 
    struct BaseVisitorImplHelper
    {
        template <typename T, typename R>
        struct In 
        { 
            typedef typename T::ERROR_THIS_INSTANCE_SELECTED Result; 
        };
    };
    
    template<> 
    struct BaseVisitorImplHelper<TL::Typelist_tag>
    {
        template <typename TList, typename R>
        struct In 
        { 
            typedef BaseVisitorImpl<TList, R> Result; 
        };
    };

    template<> 
    struct BaseVisitorImplHelper<TL::NullType_tag>
    {
        template <typename TList, typename R>
        struct In 
        { 
            struct Result {}; 
        };
    };

    } // namespace Private
    
    template <class TList, typename R>
    class BaseVisitorImpl
        : public Visitor<typename TList::Head, R>
        
        , public Private::BaseVisitorImplHelper
          <
            typename TL::is_Typelist<typename TList::Tail>::type_tag
          >
          ::template In<typename TList::Tail, R>::Result
    {
        ASSERT_TYPELIST(TList);

    public:
        // using BaseVisitorImpl<Tail, R>::Visit;

        virtual R Visit(typename TList::Head&)
        { return R(); }
    };
    
////////////////////////////////////////////////////////////////////////////////
// class template NonStrictVisitor
// Implements non-strict visitation (you can implement only part of the Visit
//     functions)
////////////////////////////////////////////////////////////////////////////////

    template <class T, class Base>
    struct NonStrictVisitorUnit : public Base
    {
        typedef typename Base::ReturnType ReturnType;
        ReturnType Visit(T&)
        {
            return ReturnType();
        }
    };

    template <class TList, typename R = void> 
    class NonStrictVisitor 
        : public GenLinearHierarchy<
            TList, 
            NonStrictVisitorUnit, 
            Visitor<TList, R> >
    {
    };

////////////////////////////////////////////////////////////////////////////////
// class template BaseVisitable
////////////////////////////////////////////////////////////////////////////////

template <typename R, typename Visited>
struct DefaultCatchAll
{
    static R OnUnknownVisitor(Visited&, BaseVisitor&)
    { return R(); }
};

////////////////////////////////////////////////////////////////////////////////
// class template BaseVisitable
////////////////////////////////////////////////////////////////////////////////

    template 
    <
        typename R = void, 
        template <typename, class> class CatchAll = DefaultCatchAll
    >
    class BaseVisitable
    {
    public:
        typedef R ReturnType;
        virtual ~BaseVisitable() {}
        virtual ReturnType Accept(BaseVisitor&) = 0;
        
    protected: // give access only to the hierarchy
        template <class T>
        static ReturnType AcceptImpl(T& visited, BaseVisitor& guest)
        {
            // Apply the Acyclic Visitor
            if (Visitor<T,R>* p = dynamic_cast<Visitor<T,R>*>(&guest))
            {
                return p->Visit(visited);
            }
            return CatchAll<R, T>::OnUnknownVisitor(visited, guest);
        }
    };

////////////////////////////////////////////////////////////////////////////////
// macro DEFINE_VISITABLE
// Put it in every class that you want to make visitable (in addition to 
//     deriving it from BaseVisitable<R>
////////////////////////////////////////////////////////////////////////////////

#define DEFINE_VISITABLE() \
    virtual ReturnType Accept(BaseVisitor& guest) \
    { return AcceptImpl(*this, guest); }

////////////////////////////////////////////////////////////////////////////////
// class template CyclicVisitor
// Put it in every class that you want to make visitable (in addition to 
//     deriving it from BaseVisitable<R>
////////////////////////////////////////////////////////////////////////////////

    template <typename R, class TList>
    class CyclicVisitor : public Visitor<TList, R>
    {
    public:
        typedef R ReturnType;
        // using Visitor<TList, R>::Visit;
        
        virtual ~CyclicVisitor() {}

        template <class Visited>
        ReturnType GenericVisit(Visited& host)
        {
            Visitor<Visited, ReturnType>& subObj = *this;
            return subObj.Visit(host);
        }
    };
    
////////////////////////////////////////////////////////////////////////////////
// macro DEFINE_CYCLIC_VISITABLE
// Put it in every class that you want to make visitable by a cyclic visitor
////////////////////////////////////////////////////////////////////////////////

#define DEFINE_CYCLIC_VISITABLE(SomeVisitor) \
    virtual SomeVisitor::ReturnType Accept(SomeVisitor& guest) \
    { return guest.GenericVisit(*this); }
} // namespace Loki

////////////////////////////////////////////////////////////////////////////////
// Change log:
// March 20: add default argument DefaultCatchAll to BaseVisitable
// June 20, 2001: ported by Nick Thurn to gcc 2.95.3. Kudos, Nick!!!
////////////////////////////////////////////////////////////////////////////////

#endif // VISITOR_INC_

