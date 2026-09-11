////////////////////////////////////////////////////////////////////////////
//	Module 		: script_net packet_script.cpp
//	Created 	: 06.02.2004
//  Modified 	: 24.06.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay Script net packet class script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"

#include "base_client_classes_wrappers.h"

void CScriptNetPacket::script_register(lua_State* luaState)
{
    using namespace luabind;
    using namespace luabind::policy;

    module(luaState)
    [
        class_<ClientID>("ClientID")
            .def(constructor<>())
            .def("value", &ClientID::value)
            .def("set", &ClientID::set)
            .def(self == other<ClientID>()),

        class_<NET_Packet>("net_packet")
            .def(constructor<>())
            .def("w_begin", &NET_Packet::w_begin)
            //			.def("w",				&NET_Packet::w				)
            //			.def("w_seek",			&NET_Packet::w_seek			)
            .def("w_tell", &NET_Packet::w_tell)
            .def("w_vec3", &NET_Packet::w_vec3)
            .def("w_float", &NET_Packet::w_float)
            .def("w_u64", &NET_Packet::w_u64)
            .def("w_s64", &NET_Packet::w_s64)
            .def("w_u32", &NET_Packet::w_u32)
            .def("w_s32", &NET_Packet::w_s32)
            .def("w_u16", &NET_Packet::w_u16)
            .def("w_s16", &NET_Packet::w_s16)
            .def("w_u8", &NET_Packet::w_u8)
            //			.def("w_s8",			&NET_Packet::w_s8			)
            .def("w_bool", +[](NET_Packet* self, bool value)
            {
                self->w_u8(value ? 1 : 0);
            })
            .def("w_float_q16", &NET_Packet::w_float_q16)
            .def("w_float_q8", &NET_Packet::w_float_q8)
            .def("w_angle16", &NET_Packet::w_angle16)
            .def("w_angle8", &NET_Packet::w_angle8)
            .def("w_dir", &NET_Packet::w_dir)
            .def("w_sdir", &NET_Packet::w_sdir)
            .def("w_stringZ", (void (NET_Packet::*)(pcstr))(&NET_Packet::w_stringZ))
            .def("w_matrix", &NET_Packet::w_matrix)
            .def("w_clientID", &NET_Packet::w_clientID)
            .def("w_chunk_open8", &NET_Packet::w_chunk_open8, out_value<2>())
            .def("w_chunk_close8", &NET_Packet::w_chunk_close8)
            .def("w_chunk_open16", &NET_Packet::w_chunk_open16, out_value<2>())
            .def("w_chunk_close16", &NET_Packet::w_chunk_close16)
            .def("r_begin", &NET_Packet::r_begin, out_value<2>())
            //			.def("r",				&NET_Packet::r				)
            .def("r_seek", &NET_Packet::r_seek)
            .def("r_tell", &NET_Packet::r_tell)
            // XXX: used as r_vec3(vec) -- remove pure_out_value?
            .def("r_vec3", (void (NET_Packet::*)(Fvector&))(&NET_Packet::r_vec3), pure_out_value<2>())
            .def("r_bool", +[](NET_Packet* self)
            {
                return (!!self->r_u8());
            })
            .def("r_float", (float (NET_Packet::*)())(&NET_Packet::r_float))
            .def("r_u64", (u64(NET_Packet::*)())(&NET_Packet::r_u64))
            .def("r_s64", (s64(NET_Packet::*)())(&NET_Packet::r_s64))
            .def("r_u32", (u32(NET_Packet::*)())(&NET_Packet::r_u32))
            .def("r_s32", (s32(NET_Packet::*)())(&NET_Packet::r_s32))
            .def("r_u16", (u16(NET_Packet::*)())(&NET_Packet::r_u16))
            .def("r_s16", (s16(NET_Packet::*)())(&NET_Packet::r_s16))
            .def("r_u8", (u8(NET_Packet::*)())(&NET_Packet::r_u8))
            .def("r_s8", (s8(NET_Packet::*)())(&NET_Packet::r_s8))
            // XXX: should use pure_out_value here
            .def("r_angle16", &NET_Packet::r_angle16, out_value<2>())
            .def("r_angle8", &NET_Packet::r_angle8, out_value<2>())
            .def("r_dir", &NET_Packet::r_dir)
            .def("r_sdir", &NET_Packet::r_sdir)
            .def("r_stringZ", +[](NET_Packet* self)
            {
                shared_str temp;
                self->r_stringZ(temp);
                return (temp.c_str());
            })
            .def("r_matrix", &NET_Packet::r_matrix)
            .def("r_clientID", +[](NET_Packet* self)
            {
                ClientID clientID;
                self->r_clientID(clientID);
                return clientID;
            })
            .def("r_elapsed", &NET_Packet::r_elapsed)
            .def("r_advance", &NET_Packet::r_advance)
            .def("r_eof", +[](NET_Packet* self)
            {
                return self->r_eof();
            })
    ];
}
