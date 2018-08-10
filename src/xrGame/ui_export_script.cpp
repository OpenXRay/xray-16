#include "pch_script.h"
#include "MainMenu.h"
#include "UIGameCustom.h"
#include "UI/UIScriptWnd.h"
#include "ScriptXmlInit.h"

#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

CMainMenu* MainMenu();

SCRIPT_EXPORT(UIRegistrator, (), {
    module(luaState)[class_<CGameFont>("CGameFont")
                         .enum_("EAligment")[value("alLeft", int(CGameFont::alLeft)),
                             value("alRight", int(CGameFont::alRight)), value("alCenter", int(CGameFont::alCenter))],

		class_<CMainMenu>("CMainMenu")
	];

    module(luaState, "main_menu")[def("get_main_menu", &MainMenu)];
});
