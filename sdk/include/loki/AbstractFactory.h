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

#ifndef ABSTRACTFACTORY_INC_
#define ABSTRACTFACTORY_INC_

#include "Typelist.h"
#include "TypeManip.h"
#include "HierarchyGenerators.h"

#include <cassert>

namespace Loki
{

////////////////////////////////////////////////////////////////////////////////
// class template AbstractFactoryUnit
// The building block of an Abstract Factory
////////////////////////////////////////////////////////////////////////////////

    template <class T>
    class AbstractFactoryUnit
    {
    public:
        virtual T* DoCreate(Type2Type<T>) = 0;
        virtual ~AbstractFactoryUnit() {}
    };

////////////////////////////////////////////////////////////////////////////////
// class template AbstractFactory
// Defines an Abstract Factory interface starting from a typelist
////////////////////////////////////////////////////////////////////////////////

    template
    <
        class TList,
        template <class> class Unit = AbstractFactoryUnit
    >
    class AbstractFactory : public GenScatterHierarchy<TList, Unit>
    {
    public:
        typedef TList ProductList;
        
        template <class T> T* Create()
        {
            Unit<T>& unit = *this;
            return unit.DoCreate(Type2Type<T>());
        }
    };
    
////////////////////////////////////////////////////////////////////////////////
// class template OpNewFactoryUnit
// Creates an object by invoking the new operator
////////////////////////////////////////////////////////////////////////////////

    template <class ConcreteProduct, class Base>
    class OpNewFactoryUnit : public Base
    {
        typedef typename Base::ProductList BaseProductList;
    
    protected:
        typedef typename BaseProductList::Tail ProductList;
    
    public:
        typedef typename BaseProductList::Head AbstractProduct;
        ConcreteProduct* DoCreate(Type2Type<AbstractProduct>)
        {
            return new ConcreteProduct;
        }
    };

////////////////////////////////////////////////////////////////////////////////
// class template PrototypeFactoryUnit
// Creates an object by cloning a prototype
// There is a difference between the implementation herein and the one described
//     in the book: GetPrototype and SetPrototype use the helper friend 
//     functions DoGetPrototype and DoSetPrototype. The friend functions avoid
//     name hiding issues. Plus, GetPrototype takes a reference to pointer
//     instead of returning the pointer by value.
////////////////////////////////////////////////////////////////////////////////

    template <class ConcreteProduct, class Base>
    class PrototypeFactoryUnit : public Base
    {
        typedef typename Base::ProductList BaseProductList;
    
    protected:
        typedef typename BaseProductList::Tail ProductList;

    public:
        typedef typename BaseProductList::Head AbstractProduct;

        PrototypeFactoryUnit(AbstractProduct* p = 0)
            : pPrototype_(p)
        {}
        
        friend void DoGetPrototype(const PrototypeFactoryUnit& me,
            AbstractProduct*& pPrototype)
        { pPrototype = me.pPrototype_; }
        
        friend void DoSetPrototype(PrototypeFactoryUnit& me, 
            AbstractProduct* pObj)
        { me.pPrototype_ = pObj; }
        
        template <class U>
        void GetPrototype(AbstractProduct*& p)
        { return DoGetPrototype(*this, p); }
        
        template <class U>
        void SetPrototype(U* pObj)
        { DoSetPrototype(*this, pObj); }
        
        AbstractProduct* DoCreate(Type2Type<AbstractProduct>)
        {
            assert(pPrototype_);
            return pPrototype_->Clone();
        }
        
    private:
        AbstractProduct* pPrototype_;
    };

////////////////////////////////////////////////////////////////////////////////
// class template ConcreteFactory
// Implements an AbstractFactory interface
////////////////////////////////////////////////////////////////////////////////

    template
    <
        class AbstractFact,
        template <class, class> class Creator = OpNewFactoryUnit,
        class TList = typename AbstractFact::ProductList
    >
    class ConcreteFactory
        : public GenLinearHierarchy<
            typename TL::Reverse<TList>::Result, Creator, AbstractFact>
    {
    public:
        typedef typename AbstractFact::ProductList ProductList;
        typedef TList ConcreteProductList;
    };

} // namespace Loki

////////////////////////////////////////////////////////////////////////////////
// Change log:
// June 20, 2001: ported by Nick Thurn to gcc 2.95.3. Kudos, Nick!!!
// May  10, 2002: ported by Rani Sharoni to VC7 (RTM - 9466)
////////////////////////////////////////////////////////////////////////////////

#endif // ABSTRACTFACTORY_INC_
