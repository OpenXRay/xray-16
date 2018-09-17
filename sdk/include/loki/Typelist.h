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

// Last update: Oct 11, 2002
//MSVC7

#ifndef TYPELIST_INC_
#define TYPELIST_INC_

#include <type_traits>
#include "EmptyType.h"

namespace Loki
{
////////////////////////////////////////////////////////////////////////////////
// class template Typelist
// The building block of typelists of any length
// Use it through the TYPELIST_NN macros
// Defines nested types:
//     Head (first element, a non-typelist type by convention)
//     Tail (second element, can be another typelist)
////////////////////////////////////////////////////////////////////////////////

    template <typename... Ts> struct Typelist;

    template <typename T, typename... Ts>
    struct Typelist<T, Ts ...> {
        using Head = T;
    };

    template <> struct Typelist<> {
        using Head = EmptyType;
    };

    namespace TL
    {

////////////////////////////////////////////////////////////////////////////////
// class template is_Typelist
// detects if type is Typelist (including Nulltype)
// Invocation :
// is_Typelist<T>::value
// returns a compile-time boolean constant containing true iff T is some Typelist<T1,T2>
////////////////////////////////////////////////////////////////////////////////
        template <typename>
        struct is_Typelist : std::false_type { };

        template <typename... Ts>
        struct is_Typelist<Typelist<Ts ...>> : std::true_type { };

////////////////////////////////////////////////////////////////////////////////
// class template Length
// Computes the length of a typelist
// Invocation (TList is a typelist):
// Length<TList>::value
// returns a compile-time constant containing the length of TList, not counting
//     the end terminator (which by convention is NullType)
////////////////////////////////////////////////////////////////////////////////

        // Length
        template <typename>
        struct Length;

        template <typename... Ts>
        struct Length<Typelist<Ts...>>
         : std::integral_constant<size_t, sizeof...(Ts)> { };

////////////////////////////////////////////////////////////////////////////////
// class template TypeAt
// Finds the type at a given index in a typelist
// Invocation (TList is a typelist and index is a compile-time integral 
//     constant):
// TypeAt<TList, index>::Result
// returns the type in position 'index' in TList
// If you pass an out-of-bounds index, the result is a compile-time error
////////////////////////////////////////////////////////////////////////////////

        template <typename, size_t>
        struct TypeAt;

        template <typename T, typename... Ts>
        struct TypeAt<Typelist<T, Ts...>, 0> {
          using type = T;
        };

        template <size_t ix, typename T, typename... Ts>
        struct TypeAt<Typelist<T, Ts...>, ix> {
          using type = typename TypeAt<Typelist<Ts...>, ix - 1>::type;
        };
        
////////////////////////////////////////////////////////////////////////////////
// class template TypeAtNonStrict
// Finds the type at a given index in a typelist
// Invocations (TList is a typelist and index is a compile-time integral 
//     constant):
// a) TypeAt<TList, index>::Result
// returns the type in position 'index' in TList, or NullType if index is 
//     out-of-bounds
// b) TypeAt<TList, index, D>::Result
// returns the type in position 'index' in TList, or D if index is out-of-bounds
////////////////////////////////////////////////////////////////////////////////


        template <typename, size_t>
        struct TypeAtNonStrict;

        template <size_t ix>
        struct TypeAtNonStrict<Typelist<>, ix> {
          using type = EmptyType;
        };

        template <typename T, typename... Ts>
        struct TypeAtNonStrict<Typelist<T, Ts...>, 0> {
          using type = T;
        };

        template <typename T, typename... Ts, size_t ix>
        struct TypeAtNonStrict<Typelist<T, Ts...>, ix> {
          using type = typename TypeAtNonStrict<Typelist<Ts...>, ix - 1>::type;
        };

////////////////////////////////////////////////////////////////////////////////
// class template IndexOf
// Finds the index of a type in a typelist
// Invocation (TList is a typelist and T is a type):
// IndexOf<TList, T>::value
// returns the position of T in TList, or NullType if T is not found in TList
////////////////////////////////////////////////////////////////////////////////

        template<typename, typename>
        struct IndexOf;

        template <typename T, typename... Ts>
        struct IndexOf<Typelist<T, Ts...>, T>
            : std::integral_constant<int, 0>
        {};

        template <typename T, typename TOther, typename... Ts>
        struct IndexOf<Typelist<TOther, Ts...>, T>
            : std::integral_constant<int, 1 + IndexOf<Typelist<Ts...>, T>::value>
        {};

        template <typename T>
        struct IndexOf<Typelist<>, T>
            : std::integral_constant<int, -1>
        {};


        template<typename, typename>
        struct Prepend;

        template <typename T, typename... Ts>
        struct Prepend<T, Typelist<Ts...>>
        { 
            using result = Typelist<T, Ts...>;
        };
    }   // namespace TL
}   // namespace Loki

////////////////////////////////////////////////////////////////////////////////
// Change log:
// June 09, 2001: Fix bug in parameter list of macros TYPELIST_23 to TYPELIST_27
//      (credit due to Dave Taylor)
// June 20, 2001: ported by Nick Thurn to gcc 2.95.3. Kudos, Nick!!!
// May  10, 2002: ported by Rani Sharoni to VC7 (RTM - 9466)
// Oct  10, 2002: added MakeTypelist (SGB/MKH)
// Oct  11, 2002: DerivedToFront was incorrectly using ReplaceAll, now uses Replace
////////////////////////////////////////////////////////////////////////////////

#endif // TYPELIST_INC_

