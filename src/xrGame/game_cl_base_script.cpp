#include "pch_script.h"
#include "game_cl_base.h"

using namespace luabind;

#pragma optimize("s", on)
SCRIPT_EXPORT(SZoneMapEntityData, (),
{
    module(luaState)
    [
        class_<SZoneMapEntityData>("SZoneMapEntityData")
            .def(constructor<>())
            .def_readwrite("pos", &SZoneMapEntityData::pos)
            .def_readwrite("color", &SZoneMapEntityData::color)
    ];
});
SCRIPT_EXPORT(SZoneMapEntities, (),
{
    module(luaState)
    [
        class_<xr_vector<SZoneMapEntityData>>("ZoneMapEntities")
            .def("push_back", (void (xr_vector<SZoneMapEntityData>::*)(const SZoneMapEntityData&))
                (&xr_vector<SZoneMapEntityData>::push_back))
    ];
});

SCRIPT_EXPORT(RPoint, (),
{
    module(luaState)
    [
        class_<RPoint>("RPoint")
            .def(constructor<>())
            .def_readwrite("P", &RPoint::P)
            .def_readwrite("A", &RPoint::A)
    ];
});
