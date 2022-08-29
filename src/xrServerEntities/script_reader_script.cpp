////////////////////////////////////////////////////////////////////////////
//	Module 		: script_reader_script.cpp
//	Created 	: 05.10.2004
//  Modified 	: 05.10.2004
//	Author		: Dmitriy Iassenev
//	Description : Script reader
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(IReader, (),
{
    using namespace luabind;

    module(luaState)
    [
        class_<IReader>("reader")
            .def("r_seek", &IReader::seek)
            .def("r_tell", &IReader::tell)
            .def("r_vec3", +[](IReader* self, Fvector* arg0) { self->r_fvector3(*arg0); })
            .def("r_bool", +[](IReader* self) { return !!self->r_u8(); })
            .def("r_float", &IReader::r_float)
            .def("r_u64", &IReader::r_u64)
            .def("r_s64", &IReader::r_s64)
            .def("r_u32", &IReader::r_u32)
            .def("r_s32", &IReader::r_s32)
            .def("r_u16", &IReader::r_u16)
            .def("r_s16", &IReader::r_s16)
            .def("r_u8", &IReader::r_u8)
            .def("r_s8", &IReader::r_s8)
            .def("r_float_q16", &IReader::r_float_q16)
            .def("r_float_q8", &IReader::r_float_q8)
            .def("r_angle16", &IReader::r_angle16)
            .def("r_angle8", &IReader::r_angle8)
            .def("r_dir", &IReader::r_dir)
            .def("r_sdir", &IReader::r_sdir)
            .def("r_stringZ", +[](IReader* self)
            {
                shared_str temp; // XXX: will the string be saved in shared memory?
                self->r_stringZ(temp);
                return temp.c_str();
            })
            .def("r_elapsed", &IReader::elapsed)
            .def("r_advance", &IReader::advance)
            .def("r_eof", &IReader::eof)
    ];
});
