#include "stdafx.h"

#include "XR_IOConsole.h"
#include "xr_ioc_cmd.h"

#include "xrScriptEngine/script_space.hpp"

void CConsole::script_register(lua_State* luaState)
{
    using namespace luabind;

    module(luaState)
    [
        def("get_console", +[]()
        {
            return Console;
        }),

        class_<CConsole>("CConsole")
            .def("execute", &CConsole::Execute)
            .def("execute_script", &CConsole::ExecuteScript)
            .def("show", &CConsole::Show)
            .def("hide", &CConsole::Hide)

            .def("get_string", &CConsole::GetString)
            .def("get_integer", +[](const CConsole* self, pcstr cmd)
            {
                int min = 0, max = 0;
                const int val = self->GetInteger(cmd, min, max);
                return val;
            })
            .def("get_bool", +[](const CConsole* self, pcstr cmd)
            {
                return self->GetBool(cmd);
            })
            .def("get_float", +[](const CConsole* self, pcstr cmd)
            {
                float min = 0.0f, max = 0.0f;
                const float val = self->GetFloat(cmd, min, max);
                return val;
            })
            .def("get_token", &CConsole::GetToken)
            .def("execute_deferred", +[](CConsole*, pcstr string_to_execute)
            {
                Engine.Event.Defer("KERNEL:console", size_t(xr_strdup(string_to_execute)));
            }),

        def("android_launcher_controls_display", +[]()
        {
#if defined(XR_PLATFORM_ANDROID)
            return strstr(Core.Params, "-android-render-width ") != nullptr;
#else
            return false;
#endif
        }),
        def("renderer_allow_override", +[]()
        {
            return renderer_allow_override;
        })
    ];
}
