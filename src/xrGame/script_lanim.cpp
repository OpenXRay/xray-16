////////////////////////////////////////////////////////////////////////////
//	Module 		: script_ini_file.cpp
//	Created 	: 21.05.2004
//  Modified 	: 21.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Script ini file class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_lanim.h"
#include "../xrEngine/LightAnimLibrary.h"


using namespace luabind;

struct lanim_wrapper
{
	CLAItem* item;
public:
			lanim_wrapper	(LPCSTR name){load(name);}
	void	load			(LPCSTR name)
	{
		item				= LALib.FindItem(name);
		R_ASSERT3			(item,"Can't find color anim:",name);
	}
	u32		length			()
	{
		VERIFY				(item);
		return item->Length_ms();
	}
	Fcolor	calculate		(float T)
	{
		int frame;
		VERIFY				(item);
		return Fcolor().set(item->CalculateRGB(T,frame));
	}
};

#pragma optimize("s",on)
void lanim_registrator::script_register(lua_State *L)
{
	module(L)
		[
			class_<lanim_wrapper>("color_animator")
			.def(					constructor<LPCSTR>())
			.def("load",			&lanim_wrapper::load)
			.def("calculate",		&lanim_wrapper::calculate)
			.def("length",			&lanim_wrapper::length)
		];
}