////////////////////////////////////////////////////////////////////////////
//	Module 		: property_storage_script.h
//	Created 	: 29.03.2004
//  Modified 	: 29.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Property storage class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "property_storage.h"

using namespace luabind;

#pragma optimize("s",on)
void CPropertyStorage::script_register(lua_State *L)
{
	module(L)
	[
		class_<CPropertyStorage>("property_storage")
			.def(								constructor<>())
			.def("set_property",				&CPropertyStorage::set_property)
			.def("property",					&CPropertyStorage::property)
	];
}
