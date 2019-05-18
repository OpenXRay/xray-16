////////////////////////////////////////////////////////////////////////////
//	Module 		: script_render_device_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script render device script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "xrEngine/x_ray.h"

using namespace luabind;

bool is_device_paused(CRenderDevice* d) { return !!Device.Paused(); }
void set_device_paused(CRenderDevice* d, bool b) { Device.Pause(b, TRUE, FALSE, "set_device_paused_script"); }
bool is_app_ready() { return pApp->IsLoaded(); }
u32 time_global(const CRenderDevice* self)
{
    THROW(self);
    return (self->dwTimeGlobal);
}

void set_season_clr(CRenderDevice* self, float c1, float w1, float c2, float w2)
{
    self->SeasonValColor[0] = c1; 
    self->SeasonValColor[1] = w1; 
    self->SeasonValColor[2] = c2;
    self->SeasonValColor[3] = w2;

    Msg("%f;%f;%f;%f", 
        self->SeasonValColor[0],
        self->SeasonValColor[1],
        self->SeasonValColor[2],
        self->SeasonValColor[3]
        );
}

SCRIPT_EXPORT(CRenderDevice, (), {
    module(luaState)[class_<CRenderDevice>("render_device")
                         .def_readonly("width", &CRenderDevice::dwWidth)
                         .def_readonly("height", &CRenderDevice::dwHeight)
                         .def_readonly("time_delta", &CRenderDevice::dwTimeDelta)
                         .def_readonly("f_time_delta", &CRenderDevice::fTimeDelta)
                         .def_readonly("cam_pos", &CRenderDevice::vCameraPosition)
                         .def_readonly("cam_dir", &CRenderDevice::vCameraDirection)
                         .def_readonly("cam_top", &CRenderDevice::vCameraTop)
                         .def_readonly("cam_right", &CRenderDevice::vCameraRight)
                         //			.def_readonly("view",					&CRenderDevice::mView)
                         //			.def_readonly("projection",				&CRenderDevice::mProject)
                         //			.def_readonly("full_transform",			&CRenderDevice::mFullTransform)
                         .def_readonly("fov", &CRenderDevice::fFOV)
                         .def_readonly("aspect_ratio", &CRenderDevice::fASPECT)
                         .def("time_global", &time_global)
                         .def_readonly("precache_frame", &CRenderDevice::dwPrecacheFrame)
                         .def_readonly("frame", &CRenderDevice::dwFrame)
                         .def("is_paused", &is_device_paused)
                         .def("pause", &set_device_paused)
                            //Graff46 seasons
                         .def("set_season_colors", &set_season_clr),
        def("app_ready", &is_app_ready)];
});
