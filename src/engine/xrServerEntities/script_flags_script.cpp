////////////////////////////////////////////////////////////////////////////
//	Module 		: script_flags_script.cpp
//	Created 	: 19.07.2004
//  Modified 	: 19.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Script flags script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_flags.h"

using namespace luabind;

template <typename T>
T& set(T *self, const typename T::TYPE mask, bool value)
{
	return	(self->set(mask,value));
}

template <typename T>
bool is(T *self, const typename T::TYPE mask)
{
	return	(!!self->is(mask));
}

template <typename T>
bool is_any(T *self, const typename T::TYPE mask)
{
	return	(!!self->is_any(mask));
}

template <typename T>
bool test(T *self, const typename T::TYPE mask)
{
	return	(!!self->test(mask));
}

template <typename T>
bool equal(T *self, const T &f)
{
	return	(!!self->equal(f));
}

template <typename T>
bool equal(T *self, const T &f, const typename T::TYPE mask)
{
	return	(!!self->equal(f,mask));
}

template <typename T>
void one(T *self)
{
	self->assign(typename T::TYPE(-1));
}

#pragma optimize("s",on)
void CScriptFlags::script_register(lua_State *L)
{
	module(L)
	[
//		class_<Flags8>		("flags8")
//			.def(			constructor<>())
//			.def("get",		&Flags8::get)
//			.def("zero",	&Flags8::zero)
//			.def("one",		&one<Flags8>)
//			.def("invert",	(Flags8& (Flags8::*)())(&Flags8::invert))
//			.def("invert",	(Flags8& (Flags8::*)(const Flags8&))(&Flags8::invert))
//			.def("invert",	(Flags8& (Flags8::*)(const Flags8::TYPE))(&Flags8::invert))
//			.def("assign",	(Flags8& (Flags8::*)(const Flags8&))(&Flags8::assign))
//			.def("assign",	(Flags8& (Flags8::*)(const Flags8::TYPE))(&Flags8::assign))
//			.def("or",		(Flags8& (Flags8::*)(const Flags8::TYPE))(&Flags8::or))
//			.def("or",		(Flags8& (Flags8::*)(const Flags8&,const Flags8::TYPE))(&Flags8::or))
//			.def("and",		(Flags8& (Flags8::*)(const Flags8::TYPE))(&Flags8::and))
//			.def("and",		(Flags8& (Flags8::*)(const Flags8&,const Flags8::TYPE))(&Flags8::and))
//			.def("set",		&set<Flags8>)
//			.def("is",		&is<Flags8>)
//			.def("is_any",	&is_any<Flags8>)
//			.def("test",	&test<Flags8>)
//			.def("equal",	(bool (*)(Flags8*,const Flags8&))(&equal<Flags8>))
//			.def("equal",	(bool (*)(Flags8*,const Flags8&,const Flags8::TYPE))(&equal<Flags8>)),
//
		class_<Flags16>		("flags16")
			.def(			constructor<>())
			.def("get",		&Flags16::get)
			.def("zero",	&Flags16::zero)
			.def("one",		&one<Flags16>)
			.def("invert",	(Flags16& (Flags16::*)())(&Flags16::invert))
			.def("invert",	(Flags16& (Flags16::*)(const Flags16&))(&Flags16::invert))
			.def("invert",	(Flags16& (Flags16::*)(const Flags16::TYPE))(&Flags16::invert))
			.def("assign",	(Flags16& (Flags16::*)(const Flags16&))(&Flags16::assign))
			.def("assign",	(Flags16& (Flags16::*)(const Flags16::TYPE))(&Flags16::assign))
			.def("or",		(Flags16& (Flags16::*)(const Flags16::TYPE))(&Flags16::or))
			.def("or",		(Flags16& (Flags16::*)(const Flags16&,const Flags16::TYPE))(&Flags16::or))
			.def("and",		(Flags16& (Flags16::*)(const Flags16::TYPE))(&Flags16::and))
			.def("and",		(Flags16& (Flags16::*)(const Flags16&,const Flags16::TYPE))(&Flags16::and))
			.def("set",		&set<Flags16>)
			.def("is",		&is<Flags16>)
			.def("is_any",	&is_any<Flags16>)
			.def("test",	&test<Flags16>)
			.def("equal",	(bool (*)(Flags16*,const Flags16&))(&equal<Flags16>))
			.def("equal",	(bool (*)(Flags16*,const Flags16&,const Flags16::TYPE))(&equal<Flags16>)),

		class_<Flags32>		("flags32")
			.def(			constructor<>())
			.def("get",		&Flags32::get)
			.def("zero",	&Flags32::zero)
			.def("one",		&Flags32::one)
			.def("invert",	(Flags32& (Flags32::*)())(&Flags32::invert))
			.def("invert",	(Flags32& (Flags32::*)(const Flags32&))(&Flags32::invert))
			.def("invert",	(Flags32& (Flags32::*)(const Flags32::TYPE))(&Flags32::invert))
			.def("assign",	(Flags32& (Flags32::*)(const Flags32&))(&Flags32::assign))
			.def("assign",	(Flags32& (Flags32::*)(const Flags32::TYPE))(&Flags32::assign))
			.def("or",		(Flags32& (Flags32::*)(const Flags32::TYPE))(&Flags32::or))
			.def("or",		(Flags32& (Flags32::*)(const Flags32&,const Flags32::TYPE))(&Flags32::or))
			.def("and",		(Flags32& (Flags32::*)(const Flags32::TYPE))(&Flags32::and))
			.def("and",		(Flags32& (Flags32::*)(const Flags32&,const Flags32::TYPE))(&Flags32::and))
			.def("set",		&set<Flags32>)
			.def("is",		&is<Flags32>)
			.def("is_any",	&is_any<Flags32>)
			.def("test",	&test<Flags32>)
			.def("equal",	(bool (*)(Flags32*,const Flags32&))(&equal<Flags32>))
			.def("equal",	(bool (*)(Flags32*,const Flags32&,const Flags32::TYPE))(&equal<Flags32>))
	];
}