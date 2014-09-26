////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Data Generator by Mr. Shannon Barber
// This code DOES NOT accompany the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design 
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
//
// Code covered by the MIT License
//
// The author makes no representations about the suitability of this software
//  for any purpose. It is provided "as is" without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

// Last update: Oct 10, 2002

#pragma once
#include "Typelist.h"

//MSVC7 version
namespace Loki
{
////////////////////////////////////////////////////////////////////////////////
// class template IterateTypes
////////////////////////////////////////////////////////////////////////////////
	namespace TL
		{
		template<typename T>
		struct nameof_type
			{
			const char* operator()()
				{
				return typeid(T).name();
				}
			};
		template<typename T>
		struct sizeof_type
			{
			size_t operator()()
				{
				return sizeof(T);
				}
			};    
    template <class TList, template <typename> class UnitFunc>
    struct IterateTypes;

    namespace Private
    {
    // for some reason VC7 needs the base definition altough not in use
    template <typename TListTag> 
    struct IterateTypesHelper1
    {
        template <typename T, template <typename> class GenFunc>
        struct In 
        { 
            typedef typename T::ERROR_THIS_INSTANCE_SELECTED Result; 
        };
    };

    template <typename TListTag> 
    struct IterateTypesHelper2
    {
        template <typename T, template <typename> class GenFunc>
        struct In 
        { 
            typedef typename T::ERROR_THIS_INSTANCE_SELECTED Result; 
        };
    };

    
    template <> 
    struct IterateTypesHelper1<TL::Typelist_tag>
    {
        template <class TList, template <typename> class GenFunc>
        struct In
        {
            typedef IterateTypes<typename TList::Head, GenFunc> Result;
        };
    };

    template <> 
    struct IterateTypesHelper2<TL::Typelist_tag>
    {
        template <class TList, template <typename> class GenFunc>
        struct In
        {
            typedef IterateTypes<typename TList::Tail, GenFunc> Result;
        };
    };
    

    template <> 
    struct IterateTypesHelper1<TL::NoneList_tag>
    {
        template <typename AtomicType, template <typename> class GenFunc>
        struct In 
        { 
            struct Result
            {
				typedef GenFunc<AtomicType> genfunc_t;
                template<class II>
                void operator()(II ii)
                {
                genfunc_t gen;
				//warning C4267: 'argument' : conversion from 'size_t' to 'const std::_Vbase', possible loss of data
#pragma warning(push)
#pragma warning(disable: 4267)
				//TODOSGB
				*ii = gen();
#pragma warning(pop)
				++ii;
                }
                template<class II, class P1>
                void operator()(II ii, P1 p1)
                {
                    genfunc_t gen;
						  *ii = gen(p1);
						  ++ii;
                }
            };
        };
    };

    template <> 
    struct IterateTypesHelper2<TL::NoneList_tag>
    {
        template <typename AtomicType, template <typename> class GenFunc>
        struct In 
        { 
            struct Result 
            {
                template<class II> void operator()(II) {}
				    template<class II, class P1> void operator()(II, P1) {}
            }; 
        };        
    };


    template <> 
    struct IterateTypesHelper1<TL::NullType_tag>
    {
        template <class TList, template <typename> class GenFunc>
        struct In 
        { 
            struct Result 
            {
                template<class II> void operator()(II) {}
				    template<class II, class P1> void operator()(II, P1) {}
				}; 
        };        
    };

    template <> 
    struct IterateTypesHelper2<TL::NullType_tag>
    {
        template <class TList, template <typename> class GenFunc>
        struct In 
        { 
            struct Result 
            {
                template<class II> void operator()(II) {}
				    template<class II, class P1> void operator()(II, P1) {}
            }; 
        };        
    };

    } // namespace Private



////////////////////////////////////////////////////////////////////////////////
// class template IterateTypes
// Iteratates a Typelist, and invokes the ctor of GenFunc<T>
// for each type in the list, passing a functor along the way.
// The functor is designed to be an insertion iterator which GenFunc<T>
// can use to output information about the types in the list.
////////////////////////////////////////////////////////////////////////////////

    
    template <typename T, template <typename> class GenFunc>
    struct IterateTypes
    {
    public:

        typedef typename Private::IterateTypesHelper1
        <
            typename TL::is_Typelist<T>::type_tag
        >
        ::template In<T, GenFunc>::Result head_t;

        typedef typename Private::IterateTypesHelper2
        <
            typename TL::is_Typelist<T>::type_tag
        >
        ::template In<T, GenFunc>::Result tail_t;
		  head_t head;
		  tail_t tail;

	     template<class II>
        void operator()(II ii)
        {
		  this->head.operator()(ii);
		  this->tail.operator()(ii);
		  }
	     template<class II, class P1>
        void operator()(II ii, P1 p1)
        {
		  this->head.operator()(ii, p1);
		  this->tail.operator()(ii, p1);
		  }
	 };

	//UnitFunc is really a template-template parameter, but MSVC7
	// chokes on the correct defintion.  Oddly enough, it works correctly
	// with the 'undecorated' template parameter declaraion!
	//template <class> class UnitFunc
	template<typename Types, class UnitFunc, typename II>
	void iterate_types(II ii)
		{
		Loki::TL::IterateTypes<Types, UnitFunc> it;
		it(ii);
		}
	}//ns TL
}//ns Loki

////////////////////////////////////////////////////////////////////////////////
// Change log:
// Aug 17, 2002:  Ported to MSVC7 by Rani Sharoni 
// Aug 18, 2002:  Removed ctor(II), replaced with operator(II) Shannon Barber
// Oct 10, 2002:  Changed II (insertion iterator) from pass-by-reference to pass-by-value
////////////////////////////////////////////////////////////////////////////////

