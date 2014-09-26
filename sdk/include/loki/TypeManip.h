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

#ifndef TYPEMANIP_INC_
#define TYPEMANIP_INC_

namespace Loki
{    
////////////////////////////////////////////////////////////////////////////////
// class template Int2Type
// Converts each integral constant into a unique type
// Invocation: Int2Type<v> where v is a compile-time constant integral
// Defines 'value', an enum that evaluates to v
////////////////////////////////////////////////////////////////////////////////

    template <int v>
    struct Int2Type
    {
        enum { value = v };
    };
    
////////////////////////////////////////////////////////////////////////////////
// class template Type2Type
// Converts each type into a unique, insipid type
// Invocation Type2Type<T> where T is a type
// Defines the type OriginalType which maps back to T
////////////////////////////////////////////////////////////////////////////////

    template <typename T>
    struct Type2Type
    {   
        typedef T OriginalType;
        Type2Type(){} // VC7
    };
    
////////////////////////////////////////////////////////////////////////////////
// class template Select
// Selects one of two types based upon a boolean constant
// Invocation: Select<flag, T, U>::Result
// where:
// flag is a compile-time boolean constant
// T and U are types
// Result evaluates to T if flag is true, and to U otherwise.
////////////////////////////////////////////////////////////////////////////////

    template <bool flag, typename T, typename U>
    struct Select
    {
    private:
        template<bool>
        struct In 
        { typedef T Result; };

        template<>
        struct In<false>
        { typedef U Result; };

    public:
        typedef typename In<flag>::Result Result;
    };
    

////////////////////////////////////////////////////////////////////////////////
// class template IsSameType
// Return true iff two given types are the same
// Invocation: IsSameType<T, U>::value
// where:
// T and U are types
// Result evaluates to true iff U == T (types equal)
////////////////////////////////////////////////////////////////////////////////

    template <typename T, typename U>
    struct IsSameType
    {
    private:
        template<typename>
        struct In 
        { enum { value = false }; };

        template<>
        struct In<T>
        { enum { value = true };  };

    public:
        enum { value = In<U>::value };
    };
    
////////////////////////////////////////////////////////////////////////////////
// Helper types Small and Big - guarantee that sizeof(Small) < sizeof(Big)
////////////////////////////////////////////////////////////////////////////////

    namespace Private
    {
        typedef char Small;
        class Big { char dummy[2]; };

        template<typename T>
        struct IsVoid
        {
            enum { result = 
                IsSameType<T, void>::value          ||
                IsSameType<T, const void>::value    ||
                IsSameType<T, volatile void>::value ||
                IsSameType<T, const volatile void>::value
            };
        };
    }

//
// is one type convertable to another?
//
    template <class T, class U>
    class is_convertible
    {
        struct VoidReplace {};

        typedef typename Select
        <
            Private::IsVoid<T>::result,
            VoidReplace, T
        >
        ::Result T1;

        typedef typename Select
        <
            Private::IsVoid<U>::result,
            VoidReplace, U
        >
        ::Result U1;

        static Private::Big   Test(...);
        static Private::Small Test(U1);
        static T1 MakeT();

    public:       
        enum { exists = sizeof(Test(MakeT())) == sizeof(Private::Small) };
    };

////////////////////////////////////////////////////////////////////////////////
// class template Conversion
// Figures out the conversion relationships between two types
// Invocations (T and U are types):
// a) Conversion<T, U>::exists
// returns (at compile time) true if there is an implicit conversion from T
// to U (example: Derived to Base)
// b) Conversion<T, U>::exists2Way
// returns (at compile time) true if there are both conversions from T
// to U and from U to T (example: int to char and back)
// c) Conversion<T, U>::sameType
// returns (at compile time) true if T and U represent the same type
//
// Caveat: might not work if T and U are in a private inheritance hierarchy.
////////////////////////////////////////////////////////////////////////////////

    template <class T, class U>
    struct Conversion
    {
        enum { exists = (is_convertible<T,U>::exists) };
        enum { exists2Way = (exists && is_convertible<U, T>::exists) };
        enum { sameType = (IsSameType<T, U>::value) };
    };

////////////////////////////////////////////////////////////////////////////////
// class template SuperSubclass
// Invocation: SuperSubclass<B, D>::value where B and D are types. 
// Returns true if B is a public base of D, or if B and D are aliases of the 
// same type.
//
// Caveat: might not work if T and U are in a private inheritance hierarchy.
////////////////////////////////////////////////////////////////////////////////

template <class T, class U>
struct SuperSubclass
{
  enum { value = (::Loki::Conversion<const volatile U*, const volatile T*>::exists &&
                  !::Loki::Conversion<const volatile T*, const volatile void*>::sameType) };
};

////////////////////////////////////////////////////////////////////////////////
// class template SuperSubclassStrict
// Invocation: SuperSubclassStrict<B, D>::value where B and D are types. 
// Returns true if B is a public base of D.
//
// Caveat: might not work if T and U are in a private inheritance hierarchy.
////////////////////////////////////////////////////////////////////////////////

template<class T,class U>
struct SuperSubclassStrict
{
  enum { value = (::Loki::Conversion<const volatile U*, const volatile T*>::exists &&
                 !::Loki::Conversion<const volatile T*, const volatile void*>::sameType &&
                 !::Loki::Conversion<const volatile T*, const volatile U*>::sameType) };
};

}   // namespace Loki

////////////////////////////////////////////////////////////////////////////////
// macro SUPERSUBCLASS
// Invocation: SUPERSUBCLASS(B, D) where B and D are types. 
// Returns true if B is a public base of D, or if B and D are aliases of the 
// same type.
//
// Caveat: might not work if T and U are in a private inheritance hierarchy.
// Deprecated: Use SuperSubclass class template instead.
////////////////////////////////////////////////////////////////////////////////

#define SUPERSUBCLASS(T, U) \
    ::Loki::SuperSubclass<T,U>::value

////////////////////////////////////////////////////////////////////////////////
// macro SUPERSUBCLASS_STRICT
// Invocation: SUPERSUBCLASS(B, D) where B and D are types. 
// Returns true if B is a public base of D.
//
// Caveat: might not work if T and U are in a private inheritance hierarchy.
// Deprecated: Use SuperSubclassStrict class template instead.
////////////////////////////////////////////////////////////////////////////////

#define SUPERSUBCLASS_STRICT(T, U) \
    ::Loki::SuperSubclassStrict<T,U>::value

////////////////////////////////////////////////////////////////////////////////
// Change log:
// June    20, 2001: ported by Nick Thurn to gcc 2.95.3. Kudos, Nick!!!
// May     10, 2002: ported by Rani Sharoni to VC7 (RTM - 9466)
// October 10, 2002: Commented IsSameType template (not a Loki-template - yet). MKH
// October 12, 2002: Added SuperSubclass and SuperSubclassStrict templates.
//                   The corresponding macros are deprecated. T.S.
////////////////////////////////////////////////////////////////////////////////

#endif // TYPEMANIP_INC_
