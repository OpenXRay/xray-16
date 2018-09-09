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

#ifdef _MSC_VER
#pragma once

#pragma warning( push )
 
 // 'class1' : base-class 'class2' is already a base-class of 'class3'
#pragma warning( disable : 4584 )

#endif // _MSC_VER

#ifndef HIERARCHYGENERATORS_INC_
#define HIERARCHYGENERATORS_INC_

#include "Typelist.h"

namespace Loki
{
////////////////////////////////////////////////////////////////////////////////
// class template GenLinearHierarchy
// Generates a linear hierarchy starting from a typelist and a template
// Invocation (TList is a typelist, Model is a template of two args):
// GenLinearHierarchy<TList, Model, Root>
////////////////////////////////////////////////////////////////////////////////
    
    template <typename, template <typename, typename> typename, typename Root = EmptyType>
    struct GenLinearHierarchy;

    template <typename T, template <typename, typename> typename Unit, typename Root>
    struct GenLinearHierarchy<Typelist<T>, Unit, Root>
        : Unit<T, Root>
    { };

    template <typename T, typename... Ts, template <typename, typename> typename Unit, typename Root>
    struct GenLinearHierarchy<Typelist<T, Ts...>, Unit, Root>
        : Unit<T, GenLinearHierarchy<Typelist<Ts...>, Unit, Root>>
    { };

}   // namespace Loki

////////////////////////////////////////////////////////////////////////////////
// Change log:
// June 20, 2001: ported by Nick Thurn to gcc 2.95.3. Kudos, Nick!!!
// May  10, 2002: ported by Rani Sharoni to VC7 (RTM - 9466)
////////////////////////////////////////////////////////////////////////////////

#endif // HIERARCHYGENERATORS_INC_

