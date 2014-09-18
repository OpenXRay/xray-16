////////////////////////////////////////////////////////////////////////////
//	Module 		: script_engine_export.cpp
//	Created 	: 01.04.2004
//  Modified 	: 22.06.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script Engine export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"

#define SCRIPT_REGISTRATOR
#include "script_export_space.h"
#include "script_engine_export.h"

#pragma optimize("s",on)
template <typename TList> struct Register
{
	ASSERT_TYPELIST(TList);

	static void _Register(lua_State *L)
	{
		Register<TList::Tail>::_Register(L);
#ifdef XRGAME_EXPORTS
#	ifdef _DEBUG
		Msg("Exporting %s",typeid(TList::Head).name());
#	endif
#endif
		TList::Head::script_register(L);
	}
};

template <> struct Register<Loki::NullType>
{
	static void _Register(lua_State *L)
	{
	}
};

template <typename T1, typename T2>
struct TypePair {
	typedef T1	first;
	typedef T2	second;
};

template <typename TFullList> struct DynamicCast
{
	ASSERT_TYPELIST(TFullList);

	template <typename TList, typename T> struct Helper2
	{
		typedef typename TList::Head Head;

		static void Register(lua_State *L)
		{
			Helper2<TList::Tail,T>::Register(L);
			declare<Loki::SuperSubclassStrict<Head,T>::value>();
		}

		template <bool _1>
		static void declare()
		{
		}

		template <>
		static void declare<true>()
		{
			Msg		("Exporting function to cast from \"%s\" to \"%s\"",typeid(T).name(),typeid(Head).name());
		}
	};

	template <typename T> struct Helper2<Loki::NullType,T>
	{
		static void Register(lua_State *L)
		{
		}
	};

	template <typename TList> struct Helper
	{
		static void Register(lua_State *L)
		{
			Helper<TList::Tail>::Register(L);
			Helper2<TFullList,TList::Head>::Register(L);
		}
	};

	template <> struct Helper<Loki::NullType>
	{
		static void Register(lua_State *L)
		{
		}
	};

	static void Register(lua_State *L)
	{
		Helper<TFullList>::Register(L);
	}
};

void export_classes	(lua_State *L)
{
	Register<script_type_list>::_Register(L);
//	DynamicCast<script_type_list>::Register(L);
//	Register<Loki::TL::DerivedToFrontAll<script_type_list>::Result>::_Register(L);
}
