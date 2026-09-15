////////////////////////////////////////////////////////////////////////////
//	Module 		: script_reader_script.cpp
//	Created 	: 05.10.2004
//  Modified 	: 05.10.2004
//	Author		: Dmitriy Iassenev
//	Description : Script reader
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"

#include "base_client_classes_wrappers.h"

void CScriptReader::script_register(lua_State* luaState)
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
            .def("r_u32", +[](IReader* self) -> lua_Number
            {
                const u32 value = self->r_u32();

                // SoC scripts use -1 as a u32 sentinel. In the original 32-bit
                // engine, Lua received 0xffffffff as -1. Keep that behavior in
                // SoC mode so original saves do not contain invalid object IDs.
                if (ShadowOfChernobylMode)
                    return static_cast<s32>(value);

                return value;
            })
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
}
