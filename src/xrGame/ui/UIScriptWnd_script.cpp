#include "pch_script.h"

// UI-controls
#include "UIScriptWnd.h"
#include "uiscriptwnd_script.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "xrScriptEngine/Functor.hpp"

using namespace luabind;

extern export_class& script_register_ui_window1(export_class&);
extern export_class& script_register_ui_window2(export_class&);

SCRIPT_EXPORT(CUIDialogWndEx, (CUIDialogWnd, IFactoryObject), {
    export_class instance("CUIScriptWnd");

    module(luaState)[script_register_ui_window2(script_register_ui_window1(instance)).def("Load", &BaseType::Load)];
});

export_class& script_register_ui_window1(export_class& instance)
{
    instance.def(constructor<>())
        .def("AddCallback", (void (BaseType::*)(LPCSTR, s16, const luabind::functor<void>&, const luabind::object&)) &
                BaseType::AddCallback)
        .def("Register", (void (BaseType::*)(CUIWindow*, LPCSTR)) & BaseType::Register);
    return (instance);
}
