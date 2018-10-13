#include "pch_script.h"
#include "MainMenu.h"
#include "UIGameCustom.h"
#include "ui/UIScriptWnd.h"
#include "ui/UIMapInfo.h"
#include "ScriptXMLInit.h"
#include "login_manager.h"
#include "account_manager.h"
#include "profile_store.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;

CMainMenu* MainMenu();

ICF static void UIRegistratorScriptExport(lua_State* luaState)
{
    module(luaState)
    [
        class_<CGameFont>("CGameFont")
            .enum_("EAligment")
            [
                value("alLeft", int(CGameFont::alLeft)),
                value("alRight", int(CGameFont::alRight)),
                value("alCenter", int(CGameFont::alCenter))
            ],

        class_<Patch_Dawnload_Progress>("Patch_Dawnload_Progress")
            .def("GetInProgress", &Patch_Dawnload_Progress::GetInProgress)
            .def("GetStatus", &Patch_Dawnload_Progress::GetStatus)
            .def("GetFlieName", &Patch_Dawnload_Progress::GetFlieName)
            .def("GetProgress", &Patch_Dawnload_Progress::GetProgress),

        class_<CMainMenu>("CMainMenu")
            .def("GetPatchProgress", &CMainMenu::GetPatchProgress)
            .def("CancelDownload", &CMainMenu::CancelDownload)
            .def("ValidateCDKey", &CMainMenu::ValidateCDKey)
            .def("GetGSVer", &CMainMenu::GetGSVer)
            .def("GetCDKey", &CMainMenu::GetCDKeyFromRegistry)
            .def("GetPlayerName", &CMainMenu::GetPlayerName)
            .def("GetDemoInfo", &CMainMenu::GetDemoInfo)
#ifdef WINDOWS
            .def("GetLoginMngr", &CMainMenu::GetLoginMngr)
            .def("GetAccountMngr", &CMainMenu::GetAccountMngr)
            .def("GetProfileStore", &CMainMenu::GetProfileStore)
#endif
    ];
    module(luaState, "main_menu")
    [
        def("get_main_menu", &MainMenu)
    ];
}
SCRIPT_EXPORT_FUNC(UIRegistrator, (), UIRegistratorScriptExport)
