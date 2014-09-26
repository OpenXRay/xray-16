////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2001, 2002 by Andrei Alexandrescu
// Permission to use, copy, modify, distribute and sell this software for any 
//     purpose is hereby granted without fee, provided that the above copyright 
//     notice appear in all copies and that both that copyright notice and this 
//     permission notice appear in supporting documentation.
// The author makes no representations about the suitability of this software 
//     for any purpose. It is provided "as is" 
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

#ifndef VARIANT_INC_
#define VARIANT_INC_

#include <cstddef>
#include <memory>
#include <typeinfo>
#include "Visitor.h"
#include "Typelist.h"
#include "static_check.h"

//
// At the moment there is no namespace for Variant
//

#ifdef _MSC_VER
# include "VC_Alignment.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// class template ConfigurableUnion
// Builds a union that contains each type in a typelist
// Usage: ConfigurableUnion<TList> is the very type
////////////////////////////////////////////////////////////////////////////////

template <class U> union ConfigurableUnion;

template <> union ConfigurableUnion< ::Loki::NullType >
{
};

template <class TList> 
union ConfigurableUnion
{
private:
    ASSERT_TYPELIST(TList);

    typedef typename TList::Head Head;
    typedef typename TList::Tail Tail;

public:
   Head head_;
   ConfigurableUnion<Tail> tail_;
};


////////////////////////////////////////////////////////////////////////////////
// class template MaxSize
// Computes the maximum sizeof for all types in a typelist
// Usage: MaxSize<TList>::result
////////////////////////////////////////////////////////////////////////////////

template <class TList> struct MaxSize;

template <> 
struct MaxSize< ::Loki::NullType >
{
    enum { result = 0 };
};

template <class TList>
struct MaxSize
{
private:
    ASSERT_TYPELIST(TList);

    typedef typename TList::Head Head;
    typedef typename TList::Tail Tail;

private:
    enum { headResult = sizeof(Head)          };
    enum { tailResult = MaxSize<Tail>::result };

public:
    enum { result = headResult > tailResult ? 
           headResult : tailResult };
};


////////////////////////////////////////////////////////////////////////////////
// class AlignedPODBase
// Defines a host of protected types used by AlignedPOD (defined later)
// Could be just part of AlignedPOD itself, but making it separate ought to 
// reduce compile times 
////////////////////////////////////////////////////////////////////////////////

class AlignedPODBase
{
protected:
    template <class TList, std::size_t size> 
    struct ComputeAlignBound
    {
    private:
        ASSERT_TYPELIST(TList);
    
        typedef typename TList::Head Head;
        typedef typename TList::Tail Tail;
    
    private:
        template<class TList1>
        struct In
        {
        private:
            typedef typename TList1::Head Head1;
            typedef typename TList1::Tail Tail1;
    
            typedef typename ComputeAlignBound<Tail1, size>::Result TailResult;
    
        public:
            typedef typename ::Loki::Select
            <
                sizeof(Head1) <= size,
                ::Loki::Typelist<Head1, TailResult>,
                TailResult
            >
            ::Result Result;
        };

        template<>
        struct In< ::Loki::NullType >
        {
            typedef ::Loki::NullType Result;
        };

    public:
        typedef typename In<TList>::Result Result;
    };

    template <typename U> struct Structify
    { U dummy_; };
    
    class Unknown;

    // VC7: fatal error C1067: compiler limit :
    // debug information module size exceeded
    // Therfore I decreased the list to 26 without 
    // changing the rage of detectable alignment
    typedef TYPELIST_26(
            char,
            wchar_t,
            short int,
            int,
            long int,
            float,
            double,
            long double,
            char*,
            void*,
            Unknown (*)(Unknown),
            Unknown* Unknown::*,
            Unknown (Unknown::*)(Unknown),
            Structify<char>, 
            Structify<wchar_t>, 
            Structify<short int>, 
            Structify<int>,
            Structify<long int>,
            Structify<float>,
            Structify<double>,
            Structify<long double>,
            Structify<char*>,
            Structify<void*>,
            Structify<Unknown (*)(Unknown)>,
            Structify<Unknown* Unknown::*>,
            Structify<Unknown (Unknown::*)(Unknown)>
            )
        TypesOfAllAlignments;
};

////////////////////////////////////////////////////////////////////////////////
// class template AlignedPOD
// Computes the alignment of all types in a typelist
// Usage: ConfigurableUnion<TList> is the very type
////////////////////////////////////////////////////////////////////////////////

template <typename TList>
class AlignedPOD : private AlignedPODBase
{
    enum { maxSize = MaxSize<TList>::result };

    typedef typename ComputeAlignBound
    <
        TypesOfAllAlignments, 
        maxSize
    >
    ::Result AlignTypes;

public:
    typedef ConfigurableUnion<AlignTypes> Result;
};

////////////////////////////////////////////////////////////////////////////////
// class template MakeConst
// Given a typelist TList, returns a typelist that contains the types in TList 
// adding a const qualifier to each.
// Usage: MakeConst<TList>::Result 
////////////////////////////////////////////////////////////////////////////////

template <class TList> struct MakeConst;

template <> struct MakeConst< ::Loki::NullType >
{
    typedef ::Loki::NullType Result; // terminator is not const
};

template <class TList>
struct MakeConst
{
private:
    ASSERT_TYPELIST(TList);

    typedef typename TList::Head Head;
    typedef typename TList::Tail Tail;
    
private:
    typedef typename MakeConst<Tail>::Result NewTail;

public:
    typedef ::Loki::Typelist<const Head, NewTail> Result; 
};

////////////////////////////////////////////////////////////////////////////////
// class template Converter
// Supports the Variant-to-Variant conversion constructor
// Guaranteed to issue an internal compiler error on:
//      1. Metrowerks CodeWarrior 7.0 (internal compiler error: File: 
//          'CTemplateTools.c' Line: 1477 
//          Variant.h line 244           UnitBase > VisitorBase;)
//      2. Microsoft Visual C++ 7.1 alpha release (Assertion failed: 
//          ( name - nameBuf ) < LIMIT_ID_LENGTH, 
//          file f:\vs70builds\2108\vc\Compiler\CxxFE\sl\P1\C\outdname.c, 
//          line 4583)
//      3. GNU gcc 2.95.3-6 (Internal compiler error 980422)
////////////////////////////////////////////////////////////////////////////////

template <class VariantFrom, class VariantTo>
struct Converter
{
//private: VC7 complains
    struct UnitBase : public VariantFrom::ConstStrictVisitor
    {
    protected:
        VariantTo* storageForDestination;
    };

    template <class Type, class Base> 
    struct Unit : public Base
    {
    private:
        void DoVisit(const Type& obj, ::Loki::Int2Type<true>)
        {
            new(this->storageForDestination) VariantTo(obj);
        }
        void DoVisit(const Type&, ::Loki::Int2Type<false>)
        {
            throw std::runtime_error("Cannot convert");
        }
        virtual void Visit(const Type& obj)
        {
            using namespace Loki;
            typedef typename VariantTo::Types TList;
            enum { dispatch = TL::IndexOf<TList, Type>::value > -1 };
            this->DoVisit(obj, Int2Type<dispatch>());
        }
    };
    
private:
    typedef ::Loki::GenLinearHierarchy
    <
        typename VariantFrom::Types,
        Unit,
        UnitBase 
    > 
    VisitorBase;

public:
    struct Visitor : public VisitorBase
    {
        explicit Visitor(VariantTo& dest)
        {
            // Initialize the pointer to destination
            this->storageForDestination = &dest;
        }
    };
};


////////////////////////////////////////////////////////////////////////////////
// class template ConverterTo
// Supports Variant-to-T conversion
////////////////////////////////////////////////////////////////////////////////
#if 0
template <class VariantFrom, class T>
struct ConverterTo
{
private:
    struct DestHolder 
        : VariantFrom::ConstStrictVisitor
    {
    protected:
        DestHolder() {}
        DestHolder(const T& dest) : destination_(dest) {}
        T destination_;
    };

    template <class TList> 
    struct VisitorBase
        : VisitorBase<typename TList::Tail>
    {
    private:
        ASSERT_TYPELIST(TList);
    
        typedef typename TList::Head Type;
        typedef typename TList::Tail Tail;
    
    protected:
        VisitorBase<TList>() {}
        VisitorBase<TList>(const T& dest) 
            : VisitorBase<Tail>(dest) {}
    
    private:
        void DoVisit(const Type& obj, ::Loki::Int2Type<true>)
        {   //
            // T temp(obj)
            // swap(destination_, temp) or destination_ = temp
            //
            this->destination_ = obj;
        }
        void DoVisit(const Type&, ::Loki::Int2Type<false>)
        {
            throw std::runtime_error("Cannot convert");
        }

        virtual void Visit(const Type& obj)
        {
            using namespace Loki;
            enum { dispatch = Conversion<Type, T>::exists != 0 };
            this->DoVisit(obj, Int2Type<dispatch>());
        }
    };
    
    template <> 
    struct VisitorBase< ::Loki::NullType > 
        : DestHolder
    {
    protected:
        VisitorBase< ::Loki::NullType >() {}
        VisitorBase< ::Loki::NullType >(const T& dest) 
            : DestHolder(dest) {}
    };

    
    typedef VisitorBase
    <
        typename VariantFrom::Types
    >
    VisitorBaseType;
    
public:
    struct Visitor : public VisitorBaseType
    {
        Visitor() {}

        explicit Visitor(const T& dest) 
            : VisitorBaseType(dest) {}

        const T &GetDestination() const 
        { return this->destination_; }
    };
};

#else

template <class VariantFrom, class T>
struct ConverterTo
{
//private:
    struct UnitBase : public VariantFrom::ConstStrictVisitor
    {
    protected:
        T destination_;
    };

    template <class Type, class Base> 
    struct Unit : public Base
    {
    private:
        void DoVisit(const Type& obj, ::Loki::Int2Type<true>)
        {
            this->destination_ = obj;
        }
        void DoVisit(const Type&, ::Loki::Int2Type<false>)
        {
            throw std::runtime_error("Cannot convert");
        }
        virtual void Visit(const Type& obj)
        {
            using namespace Loki;
            enum { dispatch = Conversion<Type, T>::exists != 0 };
            this->DoVisit(obj, Int2Type<dispatch>());
        }
    };
    
    typedef ::Loki::GenLinearHierarchy<
        typename VariantFrom::Types,
        Unit,
        UnitBase > VisitorBase;

public:
    struct Visitor : public VisitorBase
    {
        const T &GetDestination() const 
        { return this->destination_; }
    };
};

#endif

namespace Private 
{
    template<typename T>
    struct RawDataKeeper
    {        
    private:        
        typedef char RawBuffer_t[sizeof(T)];
        enum ObjectState_e { eNone, ePreConstruct, ePostConstruct };

        T               &obj_;
        ObjectState_e   eObjState_;
        RawBuffer_t     bufferOrg_;
        RawBuffer_t     bufferNew_;

    private:
        void SetObj(const RawBuffer_t &buf) throw()
        {
            memcpy(&reinterpret_cast<char &>(obj_), buf, sizeof(buf));
        }    

        void GetObj(RawBuffer_t &buf) throw()
        {
            memcpy(buf, &reinterpret_cast<const char &>(obj_), sizeof(buf));
        }    

    public:
        explicit RawDataKeeper(T &obj) 
            : obj_(obj), eObjState_(eNone)
        {}
    
        // add const U & version ?
        template<typename U>
        void ConstructNew(U &src)
        {
            assert(eObjState_ == eNone);

            GetObj(bufferOrg_);
            
            eObjState_ = ePreConstruct;
            new (&obj_) T(src);
            eObjState_ = ePostConstruct;

            GetObj(bufferNew_);
            SetObj(bufferOrg_);
        }

        void SetNew() throw()
        {
            assert(eObjState_ == ePostConstruct);

            obj_.~T();
            SetObj(bufferNew_);
            eObjState_ = eNone;
        }

        ~RawDataKeeper()
        {
            switch(eObjState_)
            {
            case ePostConstruct:
                SetObj(bufferNew_);
                obj_.~T();
                // fall
            
            case ePreConstruct:
                SetObj(bufferOrg_);
                // fall

            case eNone:
                break;
            }
        }

    private:
        RawDataKeeper(const RawDataKeeper &);
        RawDataKeeper& operator=(const RawDataKeeper &);
    };

    //
    // based on Eric Fridman's safe_swap from boost Variant 
    //
    //   strong exception-safety guarantee.
    //
    // !WARNING!
    //   SafeSwap CANNOT be safely used in the general case if either
    //   argument's data members may be accessed concurrently.
    //
    template<typename T>
    void SafeSwap(T &lhs, T &rhs)
    {
        typedef RawDataKeeper<T> RhsDataKeeper;
        typedef RawDataKeeper<T> LhsDataKeeper;

        RhsDataKeeper rhsKeeper(rhs);
        rhsKeeper.ConstructNew(lhs);
            
        LhsDataKeeper lhsKeeper(lhs);
        lhsKeeper.ConstructNew(rhs);
    
        rhsKeeper.SetNew();
        lhsKeeper.SetNew();
    }


    //
    // based on Eric Fridman's safe_assign from boost Variant 
    //
    //   strong exception-safety guarantee.
    //
    // !WARNING!
    //   SafeAssign CANNOT be safely used in the general case if either
    //   argument's data members may be accessed concurrently.
    //
    template<typename T, typename U>
    void SafeAssign(T &lhs, const U &src)
    {
        typedef RawDataKeeper<T> LhsDataKeeper;

        LhsDataKeeper lhsKeeper(lhs);
        lhsKeeper.ConstructNew(src);
        lhsKeeper.SetNew();
    }

    template<typename T>
    inline void SwapHelper(T &lhs, T &rhs)
    {
        using namespace std;
        swap(lhs, rhs);
    }
} // namespace Private 

////////////////////////////////////////////////////////////////////////////////
// class template Variant
// Implements a discriminated union in C++
////////////////////////////////////////////////////////////////////////////////

template <class TList, class AlignedPODType = AlignedPOD<TList> >
class Variant
{
    // VC7: fatal error C1067: compiler limit :
    // debug information module size exceeded
    // Therfore define this type here 
    typedef typename AlignedPODType::Result Align;

public:
    typedef TList Types;

    // Default constructor
    // Initializes the Variant with a default-constructed object of the 
    // first type in the typelist
    Variant()
    {
        typedef typename TList::Head T;
        new(&buffer_[0]) T;
        vptr_ = VTableImpl<T>::GetVPTR();
    }

    // Copy constructor
    Variant(const Variant& rhs)
    {
        (rhs.vptr_->clone_)(rhs, *this);
    }

private:
    // Converting constructor; accepts any type in the typelist
    // @@@ Suggested simple improvement: accept any type convertible to one of 
    // the types in the typelist. Use Loki::Conversion as a building block
    template <class T>
    void VariantConstruct(const T& val, double)
    {
        STATIC_CHECK((::Loki::TL::IndexOf<TList, T>::value >= 0), 
            Invalid_Type_Used_As_Initializer);
        
        new(&buffer_[0]) T(val);
        vptr_ = VTableImpl<T>::GetVPTR();
    }
    
    // Inter-Variant conversion constructor
    // Current policy is: conversion succeeds iff the actual type of the source 
    // is one of the types accepted by the target
    // @@@ Possible change: accept if the actual type of the source is
    //     convertible to one of the types accepted by the target. Problem to 
    //     solve: handle ambiguities in a satisfactory manner. Suggestion: when
    //     in doubt, do closest to what the compiler would do.
    template <class TList2, typename Align2>
    void VariantConstruct(const Variant<TList2, Align2>& rhs, int)
    {
        typename Converter<Variant<TList2, Align2>, Variant>::Visitor v(*this);
        typename Variant<TList2, Align2>::ConstStrictVisitor& visitor = v;
        rhs.Accept(visitor);
    }

public:
    // VC7 don't support partial ordering
    // The constructor initialization section 
    // is empty which make it to use function instead
    // without the int = 0 the explicit seems to confuse
    // VC7 when the copy constructor should be selected
    // Crazy stuff
    template <class T>
    explicit Variant(const T& val, int = 0)
    {
        VariantConstruct(val, int(0));
    }

    // Canonic assignment operator
    Variant& operator=(const Variant& rhs)
    {
        Private::SafeAssign(*this, rhs);
        return *this;
    }
    
private:
    // Assignment operator from one of the allowed types
    // This is necessary because the constructor is explicit
    template <class T> 
    void VariantAssign(const T& rhs, double)
    {
        Private::SafeAssign(*this, rhs);
    }
    
    // Assignment from another Variant instantiation
    template <class TList2, typename Align2>
    void VariantAssign(const Variant<TList2, Align2>& rhs, int)
    {
        Private::SafeAssign(*this, rhs);
    }
    
public:
    // VC7 don't support partial ordering
    template <class T> 
    Variant& operator=(const T& rhs)
    {
        // Both VariantAssign are the same in this implementation
        VariantAssign(rhs, int(0));
        return *this;
    }

    // ~
    ~Variant()
    {
        (vptr_->destroy_)(*this);
    }

    // Visitors definitions
    // @@@ Possible improvement: add defintions of visitor who return
    //     something else than void

    typedef ::Loki::Visitor<TList, void> StrictVisitor;
    typedef ::Loki::Visitor<typename MakeConst<TList>::Result, void>
        ConstStrictVisitor;
    typedef ::Loki::NonStrictVisitor<TList, void> NonStrictVisitor;
    typedef ::Loki::NonStrictVisitor<typename MakeConst<TList>::Result, void> 
        ConstNonStrictVisitor;

private:
    // VTable structure
    // The essential component of the fake vtable idiom, VTable contains
    // pointers to functions, pointers that will be filled up with addresses
    // of functions in VTableImpl

    struct VTable
    {
        const std::type_info& (*typeId_)();
        void (*destroy_)(const Variant&);
        void (*clone_)(const Variant&, Variant&);
        void (*swap_)(void* lhs, void* rhs);
        void (*accept_)(Variant&, StrictVisitor&);
        void (*acceptConst_)(const Variant&, ConstStrictVisitor&);
    };

    // VTable concrete implementations
    // VTableImpl<T> contains definitions for all of a VTable's pointer to
    // functions.

    // VC7 thinks that Variant inside VTableImpl is template
    typedef Variant VariantType;

    template <class T>
    struct VTableImpl
    {
    private:
        static const std::type_info& TypeId()
        {
            return typeid(T);
        }
        
        static void Destroy(const VariantType& var)
        {
            const T& data = *reinterpret_cast<const T*>(&var.buffer_[0]);
            (void)data.~T();
        }
        
        static void Swap(void* lhs, void* rhs)
        {
            Private::SwapHelper(*static_cast<T*>(lhs), *static_cast<T*>(rhs));
        }

        static void Clone(const VariantType& src, VariantType& dest)
        {
            new(&dest.buffer_[0]) T(
                *reinterpret_cast<const T*>(&src.buffer_[0]));
            dest.vptr_ = src.vptr_;
        }
                
        static void Accept(VariantType& var, StrictVisitor& visitor)
        {
            typedef typename StrictVisitor::ReturnType RType;
            ::Loki::Visitor<T,RType> &v = visitor;

            v.Visit(*reinterpret_cast<T*>(&var.buffer_[0]));
        }        
        
        static void AcceptConst(const VariantType& var, ConstStrictVisitor& visitor)
        {
            typedef typename ConstStrictVisitor::ReturnType RType;
            ::Loki::Visitor<const T,RType> &v = visitor;

            v.Visit(*reinterpret_cast<const T*>(&var.buffer_[0]));
        }        

    public:
        static const VTable *GetVPTR()
        {
            static const VTable vTbl_ =
            {
                &TypeId,
                &Destroy,
                &Clone,
                &Swap,
                &Accept,
                &AcceptConst,
            };

            return &vTbl_;
        }
    };

    template <class T> friend struct VTableImpl;

private:   // should be private; some compilers prefer 'public' :o}

    enum { neededSize = MaxSize<TList>::result };

    VTable const * vptr_;
    union
    {
        Align dummy_;
        char buffer_[neededSize];
    };
    
public:
    void swap(Variant& rhs)
    {
        if (this->TypeId() == rhs.TypeId())
        {
            (vptr_->swap_)(this->buffer_, rhs.buffer_);
        }
        else
        {
            Private::SafeSwap(*this, rhs);
        }
    }
    
    const std::type_info& TypeId() const
    {
        return (vptr_->typeId_)();
    }
    
    template <typename T> T* GetPtr()
    {
        return TypeId() == typeid(T) 
            ? reinterpret_cast<T*>(&buffer_[0])
            : 0;
    }
    
    template <typename T> const T* GetPtr() const
    {
        return TypeId() == typeid(T) 
            ? reinterpret_cast<const T*>(&buffer_[0])
            : 0;
    }
    
    template <typename T> T& Get()
    {
        T* p = GetPtr<T>();
        if (!p) throw std::runtime_error("Variant::Get() Invalid variant type");
        return *p;
    }
    
    template <typename T> const T& Get() const
    {
        const T* p = GetPtr<T>();
        if (!p) throw std::runtime_error("Variant::Get() const Invalid variant type");
        return *p;
    }

    // Visitation primitives
    // Although there are four visitor types, only two Accept functions are 
    // necessary, because the non-strict visitors inherit the strict visitors

    void Accept(StrictVisitor& visitor)
    {
        (vptr_->accept_)(*this, visitor);
    }

    void Accept(ConstStrictVisitor& visitor) const
    {
        (vptr_->acceptConst_)(*this, visitor);
    }

    // Extracts the value of a Variant converted to a specific type

    template <class To> To ConvertTo() const
    {
        typename ConverterTo<Variant, To>::Visitor v;
        ConstStrictVisitor& visitor = v;
        this->Accept(visitor);
        return v.GetDestination();
    }

    // Changes the type of a Variant in-place

    template <class To> void ChangeType()
    {
        Variant(this->ConvertTo<To>()).swap(*this);
    }

};

////////////////////////////////////////////////////////////////////////////////
// Change log:
// July 10, 2002: ported by Rani Sharoni to VC7 (RTM - 9466)
////////////////////////////////////////////////////////////////////////////////

#endif

