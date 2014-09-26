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

#ifndef MINMAX_INC_
#define MINMAX_INC_

#include "Typelist.h"
#include "TypeTraits.h"

namespace Private
{
    typedef TYPELIST_14(
            const bool,
            const char,
            const signed char,
            const unsigned char,
            const wchar_t,
            const short int,
            const unsigned short int,
            const int,
            const unsigned int,
            const long int,
            const unsigned long int,
            const float,
            const double,
            const long double)
        ArithTypes;
}

template <class L, class R>
class MinMaxTraits
{
    typedef typename Loki::Select<Loki::Conversion<R, L>::exists, 
            L, R>::Result
        T1;
    
    enum { pos1 = Loki::TL::IndexOf<Private::ArithTypes, const L>::value };
    enum { pos2 = Loki::TL::IndexOf<Private::ArithTypes, const R>::value };
    typedef Loki::Select<pos1 != -1 && pos1 < pos2, R, T1>::Result T2;

    enum { rConst = Loki::TypeTraits<R>::isConst >=
        Loki::TypeTraits<L>::isConst };
    enum { l2r = rConst && Loki::Conversion<
        typename Loki::TypeTraits<L>::NonConstType&, 
        typename Loki::TypeTraits<R>::NonConstType&>::exists };
    typedef typename Loki::Select<l2r, R&, T2>::Result T3;

    enum { lConst = Loki::TypeTraits<L>::isConst >=
        Loki::TypeTraits<R>::isConst };
    enum { r2l = lConst && Loki::Conversion<
        typename Loki::TypeTraits<R>::NonConstType&, 
        typename Loki::TypeTraits<L>::NonConstType&>::exists };
public:
    typedef typename Loki::Select<r2l, L&, T3>::Result Result;
};

template <class L, class R>
typename MinMaxTraits<L, R>::Result
Min(L& lhs, R& rhs)
{ if (lhs < rhs) return lhs; return rhs; }

template <class L, class R>
typename MinMaxTraits<const L, R>::Result
Min(const L& lhs, R& rhs)
{ if (lhs < rhs) return lhs; return rhs; }

template <class L, class R>
typename MinMaxTraits<L, const R>::Result
Min(L& lhs, const R& rhs)
{ if (lhs < rhs) return lhs; return rhs; }

template <class L, class R>
typename MinMaxTraits<const L, const R>::Result
Min(const L& lhs, const R& rhs)
{ if (lhs < rhs) return lhs; return rhs; }

template <class L, class R>
typename MinMaxTraits<L, R>::Result
Max(L& lhs, R& rhs)
{ if (lhs > rhs) return lhs; return rhs; }

template <class L, class R>
typename MinMaxTraits<const L, R>::Result
Max(const L& lhs, R& rhs)
{ if (lhs > rhs) return lhs; return rhs; }

template <class L, class R>
typename MinMaxTraits<L, const R>::Result
Max(L& lhs, const R& rhs)
{ if (lhs > rhs) return lhs; return rhs; }

template <class L, class R>
typename MinMaxTraits<const L, const R>::Result
Max(const L& lhs, const R& rhs)
{ if (lhs > rhs) return lhs; return rhs; }


#endif // MINMAX_INC_
