////////////////////////////////////////////////////////////////////////////
//	Module 		: script_world_property_script.h
//	Created 	: 19.03.2004
//  Modified 	: 19.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script world property script export
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "script_world_property.h"
#include "operator_abstract.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

SCRIPT_EXPORT(CScriptWorldProperty, (), {
    module(luaState)[class_<CScriptWorldProperty>("world_property")
                         .def(constructor<CScriptWorldProperty::condition_type, CScriptWorldProperty::value_type>())
                         .def("condition", &CScriptWorldProperty::condition)
                         .def("value", &CScriptWorldProperty::value)
                         .def(const_self < other<CScriptWorldProperty>())
                         .def(const_self == other<CScriptWorldProperty>())];
});
