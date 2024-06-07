#include "pch_script.h"
#include "map_location.h"
#include "map_manager.h"
#include "xrScriptEngine/ScriptExporter.hpp"

SCRIPT_EXPORT(CMapManager, (),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CMapManager>("CMapManager")
            .def(constructor<>())
            .def("RemoveMapLocationByObjectID", &CMapManager::RemoveMapLocationByObjectID)
            .def("RemoveMapLocation", (void (CMapManager::*)(CMapLocation*))&CMapManager::RemoveMapLocation)
            .def("DisableAllPointers", &CMapManager::DisableAllPointers)
    ];
});

SCRIPT_EXPORT(CMapLocation, (),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CMapLocation>("CMapLocation")
            //.def(constructor<>())
            .def("HintEnabled", &CMapLocation::HintEnabled)
            .def("GetHint", &CMapLocation::GetHint)
            .def("SetHint", +[](CMapLocation* self, pcstr hint)
            {
                self->SetHint(hint);
            })
            .def("PointerEnabled", &CMapLocation::PointerEnabled)
            .def("EnablePointer", &CMapLocation::EnablePointer)
            .def("DisablePointer", &CMapLocation::DisablePointer)
            .def("SpotSize", &CMapLocation::SpotSize)
            .def("IsUserDefined", &CMapLocation::IsUserDefined)
            .def("SetUserDefinedFlag", &CMapLocation::SetUserDefinedFlag)
            .def("HighlightSpot", &CMapLocation::HighlightSpot)
            .def("Collidable", &CMapLocation::Collidable)
            .def("SpotEnabled", &CMapLocation::SpotEnabled)
            .def("EnableSpot", &CMapLocation::EnableSpot)
            .def("DisableSpot", &CMapLocation::DisableSpot)
            .def("GetLevelName", +[](CMapLocation* self)
            {
                return self->GetLevelName().c_str();
            })
            .def("GetPosition", &CMapLocation::GetPosition)
            .def("ObjectID", &CMapLocation::ObjectID)
            .def("GetLastPosition", &CMapLocation::GetLastPosition)
    ];
});
