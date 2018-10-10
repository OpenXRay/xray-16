////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_script.cpp
//	Created 	: 19.09.2002
//  Modified 	: 23.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Server objects script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer_Objects.h"
#include "PHNetState.h"
#include "xrServer_script_macroses.h"
#include "script_ini_file.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

LPCSTR get_section_name(const CSE_Abstract* abstract) { return (abstract->name()); }
LPCSTR get_name(const CSE_Abstract* abstract) { return (abstract->name_replace()); }
CScriptIniFile* get_spawn_ini(CSE_Abstract* abstract) { return ((CScriptIniFile*)&abstract->spawn_ini()); }
template <typename T>
struct CWrapperBase : public T, public luabind::wrap_base
{
    typedef T inherited;
    typedef CWrapperBase<T> self_type;

    IC CWrapperBase(LPCSTR section) : T(section) {}
    virtual void STATE_Read(NET_Packet& p1) { call<void>("STATE_Read", &p1); }
    static void STATE_Read_static(inherited* ptr, NET_Packet* p1)
    {
        Log("Attempt to call pure virtual method STATE_Read in CSE_Abstract");
        // ptr->self_type::inherited::STATE_Read(*p1);
    }
    virtual void STATE_Write(NET_Packet& p1) { call<void>("STATE_Write", &p1); }
    static void STATE_Write_static(inherited* ptr, NET_Packet* p1)
    {
        Log("Attempt to call pure virtual method STATE_Write in CSE_Abstract");
        // ptr->self_type::inherited::STATE_Write(*p1);
    }

    virtual void UPDATE_Read(NET_Packet& p1) { call<void>("UPDATE_Read", &p1); }
    static void UPDATE_Read_static(inherited* ptr, NET_Packet* p1)
    {
        Log("Attempt to call pure virtual method UPDATE_Read in CSE_Abstract");
        // ptr->self_type::inherited::UPDATE_Read(*p1);
    }
    virtual void UPDATE_Write(NET_Packet& p1) { call<void>("UPDATE_Write", &p1); }
    static void UPDATE_Write_static(inherited* ptr, NET_Packet* p1)
    {
        Log("Attempt to call pure virtual method UPDATE_Write in CSE_Abstract");
        // ptr->self_type::inherited::UPDATE_Write(*p1);
    }
};

SCRIPT_EXPORT(CPureServerObject, (), {
    module(luaState)[class_<ISerializable>("iserializable"),
        class_<IPureServerObject, ISerializable>("ipure_server_object"),
        class_<CPureServerObject, IPureServerObject>("cpure_server_object")
        //			.def(		constructor<>())
    ];
});

SCRIPT_EXPORT(CSE_Abstract, (CPureServerObject), {
    typedef CWrapperBase<CSE_Abstract> WrapType;
    typedef CSE_Abstract BaseType;
    module(luaState)[class_<CSE_Abstract, CPureServerObject, default_holder, WrapType>("cse_abstract")
                         .def_readonly("id", &BaseType::ID)
                         .def_readonly("parent_id", &BaseType::ID_Parent)
                         .def_readonly("script_version", &BaseType::m_script_version)
                         .def_readwrite("position", &BaseType::o_Position)
                         .def_readwrite("angle", &BaseType::o_Angle)
                         .def("section_name", &get_section_name)
                         .def("name", &get_name)
                         .def("clsid", &BaseType::script_clsid)
                         .def("spawn_ini", &get_spawn_ini)
                         .def("STATE_Read", &BaseType::STATE_Read, &WrapType::STATE_Read_static)
                         .def("STATE_Write", &BaseType::STATE_Write, &WrapType::STATE_Write_static)
                         .def("UPDATE_Read", &BaseType::UPDATE_Read, &WrapType::UPDATE_Read_static)
                         .def("UPDATE_Write", &BaseType::UPDATE_Write, &WrapType::UPDATE_Write_static)
        //			.def(		constructor<LPCSTR>())
    ];
});

SCRIPT_EXPORT(CSE_Shape, (), {
    module(luaState)[class_<CSE_Shape>("cse_shape")
        //			.def(		constructor<>())
    ];
});

SCRIPT_EXPORT(CSE_Visual, (), {
    module(luaState)[class_<CSE_Visual>("cse_visual")
        //			.def(		constructor<>())
        //			.def(		constructor<LPCSTR>())
    ];
});

SCRIPT_EXPORT(CSE_Motion, (), {
    module(luaState)[class_<CSE_Motion>("cse_motion")
        //			.def(		constructor<>())
        //			.def(		constructor<LPCSTR>())
    ];
});

SCRIPT_EXPORT(CSE_Spectator, (CSE_Abstract),
    { module(luaState)[luabind_class_abstract1(CSE_Spectator, "cse_spectator", CSE_Abstract)]; });

SCRIPT_EXPORT(CSE_Temporary, (CSE_Abstract),
    { module(luaState)[luabind_class_abstract1(CSE_Temporary, "cse_temporary", CSE_Abstract)]; });
